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
#define PS_BN	"BUFFER_NUM"						/* asynTin32, 		r/o */

#define P_JUDGEMENT_RESULT "JUDGEMENT_RESULT"		/* asynInt8Array[NCHAN] r/o */

class acq400Judgement: public asynPortDriver {
public:
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	static int factory(const char *portName, int maxPoints, int nchan);
	virtual void task();

protected:
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

    /* our data */
    epicsInt16* RAW_MU;
    epicsInt16* RAW_ML;
    epicsInt16* RAW;
};

#endif /* ACQ400IOCAPP_SRC_ACQ400_JUDGEMENT_H_ */
