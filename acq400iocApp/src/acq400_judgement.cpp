/*
 * acq400_judgement.cpp : compares incoming waveforms against Mask Upper MU, Mask Lower ML and reports FAIL if outside mask
 *
 *  Created on: 7 Jan 2021
 *      Author: pgm
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include "acq400_judgement.h"
#include <epicsExport.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_ENUM_STRING_SIZE 20

#include "acq-util.h"
#include <vector>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq164AsynPortDriver";

#define NANO		1000000000

void task_runner(void *drvPvt)
{
	acq400Judgement *pPvt = (acq400Judgement *)drvPvt;
	pPvt->task();
}


acq400Judgement::acq400Judgement(const char* portName, int _nchan, int _nsam):
		asynPortDriver(portName,
		                    _nchan, 														/* maxAddr */
		                    asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynEnumMask | asynInt8ArrayMask | asynInt16ArrayMask| asynDrvUserMask, /* Interface mask */
		                    asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynEnumMask | asynInt8ArrayMask | asynInt16ArrayMask,  				 /* Interrupt mask */
		                    0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
		                    1, /* Autoconnect */
		                    0, /* Default priority */
		                    0) /* Default stack size*/,
		nchan(_nchan), nsam(_nsam), sample_delta_ns(0), fill_requested(false)
{
	clock_count[0] = clock_count[1] = 0;
	memset(&t0, 0, sizeof(t0));
	memset(&t1, 0, sizeof(t1));

	asynStatus status = asynSuccess;

	createParam(PS_NCHAN,               asynParamInt32,         	&P_NCHAN);
	createParam(PS_NSAM,                asynParamInt32,         	&P_NSAM);
	createParam(PS_MASK_FROM_DATA,      asynParamInt32,         	&P_MASK_FROM_DATA);

	createParam(PS_BN, 		    asynParamInt32, 		&P_BN);
	createParam(PS_RESULT_FAIL,	    asynParamInt8Array,    	&P_RESULT_FAIL);
	createParam(PS_OK,		    asynParamInt32,		&P_OK);

	createParam(PS_RESULT_MASK32,	    asynParamInt32,		&P_RESULT_MASK32);

	createParam(PS_SAMPLE_COUNT,	    asynParamInt32,		&P_SAMPLE_COUNT);
	createParam(PS_CLOCK_COUNT,	    asynParamInt32,		&P_CLOCK_COUNT);
	createParam(PS_SAMPLE_TIME,	    asynParamFloat64,		&P_SAMPLE_TIME);
	createParam(PS_BURST_COUNT, 	    asynParamInt32, 		&P_BURST_COUNT);
	createParam(PS_SAMPLE_DELTA_NS,     asynParamInt32, 		&P_SAMPLE_DELTA_NS);
	createParam(PS_UPDATE,     	    asynParamInt32, 		&P_UPDATE);


	setIntegerParam(P_NCHAN, 			nchan);
	setIntegerParam(P_NSAM, 			nsam);
	setIntegerParam(P_MASK_FROM_DATA, 	0);


	RESULT_FAIL = new epicsInt8[nchan+1];		// index from 1, [0] is update %256
	FAIL_MASK32 = new epicsInt32[nchan/32];

	WINL = new epicsInt16[nchan];
	WINR = new epicsInt16[nchan];

	for (int ic = 0; ic < nchan; ++ic){
		WINL[ic] = FIRST_SAM;
		WINR[ic] = nchan-1;
	}

    /* Create the thread that computes the waveforms in the background */
    status = (asynStatus)(epicsThreadCreate("acq400JudgementTask",
                          epicsThreadPriorityHigh,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
						  (EPICSTHREADFUNC)::task_runner,
                          this) == NULL);
    if (status) {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, __FUNCTION__);
        return;
    }
}

