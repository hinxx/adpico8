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

#define PICO8_BUFFER_SIZE		256
#define PICO8_DATA_SIZE			1024*1024

#define Pico8RangeString              "RANGE"            /* (asynParamInt32, r/w) range */
#define Pico8FSampString              "FSAMP"            /* (asynParamInt32, r/w) sampling frequency */
#define Pico8BTransString             "BTRANS"           /* (asynParamInt32, r/o) no. of transferred bits */
#define Pico8TrgString                "TRG"              /* (asynParamInt32, w/o) trigger source */
#define Pico8RingBufString            "RINGBUF"          /* (asynParamInt32, w/o) pre-trigger sample count */
#define Pico8GateMuxString            "GATEMUX"          /* (asynParamInt32, w/o) gate source */
#define Pico8ConvMuxString            "CONVMUX"          /* (asynParamInt32, w/o) start of conversion source */
#define Pico8DataString               "DATA"             /* (asynParamInt32Array, r/o) data */

class epicsShareClass Pico8 : public ADDriver {
public:
	Pico8(const char *portName,
			int maxBuffers, size_t maxMemory,
			int maxChannels, const char *devicePath,
			int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
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
    int Pico8Trg;
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
