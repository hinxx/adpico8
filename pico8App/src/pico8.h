/*
 * pico8.h
 *
 *  Created on: Apr 29, 2016
 *      Author: hinkokocevar
 */

#ifndef _PICO8_H_
#define _PICO8_H_

#include <epicsTypes.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <asynNDArrayDriver.h>

/* Number of channels is fixed to 8 */
#define PICO8_NR_CHANNELS			8
/* For strings */
#define PICO8_BUFFER_SIZE			256

#define Pico8AcquireString            "PICO8_ACQUIRE"          /* (asynParamInt32, r/w) acquire control */
#define Pico8NumPointsString          "PICO8_NUMPOINTS"        /* (asynParamInt32, r/w) number of points */
#define Pico8RangeString              "PICO8_RANGE"            /* (asynParamInt32, r/w) range */
#define Pico8FSampString              "PICO8_FSAMP"            /* (asynParamInt32, r/w) sampling frequency */
#define Pico8BTransString             "PICO8_BTRANS"           /* (asynParamInt32, r/o) no. of transferred bytes */
#define Pico8TrgModeString            "PICO8_TRGMODE"          /* (asynParamInt32, w/o) trigger mode */
#define Pico8TrgChString              "PICO8_TRGCH"            /* (asynParamInt32, w/o) trigger channel */
#define Pico8TrgLevelString           "PICO8_TRGLEVEL"         /* (asynParamInt32, w/o) trigger level */
#define Pico8TrgLengthString          "PICO8_TRGLENGTH"        /* (asynParamInt32, w/o) trigger length */
#define Pico8RingBufString            "PICO8_RINGBUF"          /* (asynParamInt32, w/o) pre-trigger sample count */
#define Pico8GateMuxString            "PICO8_GATEMUX"          /* (asynParamInt32, w/o) gate source */
#define Pico8ConvMuxString            "PICO8_CONVMUX"          /* (asynParamInt32, w/o) start of conversion source */
#define Pico8DataString               "PICO8_DATA"             /* (asynParamInt32Array, r/o) data */

/** AMC Pico8 driver; does 1-D waveforms on 8 channels.
  * Inherits from asynNDArrayDriver */
class epicsShareClass Pico8 : public asynNDArrayDriver {
public:
	Pico8(const char *portName, const char *devicePath, int numPoints,
			NDDataType_t dataType, int maxBuffers, size_t maxMemory,
			int priority, int stackSize);
	~Pico8();
	
    /* These methods override the virtual methods in the base class */
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual void report(FILE *fp, int details);

    // Should be private, but are called from C so must be public
    void dataTask(void);

private:
    /* These are the methods that are new to this class */
    template <typename epicsType> int acquireArraysT();
    int acquireArrays();
    void setAcquire(int value);
	int openDevice();
	void closeDevice();
	int readDevice(void *buf, int samp, int *count);
	int setFSamp(uint32_t val);
	int setRange(uint8_t val);
	int getBTrans(int *val);
	int setGateMux(uint32_t val);
	int setConvMux(uint32_t val);
	int setRingBuf(uint32_t val);
	int setTrigger(float level, int32_t length, int32_t ch, int32_t mode);

protected:
	int Pico8Acquire;
#define FIRST_PICO8_PARAM Pico8Acquire
    int Pico8NumPoints;
    int Pico8Range;
    int Pico8FSamp;
    int Pico8BTrans;
    int Pico8TrgMode;
    int Pico8TrgCh;
    int Pico8TrgLevel;
    int Pico8TrgLength;
    int Pico8RingBuf;
    int Pico8GateMux;
    int Pico8ConvMux;
    int Pico8Data;

    int Pico8DummyEnd;
    #define LAST_PICO8_PARAM Pico8DummyEnd

private:
	int mHandle;
	void *mDataBuf;
    char mDevicePath[PICO8_BUFFER_SIZE];
    char mManufacturerName[PICO8_BUFFER_SIZE];
    char mDeviceName[PICO8_BUFFER_SIZE];
    char mSerialNumber[PICO8_BUFFER_SIZE];
    char mFirmwareRevision[PICO8_BUFFER_SIZE];

    epicsEventId startEventId_;
    epicsEventId stopEventId_;
    int uniqueId_;
    int acquiring_;
};
#define NUM_PICO8_PARAMS ((int)(&LAST_PICO8_PARAM - &FIRST_PICO8_PARAM + 1))

#endif /* _PICO8_H_ */