bool acq400Judgement::onCalculate(bool fail)
{
	epicsInt32 update;

	asynStatus rc = getIntegerParam(P_UPDATE, &update);
	if (rc != asynSuccess){
		return rc;
	}

	switch(update){
	case UPDATE_NEVER:
		return fail;
	}

	for (int ic = 0; ic < nchan; ic++){
		bool cfail = FAIL_MASK32[ic/32] & (1 << (ic&0x1f));
		switch(update){
		case UPDATE_ALWAYS:
			break;
		case UPDATE_ON_FAIL:
			if (!cfail){
				continue;
			}else{
				break;
			}
		case UPDATE_ON_SUCCESS:
			if (cfail){
				continue;
			}else{
				break;
			}
		}

		doDataUpdateCallbacks(ic);
	}
	return fail;
}






static AbstractES& ESX = *AbstractES::evX_instance();


void epicsTimeStampAdd(epicsTimeStamp& ts, unsigned _delta_ns)
{
	if (ts.nsec + _delta_ns < NANO){
		ts.nsec += _delta_ns;
	}else{
		ts.nsec += _delta_ns;
		ts.nsec -= NANO;
		ts.secPastEpoch += 1;
	}
}

/* set EPICS TS assuming that the FIRST tick is at epoch seconds %0 nsec eg GPS system, others, well, don't really care */
asynStatus acq400Judgement::updateTimeStamp(int offset)
{
	asynStatus rc = getIntegerParam(P_SAMPLE_DELTA_NS, &sample_delta_ns);
	if (rc != asynSuccess){
		return rc;
	}else if (sample_delta_ns == 0){
		return asynPortDriver::updateTimeStamp();
	}else{
		epicsTimeStamp ts = t0;
		if (offset == 0){
			clock_count[0] = clock_count[1];
		}else{
			unsigned delta = (clock_count[1] - clock_count[0]) * sample_delta_ns;
			//printf("now.nsec %u -> %u\n", now.nsec, now.nsec+delta);
			epicsTimeStampAdd(ts, delta);
		}
		setTimeStamp(&ts);
		return rc;
	}
}
int acq400Judgement::handle_es(unsigned* raw)
{
	for (int ii = 0; ii < 3; ++ii){
		unsigned* esx = raw + ii*nchan;
		if (ESX.isES(esx)){
			sample_count = esx[4];
			clock_count[1]= esx[5];
			/** @@todo: not sure how to merge EPICS and SAMPLING timestamps.. go pure EPICS */
			++burst_count;
			RESULT_FAIL[0] = burst_count;			// burst_count%256 .. maybe match to exact count and TS */
			return ii;
		}
	}
	return -1;


}



void acq400Judgement::task()
{
	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);
	for (unsigned ii = 0; ii < Buffer::nbuffers; ++ii){
		Buffer::create(getRoot(0), Buffer::bufferlen);
	}


	while((ib = getBufferId(fc)) >= 0){
		t0 = t1; epicsTimeGetCurrent(&t1);
		if (t0.secPastEpoch == 0){
			continue;
		}
		handle_burst(ib*2, 		0);
		handle_burst(ib*2+1, 	nsam*nchan);

		if (fill_requested){
			for (int ic = 0; ic< nchan; ic++){
				doMaskUpdateCallbacks(ic);
			}
			fill_requested = false;
		}
	}
	printf("%s:%s: exit on getBufferId failure\n", driverName, __FUNCTION__);
}



asynStatus acq400Judgement::readInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                        size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    epicsTimeStamp timeStamp;

    getTimeStamp(&timeStamp);
    pasynUser->timestamp = timeStamp;

    printf("%s function:%d\n", __FUNCTION__, function);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d",
                  driverName, __FUNCTION__, status, function);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d\n",
              driverName, __FUNCTION__, function);
    return status;
}

asynStatus acq400Judgement::readInt16Array(asynUser *pasynUser, epicsInt16 *value,
                                        size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    epicsTimeStamp timeStamp;

    getTimeStamp(&timeStamp);
    pasynUser->timestamp = timeStamp;

    printf("%s function:%d\n", __FUNCTION__, function);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d",
                  driverName, __FUNCTION__, status, function);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d\n",
              driverName, __FUNCTION__, function);
    return status;
}




