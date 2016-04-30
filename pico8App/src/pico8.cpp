/*
 * pico8.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: hinkokocevar
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsExport.h>
#include <epicsExit.h>

#include <asynDriver.h>

#include <iocsh.h>

#include <amc_pico.h>
#include "pico8.h"


static const char *driverName = "Pico8";
static void dataTaskC(void *drvPvt);
static void exitHandler(void *drvPvt);

static void dataTaskC(void *drvPvt) {
	Pico8 *pPvt = (Pico8 *) drvPvt;
	pPvt->dataTask();
}

int Pico8::openDevice() {
	int ret;
	
	ret = open(mDevicePath, O_RDWR);
	if (ret == -1) {
		fprintf(stderr, "open() failed: %s\n", strerror(errno));
		return ret;
	}
	
	mHandle = ret;
	return 0;
}

void Pico8::closeDevice() {
	if (mHandle > 0) {
		close(mHandle);
	}
	mHandle = -1;
}

int Pico8::readDevice(uint32_t samp, uint32_t *count) {
	int ret;
	uint32_t btrans;
	
	/* read samples of 4 bytes each for all 8 channels */
	ret = read(mHandle, mDataBuf, samp * 8 * 4);
	if (ret == -1) {
		fprintf(stderr, "read() failed: %s\n", strerror(errno));
		return ret;
	}
	
	btrans = 0;
	ret = this->getBTrans(&btrans);
	if (ret == -1) {
		return ret;
	}
	*count = btrans;
	
	return 0;
}
	
