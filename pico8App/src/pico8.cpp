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

#include <asynNDArrayDriver.h>

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

int Pico8::readDevice(void *buf, int samp, int *count) {
	int ret;
	int btrans;
	
	/* read samples of 4 bytes each for all 8 channels */
	ret = read(mHandle, buf, samp * 8 * 4);
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

int Pico8::getBTrans(int *val) {
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

void Pico8::setAcquire(int value) {
    if (value && !acquiring_) {
        /* Send an event to wake up the simulation task */
        epicsEventSignal(this->startEventId_);
    }
    if (!value && acquiring_) {
        /* This was a command to stop acquisition */
        /* Send the stop event */
        epicsEventSignal(this->stopEventId_);
    }
}

/** Template function to acquire the detector data for any data type */
template <typename epicsType> int Pico8::acquireArraysT()
{
    size_t dims[2];
    int numPoints;
    int count;
    NDDataType_t dataType;
    epicsType *pData;
    int ret;

    getIntegerParam(NDDataType, (int *)&dataType);
    getIntegerParam(Pico8NumPoints, &numPoints);

    dims[0] = PICO8_NR_CHANNELS;
    dims[1] = numPoints;

    /* device returns proper sample / channel data layout, same as in
     * ADCSimDetector:
     *               channels
     * sample 1 : 0 1 2 3 4 5 6 7
     * sample 2 : 0 1 2 3 4 5 6 7
     * ...
     */
    if (this->pArrays[0]) this->pArrays[0]->release();
    this->pArrays[0] = pNDArrayPool->alloc(2, dims, dataType, 0, 0);
    pData = (epicsType *)this->pArrays[0]->pData;
    memset(pData, 0, PICO8_NR_CHANNELS * numPoints * sizeof(epicsType));

    ret = readDevice(pData, numPoints, &count);
    if (ret == -1) {
    	return ret;
    }

    return 0;
}

/** Computes the new image data */
int Pico8::acquireArrays() {
	int ret = 1;
    int dataType;
    getIntegerParam(NDDataType, &dataType);

    switch (dataType) {
        case NDInt8:
            ret = acquireArraysT<epicsInt8>();
            break;
        case NDUInt8:
        	ret = acquireArraysT<epicsUInt8>();
            break;
        case NDInt16:
        	ret = acquireArraysT<epicsInt16>();
            break;
        case NDUInt16:
        	ret = acquireArraysT<epicsUInt16>();
            break;
        case NDInt32:
        	ret = acquireArraysT<epicsInt32>();
            break;
        case NDUInt32:
        	ret = acquireArraysT<epicsUInt32>();
            break;
        case NDFloat32:
        	ret = acquireArraysT<epicsFloat32>();
            break;
        case NDFloat64:
        	ret = acquireArraysT<epicsFloat64>();
            break;
    }
    return ret;
}

void Pico8::dataTask(void) {
    int status = asynSuccess;
    NDArray *pImage;
    epicsTimeStamp startTime;
    int arrayCounter;
    int i;
	static const char *functionName = "dataTask";

	sleep(1);

	this->lock();

	if (mHandle == -1) {
		printf("%s:%s: Data thread will not start...\n", driverName, functionName);
		this->unlock();
		return;
	}

	printf("%s:%s: Data thread started...\n", driverName, functionName);


    this->lock();
    /* Loop forever */
    while (1) {
        /* Has acquisition been stopped? */
        status = epicsEventTryWait(this->stopEventId_);
        if (status == epicsEventWaitOK) {
            acquiring_ = 0;
        }

        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquiring_) {
          /* Release the lock while we wait for an event that says acquire has started, then lock again */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            this->unlock();
            status = epicsEventWait(this->startEventId_);
            this->lock();
            acquiring_ = 1;
        }

        /* Get the data */
        acquireArrays();

        pImage = this->pArrays[0];

        /* Put the frame number and time stamp into the buffer */
        pImage->uniqueId = uniqueId_++;
        getIntegerParam(NDArrayCounter, &arrayCounter);
        arrayCounter++;
        setIntegerParam(NDArrayCounter, arrayCounter);
        epicsTimeGetCurrent(&startTime);
        pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
        updateTimeStamp(&pImage->epicsTS);

        /* Get any attributes that have been defined for this driver */
        this->getAttributes(pImage->pAttributeList);

        /* Call the NDArray callback */
        /* Must release the lock here, or we can get into a deadlock, because we can
         * block on the plugin lock, and the plugin can be calling us */
        this->unlock();
        doCallbacksGenericPointer(pImage, NDArrayData, 0);
        this->lock();

        /* Call the callbacks to update any changes */
        for (i = 0; i < PICO8_NR_CHANNELS; i++) {
            callParamCallbacks(i);
        }
    }

	printf("Data thread is down!\n");
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
    if (function == Pico8Acquire) {
        setAcquire(value);
    } else if (function == Pico8Range) {
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
    	status = asynNDArrayDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(addr);

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

/** Report status of the driver.
 * Prints details about the detector in us if details>0.
 * It then calls the ADDriver::report() method.
 * \param[in] fp File pointed passed by caller where the output is written to.
 * \param[in] details Controls the level of detail in the report. */
void Pico8::report(FILE *fp, int details) {
//	static const char *functionName = "report";

	fprintf(fp, "CAENELS AMC Pico8 port=%s\n", this->portName);
	if (details > 0) {
		/* XXX: Add some details? */
	}
	// Call the base class method
	asynNDArrayDriver::report(fp, details);
}

/** Constructor for Pico8; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] devicePath Path to the /dev entry (usually /dev/amc_pico)
  * \param[in] numPoints The initial number of points.
  * \param[in] dataType The initial data type (NDDataType_t) of the arrays that this driver will create.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
Pico8::Pico8(const char *portName, const char *devicePath, int numPoints,
			NDDataType_t dataType, int maxBuffers, size_t maxMemory,
			int priority, int stackSize)
    /* Invoke the base class constructor */
	: asynNDArrayDriver(portName, PICO8_NR_CHANNELS, NUM_PICO8_PARAMS,
			maxBuffers, maxMemory,
           0, 0, /* No interfaces beyond those set in ADDriver.cpp */
           ASYN_CANBLOCK | ASYN_MULTIDEVICE, /* asyn flags*/
           1,                                /* autoConnect=1 */
           priority, stackSize),
		   uniqueId_(0), acquiring_(0)
{
	int status = asynSuccess;
    static const char *functionName = "Pico8";
	
	mHandle = -1;

	/* Create the epicsEvents for signaling to the simulate task when acquisition starts and stops */
    this->startEventId_ = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId_) {
        printf("%s:%s epicsEventCreate failure for start event\n",
            driverName, functionName);
        return;
    }
    this->stopEventId_ = epicsEventCreate(epicsEventEmpty);
    if (!this->stopEventId_) {
        printf("%s:%s epicsEventCreate failure for stop event\n",
            driverName, functionName);
        return;
    }

	/* Create an EPICS exit handler */
	epicsAtExit(exitHandler, this);

	openDevice();
	
    createParam(Pico8AcquireString,          asynParamInt32,       &Pico8Acquire);
    createParam(Pico8NumPointsString,        asynParamInt32,       &Pico8NumPoints);
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


	/* Set some default values for parameters */
	status = setIntegerParam(Pico8Acquire,    0);
	status |= setIntegerParam(Pico8NumPoints, 1000);
	status |= setIntegerParam(Pico8Range,     0);
	status |= setIntegerParam(Pico8FSamp,     100000);
	status |= setIntegerParam(Pico8BTrans,    0);
	status |= setIntegerParam(Pico8TrgMode,   0);
	status |= setIntegerParam(Pico8TrgCh,     0);
	status |= setIntegerParam(Pico8TrgLevel,  100);
	status |= setIntegerParam(Pico8TrgLength, 10);
	status |= setIntegerParam(Pico8RingBuf,   0);
	status |= setIntegerParam(Pico8GateMux,   0);
	status |= setIntegerParam(Pico8ConvMux,   0);

	//	status = setIntegerParam(NDArraySizeX, PICO8_MAX_SAMPLES);
	//	status |= setIntegerParam(NDArraySizeY, 1);
	status |= setIntegerParam(NDDataType, dataType);
	//	status |= setIntegerParam(NDArraySize, PICO8_MAX_SAMPLES * 1 * sizeof(epicsFloat32));

	callParamCallbacks();

	if (status) {
		printf("%s:%s: unable to set parameters\n", driverName, functionName);
		return;
	}

	if (stackSize == 0) {
		stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
	}

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
	printf("Data thread is already down!\n");
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
extern "C" int Pico8Configure(const char *portName,
								const char *devicePath,
								int numPoints,
								int dataType,
								int maxBuffers,
								size_t maxMemory,
								int priority,
								int stackSize)
{
    new Pico8(portName,
    		devicePath,
			numPoints,
			(NDDataType_t)dataType,
			(maxBuffers < 0) ? 0 : maxBuffers,
			(maxMemory < 0) ? 0 : maxMemory,
			priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",   iocshArgString};
static const iocshArg initArg1 = { "devicePath", iocshArgString };
static const iocshArg initArg2 = { "# points",   iocshArgInt};
static const iocshArg initArg3 = { "dataType",   iocshArgInt};
static const iocshArg initArg4 = { "maxBuffers", iocshArgInt};
static const iocshArg initArg5 = { "maxMemory",  iocshArgInt};
static const iocshArg initArg6 = { "priority",   iocshArgInt};
static const iocshArg initArg7 = { "stackSize",  iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
											&initArg5,
											&initArg6,
											&initArg7};
static const iocshFuncDef initFuncDef = {"Pico8Configure", 8, initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    Pico8Configure(args[0].sval, args[1].sval, args[2].ival,
            args[3].ival, args[4].ival, args[5].ival, args[6].ival,
			args[7].ival);
}

extern "C" void Pico8Register(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

extern "C" {
epicsExportRegistrar(Pico8Register);
}