template <class ETYPE>
class acq400JudgementImpl : public acq400Judgement {
	ETYPE* RAW_MU;	/* raw [sample][chan] Mask Upper */
	ETYPE* RAW_ML;	/* raw [sample][chan] Mask Lower */
	ETYPE* CHN_MU;	/* chn [chan][sample] Mask Upper */
	ETYPE* CHN_ML;	/* chn [chan][sample] Mask Lower */
	ETYPE* RAW;

	static const ETYPE MAXVAL;
	static const ETYPE MINVAL;
	static const asynParamType AATYPE;

	void fill_masks(asynUser *pasynUser, ETYPE* raw,  int threshold)
	{
		ETYPE uplim = MAXVAL - threshold;
		ETYPE lolim = MINVAL + threshold;

		for (int isam = FIRST_SAM; isam < nsam; ++isam){
			for (int ic = 0; ic < nchan; ++ic){
				int ib = isam*nchan+ic;
				ETYPE xx = raw[ib];

				if (isam < 4 && (ic < 4 || (ic > 32 && ic < 36))) asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				              "%s:%s: raw[%d][%d] = %0x4 %s\n",
				              driverName, __FUNCTION__, isam, ic, xx, xx > 32760 || xx < -32760? "RAILED": "");

				if (xx > MAXVAL-10 || xx < -MINVAL+10){
					// disable on railed signal (for testing with dummy module
					RAW_MU[ib] = MAXVAL;
					RAW_ML[ib] = MINVAL;
				}else{
					RAW_MU[ib] = xx>uplim? uplim: xx + threshold;
					RAW_ML[ib] = xx<lolim? lolim: xx - threshold;
				}

			}
		}
	}


	void fill_mask(ETYPE* mask,  ETYPE value)
	{
		for (int isam = 0; isam < nsam; ++isam){
			for (int ic = 0; ic < nchan; ++ic){
				mask[isam*nchan+ic] = value;
			}
		}
	}

	void fill_mask_chan(ETYPE* mask,  int addr, ETYPE* ch)
	{
		for (int isam = 0; isam < nsam; ++isam){
			mask[isam*nchan+addr] = ch[isam];
		}
		fill_requested = true;
	}

	void doMaskUpdateCallbacks(int ic){
		assert(0);
	}
	void doDataUpdateCallbacks(int ic){
		assert(0);
	}
