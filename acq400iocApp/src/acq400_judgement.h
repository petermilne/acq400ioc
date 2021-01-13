/*
 * acq400_judgement.h
 *
 *  Created on: 7 Jan 2021
 *      Author: pgm
 */

#ifndef ACQ400IOCAPP_SRC_ACQ400_JUDGEMENT_H_
#define ACQ400IOCAPP_SRC_ACQ400_JUDGEMENT_H_


#include "asynPortDriver.h"

#define PS_NCHAN "NCHAN"							/* asynInt32, 		r/o */
#define PS_NSAM	"NSAM"								/* asynInt32,       r/o */
#define PS_MASK_FROM_DATA "MAKE_MASK_FROM_DATA"		/* asynInt32,       r/w .. MU=y+val, ML=y-val */
#define PS_MU	"MASK_UPPER"						/* asynInt16Array   r/w */
#define PS_ML	"MASK_LOWER"						/* asynInt16Array   r/w */
#define PS_RAW	"RAW"								/* asynInt16Array   r/o */
#define PS_BN	"BUFFER_NUM"						/* asynInt32, 		r/o */

#define PS_RESULT_FAIL "RESULT_FAIL"				/* asynInt32 		r/o */ /* per port P=2 */
#define PS_RESULT_MASK32	"FAIL_MASK32"
#define PS_OK	"OK"								/* asynInt32		r/o */
#define PS_SAMPLE_COUNT	"SAMPLE_COUNT"				/* asynInt32		r/o */
#define PS_SAMPLE_COUNT2	"SAMPLE_COUNT2"				/* asynInt32		r/o */
#define PS_SAMPLE_TIME	"SAMPLE_TIME"				/* asynFloat64		r/o */ /* secs.usecs, synthetic */

#define FIRST_SAM	2

class acq400Judgement: public asynPortDriver {
public:
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	virtual asynStatus readInt8Array(asynUser *pasynUser, epicsInt8 *value,
	                                        size_t nElements, size_t *nIn);
	virtual asynStatus readInt16Array(asynUser *pasynUser, epicsInt16 *value,
	                                        size_t nElements, size_t *nIn);

	virtual asynStatus writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
	                                        size_t nElements);

	static int factory(const char *portName, int maxPoints, int nchan);
	virtual void task();

protected:
	int handle_es(unsigned* raw);
	void handle_burst(int vbn, int offset);
	bool calculate(epicsInt16* raw, const epicsInt16* mu, const epicsInt16* ml);
	/* return TRUE if any fail */

	acq400Judgement(const char* portName, int nchan, int nsam);

	const int nchan;
	const int nsam;
    /** Values used for pasynUser->reason, and indexes into the parameter library. */
    int P_NCHAN;
    int P_NSAM;
    int P_MASK_FROM_DATA;
    int P_MU;
    int P_ML;
    int P_RAW;
    int P_BN;
    int P_RESULT_FAIL;
    int P_OK;
    int P_RESULT_MASK32;
    int P_SAMPLE_COUNT;
    int P_SAMPLE_COUNT2;
    int P_SAMPLE_TIME;

    /* our data */
    epicsInt16* RAW_MU;
    epicsInt16* RAW_ML;
    epicsInt16* CHN_MU;
    epicsInt16* CHN_ML;
    epicsInt16* RAW;
    epicsInt8* RESULT_FAIL;
    epicsInt32* FAIL_MASK32;
    epicsInt32 sample_count;
    epicsInt32 sample_count2;
    epicsFloat64 sample_time;

    int ib;
    bool fill_requested;
    void fill_masks(asynUser *pasynUser, epicsInt16* raw,  int threshold);
    void fill_mask(epicsInt16* mask,  epicsInt16 value);
    void fill_mask_chan(epicsInt16* mask,  int addr, epicsInt16* ch);
};

#endif /* ACQ400IOCAPP_SRC_ACQ400_JUDGEMENT_H_ */