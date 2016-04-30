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

#include <epicsString.h>
#include <epicsMutex.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsExport.h>
#include <epicsExit.h>

#include <asynDriver.h>

#include <iocsh.h>

#include "pico8.h"


static const char *driverName = "Pico8";
static void dataTaskC(void *drvPvt);


static void dataTaskC(void *drvPvt) {
	Pico8 *pPvt = (Pico8 *) drvPvt;
	pPvt->dataTask();
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

	printf("%s:%s: Data thread started...\n", driverName, functionName);

	this->lock();

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


/** Callback function that is called by the NDArray driver with new NDArray data.
  * \param[in] pArray  The NDArray from the callback.
  */
//void Pico8::processCallbacks(NDArray *pArray)
//{
//    /* It is called with the mutex already locked.
//     * It unlocks it during long calculations when private structures don't
//     * need to be protected.
//     */
//    //size_t sizeX=0;
//    //int i;
//    NDArrayInfo arrayInfo;
//    static const char* functionName = "processCallbacks";
//
//    /* Call the base class method */
//    NDPluginDriver::processCallbacks(pArray);
//
//    pArray->getInfo(&arrayInfo);
//
//    /* Do work here */
//
//    int arrayCallbacks = 0;
//    getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
//    if (arrayCallbacks == 1) {
//        NDArray *pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
//        if (NULL != pArrayOut) {
//            this->getAttributes(pArrayOut->pAttributeList);
//            this->unlock();
//            doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
//            this->lock();
//        }
//        else {
//            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
//                "%s::%s: Couldn't allocate output array. Further processing terminated.\n",
//                driverName, functionName);
//        }
//    }
//
//    callParamCallbacks();
//}

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

    if (function == Pico8Range) {
        status = (asynStatus) getIntegerParam(addr, function, value);
    } else if (function == Pico8FSamp) {
        status = (asynStatus) getIntegerParam(addr, function, value);
    } else if (function < FIRST_PICO8_PARAM) {
		/* If this parameter belongs to a base class call its method */
    	status = ADDriver::readInt32(pasynUser, value);
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
    status = setDoubleParam(addr, function, value);

    /* Do param handling here */
    if (function == Pico8Range) {
    } else if (function == Pico8FSamp) {
    } else if (function == Pico8BTrans) {
    } else if (function == Pico8TrgMode) {
    } else if (function == Pico8TrgCh) {
    } else if (function == Pico8TrgLevel) {
    } else if (function == Pico8TrgLength) {
    } else if (function == Pico8RingBuf) {
    } else if (function == Pico8GateMux) {
    } else if (function == Pico8ConvMux) {

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
asynStatus Pico8::readInt32Array(asynUser *pasynUser, epicsInt32 *value,
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

	} else {
		status = ADDriver::readInt32Array(pasynUser, value, nElements, nIn);
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
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
				   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
	int status = asynSuccess;
    static const char *functionName = "Pico8";
	
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
	status |= setIntegerParam(NDDataType, NDInt32);
	status |= setIntegerParam(NDArraySize, PICO8_MAX_SAMPLES * 1 * sizeof(epicsFloat32));
	status |= setDoubleParam(ADShutterOpenDelay, 0.);
	status |= setDoubleParam(ADShutterCloseDelay, 0.);


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