int Pico8::setFSamp(uint32_t val) {
	int ret;
	ret = ioctl(mHandle, SET_FSAMP, &val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() SET_FSAMP failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

int Pico8::setRange(uint8_t val) {
	int ret;
	ret = ioctl(mHandle, SET_RANGE, &val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() SET_RANGE failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

int Pico8::getBTrans(uint32_t *val) {
	int ret;
	ret = ioctl(mHandle, GET_B_TRANS, val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() GET_B_TRANS failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

int Pico8::setGateMux(uint32_t val) {
	int ret;
	ret = ioctl(mHandle, SET_GATE_MUX, &val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() SET_GATE_MUX failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

int Pico8::setConvMux(uint32_t val) {
	int ret;
	ret = ioctl(mHandle, SET_CONV_MUX, &val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() SET_CONV_MUX failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

int Pico8::setRingBuf(uint32_t val) {
	int ret;
	ret = ioctl(mHandle, SET_RING_BUF, &val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() SET_RING_BUF failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

int Pico8::setTrigger(float level, int32_t length, int32_t ch, int32_t mode) {
	int ret;
	struct trg_ctrl val;
	
	val.limit = level;
	val.nr_samp = length;
	val.ch_sel = ch;
	val.mode = (trg_mode)mode;
	
	ret = ioctl(mHandle, SET_TRG, &val);
	if (ret == -1) {
		fprintf(stderr, "ioctl() SET_TRG failed: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

void Pico8::dataTask(void) {
	int acquireStatus;
	int acquiring = 0;
	int imageMode;
	int triggerMode;
	int itemp;
	NDDataType_t dataType;
	NDArray *pArray;
	epicsInt32 arrayCallbacks;
	epicsInt32 sizeX, sizeY;
	static const char *functionName = "dataTask";

	sleep(5);

	this->lock();

	if (mHandle == -1) {
		printf("%s:%s: Data thread will not start...\n", driverName, functionName);
		this->mFinish = 1;
		this->unlock();
		return;
	}

	printf("%s:%s: Data thread started...\n", driverName, functionName);
	
	while (1) {

		//Wait for event from main thread to signal that data acquisition has started.
		this->unlock();
		epicsEventWait(mDataEvent);
		asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
				"%s:%s:, got data event\n", driverName, functionName);

		if (this->mFinish) {
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
					"%s:%s: Stopping thread!\n", driverName, functionName);
			break;
		}
		this->lock();

		//Sanity check that main thread thinks we are acquiring data
		if (mAcquiringData) {
//			try {
				getIntegerParam(ADImageMode, &imageMode);
				getIntegerParam(ADTriggerMode, &triggerMode);
				if (triggerMode == ADTriggerInternal) {
					if (imageMode == ADImageSingle) {
//						checkStatus(tlccs_startScan(mInstr));
					} else if (imageMode == ADImageContinuous) {
//						checkStatus(tlccs_startScanCont(mInstr));
					}
				} else if (triggerMode == ADTriggerExternal) {
					if (imageMode == ADImageSingle) {
//						checkStatus(tlccs_startScanExtTrg(mInstr));
					} else if (imageMode == ADImageContinuous) {
//						checkStatus(tlccs_startScanContExtTrg(mInstr));
					}
				}
				acquiring = 1;
//			} catch (const std::string &e) {
//				asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: %s\n",
//						driverName, functionName, e.c_str());
//				continue;
//			}

			//Read some parameters
			getIntegerParam(NDDataType, &itemp);
			dataType = (NDDataType_t) itemp;
			getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
			getIntegerParam(NDArraySizeX, &sizeX);
			getIntegerParam(NDArraySizeY, &sizeY);
			// Reset the counters
			setIntegerParam(ADNumImagesCounter, 0);
			setIntegerParam(ADNumExposuresCounter, 0);
			callParamCallbacks();
		} else {
			asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
					"%s:%s:, Data thread is running but main thread thinks we are not acquiring.\n",
					driverName, functionName);
			acquiring = 0;
		}

		while (acquiring) {

		}	// End of while(acquiring)

		//Now clear main thread flag
		mAcquiringData = 0;
		setIntegerParam(ADAcquire, 0);
		setIntegerParam(ADStatus, ADStatusIdle);

		/* Call the callbacks to update any changes */
		callParamCallbacks();
	} // End of while(1) loop

	printf("Data thread is down!\n");
}

asynStatus Pico8::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    static const char *functionName = "readInt32";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) {
      return status;
    }
#if 0
    if (function == Pico8Range) {
        status = (asynStatus) getIntegerParam(addr, function, value);
    } else if (function == Pico8FSamp) {
        status = (asynStatus) getIntegerParam(addr, function, value);
    } else if (function < FIRST_PICO8_PARAM) {
		/* If this parameter belongs to a base class call its method */
    	status = ADDriver::readInt32(pasynUser, value);
    }
#endif
	if (function < FIRST_PICO8_PARAM) {
		/* If this parameter belongs to a base class call its method */
    	status = ADDriver::readInt32(pasynUser, value);
    } else {
    	status = (asynStatus) getIntegerParam(addr, function, value);
    }
    
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%d\n",
              driverName, functionName, status, function, *value);
    } else {
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%d\n",
              driverName, functionName, function, *value);
    }

    return status;
}

asynStatus Pico8::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int trgMode;
	int trgLevel;
	int trgLength;
	int trgCh;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr = 0;
    static const char *functionName = "writeInt32";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) {
      return status;
    }

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setIntegerParam(addr, function, value);

    /* Do param handling here */
    if (function == Pico8Range) {
    	/* XXX: How to collect proper bitfield?!?!*/
    	assert(1 != 0);
    	this->setRange(value);
    } else if (function == Pico8FSamp) {
    	this->setFSamp(value);
    } else if (function == Pico8TrgMode
    		|| function == Pico8TrgCh
    		|| function == Pico8TrgLevel
    		|| function == Pico8TrgLength) {
		status = (asynStatus) getIntegerParam(addr, Pico8TrgMode, &trgMode);
		status = (asynStatus) getIntegerParam(addr, Pico8TrgLength, &trgLength);
		status = (asynStatus) getIntegerParam(addr, Pico8TrgCh, &trgCh);
		status = (asynStatus) getIntegerParam(addr, Pico8TrgLevel, &trgLevel);
		if (function == Pico8TrgMode) {
			trgMode = value;
		} else if (function == Pico8TrgCh) {
			trgCh = value;
		} else if (function == Pico8TrgLevel) {
			trgLevel = (float)value;
		} else if (function == Pico8TrgLength) {
			trgLength = value;
		}
    	this->setTrigger(trgLevel, trgLength, trgCh, trgMode);
    } else if (function == Pico8RingBuf) {
	    if (value > 1023) {
	    	value = 1023;
	    }
	    if (value < 0) {
	    	value = 0;
	    }
    	this->setRingBuf(value);
    } else if (function == Pico8GateMux) {
    	this->setGateMux(value);
    } else if (function == Pico8ConvMux) {
		this->setConvMux(value);
    } else if (function < FIRST_PICO8_PARAM) {
		/* If this parameter belongs to a base class call its method */
    	status = ADDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(addr);
    status = (asynStatus) callParamCallbacks();

    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%d\n",
              driverName, functionName, status, function, value);
    } else {
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%d\n",
              driverName, functionName, function, value);
    }

    return status;
}

/** Called when asyn clients call pasynFloat64Array->read().
 * This function performs actions for some parameters.
 * For all parameters it gets the value in the parameter library.
 * \param[in] pasynUser pasynUser structure that encodes the reason and address.
 * \param[in] value Value to read.
 * \param[in] nElements Number of values to read.
 * \param[in] nIn Number of values read. */
asynStatus Pico8::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
		size_t nElements, size_t *nIn) {

	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
    int addr = 0;
	static const char *functionName = "readInt32Array";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) {
      return status;
    }

	if (function == Pico8Data) {
		/* XXX: Handle data readout .. */
	} else {
		status = ADDriver::readFloat32Array(pasynUser, value, nElements, nIn);
	}

	if (status)
		asynPrint(pasynUser, ASYN_TRACE_ERROR,
				"%s:%s: error, status=%d function=%d, nElements=%d\n",
				driverName, functionName, status, function, (int )nElements);
	else
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				"%s:%s: error, status=%d function=%d, nElements=%d\n",
				driverName, functionName, status, function, (int )nElements);
	return status;
}