public:
	acq400JudgementImpl(const char* portName, int _nchan, int _nsam) :
		acq400Judgement(portName, _nchan, _nsam)
	{
		createParam(PS_MU,                  AATYPE,    	&P_MU);
		createParam(PS_ML,                  AATYPE,    	&P_ML);
		createParam(PS_RAW,                 AATYPE,    	&P_RAW);
		RAW_MU = new ETYPE[nsam*nchan];
		RAW_ML = new ETYPE[nsam*nchan];
		CHN_MU = new ETYPE[nchan*nsam];
		CHN_ML = new ETYPE[nchan*nsam];
		RAW    = new ETYPE[nsam*nchan];
	}


	bool calculate(ETYPE* raw, const ETYPE* mu, const ETYPE* ml)
	{
		memset(RESULT_FAIL+1, 0, sizeof(epicsInt8)*nchan);
		memset(FAIL_MASK32, 0, sizeof(epicsInt32)*nchan/32);
		bool fail = false;

		for (int isam = 0; isam < nsam; ++isam){
			for (int ic = 0; ic < nchan; ++ic){

				if (isam < WINL[ic] || isam > WINR[ic]){
					continue;
				}
				int ib = isam*nchan+ic;

				if (isam >= FIRST_SAM){
					ETYPE xx = raw[ib];
					if (xx > mu[ib] || xx < ml[ib]){
						FAIL_MASK32[ic/32] |= 1 << (ic&0x1f);
						RESULT_FAIL[ic+1] = 1;
						fail = true;
					}
					RAW[ic*nsam+isam] = xx;
				}else{
					raw[ib] = 0;		// keep the ES out of the output data..
				}
			}
		}

		return onCalculate(fail);
	}

	virtual void handle_burst(int vbn, int offset)
	{
		ETYPE* raw = (ETYPE*)Buffer::the_buffers[ib]->getBase()+offset;
		handle_es((unsigned*)raw);

		updateTimeStamp(offset);
		setIntegerParam(P_SAMPLE_COUNT, sample_count);
		setIntegerParam(P_CLOCK_COUNT,  clock_count[1]);
		/** @@todo: not sure how to merge EPICS and SAMPLING timestamps.. go pure EPICS */
		setIntegerParam(P_BURST_COUNT, burst_count);


		bool fail = calculate(raw, RAW_MU, RAW_ML);

		setIntegerParam(P_OK, !fail);
		setIntegerParam(P_BN, vbn);
		for(int m32 = 0; m32 < nchan/32; ++m32){
			setIntegerParam(m32, P_RESULT_MASK32, FAIL_MASK32[m32]);
			callParamCallbacks(m32);
		}

		doCallbacksInt8Array(RESULT_FAIL,   nchan+1, P_RESULT_FAIL, 0);
	}
	/** Called when asyn clients call pasynInt32->write().
	  * This function sends a signal to the simTask thread if the value of P_Run has changed.
	  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
	  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
	  * \param[in] value Value to write. */
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value)
	{
	    int function = pasynUser->reason;
	    int addr;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    const char* functionName = "writeInt32";

	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setIntegerParam(function, value);

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    status = pasynManager->getAddr(pasynUser, &addr);
	    if(status!=asynSuccess) return status;

	    if (function == P_MASK_FROM_DATA) {
	    	if (value){
	    		fill_masks(pasynUser, (ETYPE*)Buffer::the_buffers[ib]->getBase(), (ETYPE)value);
	    	}else{
	    		/* never going to fail these limits */
	    		fill_mask(RAW_MU, 0x7fff);
	    		fill_mask(RAW_ML, 0x8000);
	    	}
		for (int isam = 0; isam < nsam; ++isam){
			for (int ic = 0; ic < nchan; ++ic){
				CHN_MU[ic*nsam+isam] = isam < FIRST_SAM? 0: RAW_MU[isam*nchan+ic];
				CHN_ML[ic*nsam+isam] = isam < FIRST_SAM? 0: RAW_ML[isam*nchan+ic];
			}
		}
	    	fill_requested = true;
	    }else if (function == P_WINL){
		    WINL[addr] = value;
	    }else if (function == P_WINR){
		    WINR[addr] = value;
	    } else {
	        /* All other parameters just get set in parameter list, no need to
	         * act on them here */
	    }

	    /* Do callbacks so higher layers see any changes */
	    status = (asynStatus) callParamCallbacks();

	    if (status)
	        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
	                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
	                  driverName, functionName, status, function, paramName, value);
	    else
	        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	              "%s:%s: function=%d, name=%s, value=%d\n",
	              driverName, functionName, function, paramName, value);
	    return status;
	}
	virtual asynStatus writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
	                                        size_t nElements)
	{
		int function = pasynUser->reason;
		int addr;
		asynStatus status = asynError;

		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
						"%s:%s: function=%d ERROR:%d\n",
						driverName, __FUNCTION__, function, status);
		return asynError;
	}
	virtual asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
	                                        size_t nElements)
	{
		int function = pasynUser->reason;
		int addr;
		asynStatus status = asynError;

		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				"%s:%s: function=%d ERROR:%d\n",
				driverName, __FUNCTION__, function, status);
		return status;
	}
};

template<>
const asynParamType acq400JudgementImpl<epicsInt16>::AATYPE = asynParamInt16Array;
template<>
const asynParamType acq400JudgementImpl<epicsInt32>::AATYPE = asynParamInt32Array;
template<>
const epicsInt16 acq400JudgementImpl<epicsInt16>::MAXVAL = 0x7fff;
template<>
const epicsInt16 acq400JudgementImpl<epicsInt16>::MINVAL = 0x8000;
template<>
const epicsInt32 acq400JudgementImpl<epicsInt32>::MAXVAL = 0x7fffff00;
template<>
const epicsInt32 acq400JudgementImpl<epicsInt32>::MINVAL = 0x80000000;


