/*
 * pico8.h
 *
 *  Created on: Apr 29, 2016
 *      Author: hinkokocevar
 */

#ifndef _PICO8_H_
#define _PICO8_H_

#include <epicsTypes.h>

#include <ADDriver.h>

/* Number of channels is fixed to 8 */
#define PICO8_NR_CHANNELS			8
/* For strings */
#define PICO8_BUFFER_SIZE			256
/* 1M samples max */
#define PICO8_MAX_SAMPLES			1024*1024

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

class epicsShareClass Pico8 : public ADDriver {
public:
	Pico8(const char *portName, int maxBuffers, size_t maxMemory,
				const char *devicePath, int priority, int stackSize);

    /* These methods override the virtual methods in the base class */
    //void processCallbacks(NDArray *pArray);
    asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);
    virtual void report(FILE *fp, int details);

    // Should be private, but are called from C so must be public
    void dataTask(void);

protected:
    int Pico8Range;
#define FIRST_PICO8_PARAM Pico8Range
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
    epicsEventId mDataEvent;
    unsigned int mAcquiringData;
    unsigned int mFinish;
    char mManufacturerName[PICO8_BUFFER_SIZE];
    char mDeviceName[PICO8_BUFFER_SIZE];
    char mSerialNumber[PICO8_BUFFER_SIZE];
    char mFirmwareRevision[PICO8_BUFFER_SIZE];

};
#define NUM_PICO8_PARAMS ((int)(&LAST_PICO8_PARAM - &FIRST_PICO8_PARAM + 1))

#endif /* _PICO8_H_ */