/** Report status of the driver.
 * Prints details about the detector in us if details>0.
 * It then calls the ADDriver::report() method.
 * \param[in] fp File pointed passed by caller where the output is written to.
 * \param[in] details Controls the level of detail in the report. */
void Pico8::report(FILE *fp, int details) {
	static const char *functionName = "report";

	fprintf(fp, "CAENELS AMC Pico8 port=%s\n", this->portName);
	if (details > 0) {
/*		try {

			checkStatus(tlccs_identificationQuery(mInstr, mManufacturerName,
							mDeviceName, mSerialNumber, mFirmwareRevision,
							mInstrumentDriverRevision));
			fprintf(fp, "  Manufacturer: %s\n", mManufacturerName);
			fprintf(fp, "  Model: %s\n", mDeviceName);
			fprintf(fp, "  Serial number: %s\n", mSerialNumber);
			fprintf(fp, "  Driver version: %s\n", mInstrumentDriverRevision);
			fprintf(fp, "  Firmware version: %s\n", mFirmwareRevision);

		} catch (const std::string &e) {
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: %s\n",
					driverName, functionName, e.c_str());
		}
*/
	}
	// Call the base class method
	ADDriver::report(fp, details);
}

/** Constructor for Pico8; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] devicePath Path to the /dev entry (usually /dev/amc_pico)
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
Pico8::Pico8(const char *portName, int maxBuffers, size_t maxMemory,
				const char *devicePath, int priority, int stackSize)
    /* Invoke the base class constructor */
    : ADDriver(portName, PICO8_NR_CHANNELS + 1, NUM_PICO8_PARAMS, maxBuffers, maxMemory,
                   asynFloat32ArrayMask | asynGenericPointerMask,
                   asynFloat32ArrayMask | asynGenericPointerMask,
				   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
	int status = asynSuccess;
    static const char *functionName = "Pico8";
	
	mHandle = -1;
	mDataBuf = calloc(PICO8_MAX_SAMPLES * 8, sizeof(float));
	
	/* Create an EPICS exit handler */
	epicsAtExit(exitHandler, this);

	openDevice();
	
    createParam(Pico8RangeString,            asynParamInt32,       &Pico8Range);
    createParam(Pico8FSampString,            asynParamInt32,       &Pico8FSamp);
    createParam(Pico8BTransString,           asynParamInt32,       &Pico8BTrans);
    createParam(Pico8TrgModeString,          asynParamInt32,       &Pico8TrgMode);
    createParam(Pico8TrgChString,            asynParamInt32,       &Pico8TrgCh);
    createParam(Pico8TrgLevelString,         asynParamInt32,       &Pico8TrgLevel);
    createParam(Pico8TrgLengthString,        asynParamInt32,       &Pico8TrgLength);
    createParam(Pico8RingBufString,          asynParamInt32,       &Pico8RingBuf);
    createParam(Pico8GateMuxString,          asynParamInt32,       &Pico8GateMux);
    createParam(Pico8ConvMuxString,          asynParamInt32,       &Pico8ConvMux);
    createParam(Pico8DataString,             asynParamFloat32Array,&Pico8Data);


	// Use this to signal the data acquisition task that acquisition has started.
	this->mDataEvent = epicsEventMustCreate(epicsEventEmpty);
	if (!this->mDataEvent) {
		printf("%s:%s epicsEventCreate failure for data event\n", driverName, functionName);
		return;
	}

	/* Set some default values for parameters */

	status = setStringParam(ADManufacturer, mManufacturerName);
	status |= setStringParam(ADModel, mDeviceName);
	status |= setIntegerParam(ADSizeX, PICO8_MAX_SAMPLES);
	status |= setIntegerParam(ADSizeY, 1);
	status |= setIntegerParam(ADBinX, 1);
	status |= setIntegerParam(ADBinY, 1);
	status |= setIntegerParam(ADMinX, 0);
	status |= setIntegerParam(ADMinY, 0);
	status |= setIntegerParam(ADMaxSizeX, PICO8_MAX_SAMPLES);
	status |= setIntegerParam(ADMaxSizeY, 1);
	status |= setIntegerParam(ADImageMode, ADImageSingle);
	status |= setDoubleParam(ADAcquireTime, 1.0);
	status |= setIntegerParam(ADNumImages, 1);
	status |= setIntegerParam(ADNumExposures, 1);
	status |= setIntegerParam(NDArraySizeX, PICO8_MAX_SAMPLES);
	status |= setIntegerParam(NDArraySizeY, 1);
	status |= setIntegerParam(NDDataType, NDFloat32);
	status |= setIntegerParam(NDArraySize, PICO8_MAX_SAMPLES * 1 * sizeof(epicsFloat32));
	status |= setDoubleParam(ADShutterOpenDelay, 0.);
	status |= setDoubleParam(ADShutterCloseDelay, 0.);

	status |= setIntegerParam(Pico8Range, 0);
	status |= setIntegerParam(Pico8FSamp, 100000);
	status |= setIntegerParam(Pico8BTrans, 0);
	status |= setIntegerParam(Pico8TrgMode, 0);
	status |= setIntegerParam(Pico8TrgCh, 0);
	status |= setIntegerParam(Pico8TrgLevel, 100);
	status |= setIntegerParam(Pico8TrgLength, 10);
	status |= setIntegerParam(Pico8FSamp, 100000);
	status |= setIntegerParam(Pico8RingBuf, 0);
	status |= setIntegerParam(Pico8GateMux, 0);
	status |= setIntegerParam(Pico8ConvMux, 0);

	callParamCallbacks();

	if (status) {
		printf("%s:%s: unable to set parameters\n", driverName, functionName);
		return;
	}

	mAcquiringData = 0;

	if (stackSize == 0) {
		stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
	}

	/* for stopping threads */
	mFinish = 0;

	/* Create the thread that does data readout */
	status = (epicsThreadCreate("Pico8DataTask",
			epicsThreadPriorityMedium, stackSize, (EPICSTHREADFUNC)dataTaskC, this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure for data task\n", driverName, functionName);
		return;
	}

	printf("Pico8 initialized OK!\n");
}

Pico8::~Pico8() {
	printf("Shutdown and freeing up memory...\n");

	this->lock();

	if (this->mFinish == 0) {
		/* make sure data thread is cleanly stopped */
		printf("Waiting for data thread...\n");
		this->mFinish = 1;
		epicsEventSignal(mDataEvent);
		sleep(1);
		printf("Data thread is down!\n");
	} else {
		printf("Data thread is already down!\n");
	}
	
	free(mDataBuf);
	closeDevice();
	
	this->unlock();
	printf("Pico8 shutdown complete!\n");	
}

/**
 * Exit handler, delete the Pico8 object.
 */
static void exitHandler(void *drvPvt) {
	Pico8 *pPico8 = (Pico8 *) drvPvt;
	delete pPico8;
}

/** Configuration command */
extern "C" int Pico8Configure(const char *portName, int maxBuffers,
								size_t maxMemory,
								const char *devicePath,
								int priority, int stackSize)
{
    new Pico8(portName, maxBuffers, maxMemory, devicePath, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg2 = { "maxMemory",iocshArgInt};
static const iocshArg initArg3 = { "devicePath", iocshArgString };
static const iocshArg initArg4 = { "priority",iocshArgInt};
static const iocshArg initArg5 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
											&initArg5};
static const iocshFuncDef initFuncDef = {"Pico8Configure", 6, initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    Pico8Configure(args[0].sval, args[1].ival, args[2].ival,
            args[3].sval, args[4].ival, args[5].ival);
}

extern "C" void Pico8Register(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

extern "C" {
epicsExportRegistrar(Pico8Register);
}