template<>
asynStatus acq400JudgementImpl<epicsInt16>::writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
		size_t nElements)
{
	int function = pasynUser->reason;
	int addr;
	asynStatus status = asynSuccess;

	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			"%s:%s: function=%d\n",
			driverName, __FUNCTION__, function);

	status = pasynManager->getAddr(pasynUser, &addr);
	if(status!=asynSuccess) return status;

	if (function == P_MU){
		memcpy(&CHN_MU[addr*nsam], value, nsam*sizeof(short));
		fill_mask_chan(RAW_MU, addr, value);
	}else if (function == P_ML){
		memcpy(&CHN_ML[addr*nsam], value, nsam*sizeof(short));
		fill_mask_chan(RAW_ML, addr, value);
	}

	return(status);
}

template<>
asynStatus acq400JudgementImpl<epicsInt32>::writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
		size_t nElements)
{
	int function = pasynUser->reason;
	int addr;
	asynStatus status = asynSuccess;

	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			"%s:%s: function=%d\n",
			driverName, __FUNCTION__, function);

	status = pasynManager->getAddr(pasynUser, &addr);
	if(status!=asynSuccess) return status;

	if (function == P_MU){
		memcpy(&CHN_MU[addr*nsam], value, nsam*sizeof(long));
		fill_mask_chan(RAW_MU, addr, value);
	}else if (function == P_ML){
		memcpy(&CHN_ML[addr*nsam], value, nsam*sizeof(long));
		fill_mask_chan(RAW_ML, addr, value);
	}

	return(status);
}


template<>
void acq400JudgementImpl<epicsInt16>::doDataUpdateCallbacks(int ic)
{
	doCallbacksInt16Array(&RAW[ic*nsam], nsam, P_RAW, ic);
}

template<>
void acq400JudgementImpl<epicsInt32>::doDataUpdateCallbacks(int ic)
{
	doCallbacksInt32Array(&RAW[ic*nsam], nsam, P_RAW, ic);
}

template<>
void acq400JudgementImpl<epicsInt16>::doMaskUpdateCallbacks(int ic){
	doCallbacksInt16Array(&CHN_MU[ic*nsam], nsam, P_MU, ic);
	doCallbacksInt16Array(&CHN_ML[ic*nsam], nsam, P_ML, ic);
}
template<>
void acq400JudgementImpl<epicsInt32>::doMaskUpdateCallbacks(int ic){
	doCallbacksInt32Array(&CHN_MU[ic*nsam], nsam, P_MU, ic);
	doCallbacksInt32Array(&CHN_ML[ic*nsam], nsam, P_ML, ic);
}




int acq400Judgement::factory(const char *portName, int maxPoints, int nchan, unsigned data_size)
{
	switch(data_size){
	case sizeof(short):
		new acq400JudgementImpl<epicsInt16> (portName, maxPoints, nchan);
		return(asynSuccess);
	case sizeof(long):
		new acq400JudgementImpl<epicsInt32> (portName, maxPoints, nchan);
		return(asynSuccess);
	default:
		fprintf(stderr, "ERROR: %s data_size %u NOT supported must be %u or %u\n",
				__FUNCTION__, data_size, (unsigned)sizeof(short), (unsigned)sizeof(long));
		exit(1);
	}



}


/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
int acq400JudgementConfigure(const char *portName, int maxPoints, int nchan, unsigned data_size)
{
	return acq400Judgement::factory(portName, maxPoints, nchan, data_size);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "max points",iocshArgInt};
static const iocshArg initArg2 = { "max chan",iocshArgInt};
static const iocshArg initArg3 = { "data size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0, &initArg1, &initArg2, &initArg3};
static const iocshFuncDef initFuncDef = {"acq400JudgementConfigure",4,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
	acq400JudgementConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
}

void acq400_judgementRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(acq400_judgementRegister);

}
