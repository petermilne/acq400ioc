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
#include <algorithm>

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

#include "acq-util.h"
#include <vector>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq400JudgementAsynPortDriver";

void task_runner(void *drvPvt)
{
	acq400Judgement *pPvt = (acq400Judgement *)drvPvt;
	pPvt->task();
}

/** abstract base class with Judgement common definitions. Use Judgement::factory() to instantiate a concrete class */
acq400Judgement::acq400Judgement(const char* portName, int _nchan, int _nsam):
	asynPortDriver(portName,
/* maxAddr */		_nchan+1,    /* nchan from 0 + ADDR_WIN_ALL */
/* Interface mask */    asynEnumMask|asynInt32Mask|asynFloat64Mask|asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|asynDrvUserMask,
/* Interrupt mask */	asynEnumMask|asynInt32Mask|asynFloat64Mask|asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask,
/* asynFlags no block*/ 0,
/* Autoconnect */       1,
/* Default priority */  0,
/* Default stack size*/ 0),
	nchan(_nchan), nsam(_nsam),
	fail_mask_len(std::max(nchan/32, 1)),
	sample_delta_ns(0),
	fill_requested(false)
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

	createParam(PS_WINL,                asynParamInt32,              &P_WINL);
	createParam(PS_WINR,                asynParamInt32,              &P_WINR);

	setIntegerParam(P_NCHAN, 			nchan);
	setIntegerParam(P_NSAM, 			nsam);
	setIntegerParam(P_MASK_FROM_DATA, 	0);

	RESULT_FAIL = new epicsInt8[nchan+1];		// index from 1, [0] is update %256
	FAIL_MASK32 = new epicsInt32[fail_mask_len];

	WINL = new epicsInt16[nchan];
	WINR = new epicsInt16[nchan];

	for (int ic = 0; ic < nchan; ++ic){
		WINL[ic] = FIRST_SAM;
		WINR[ic] = nsam-1;
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

int acq400Judgement::verbose = ::getenv_default("acq400JudgementImpl_VERBOSE", 0);

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
		if (verbose > 1){
			printf("%s doDataUpdateCallbacks(%d)\n", __FUNCTION__, ic);
		}
		doDataUpdateCallbacks(ic);
	}
	return fail;
}

#define NANO		1000000000

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

static AbstractES& ESX = *AbstractES::evX_instance();

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

	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}

	epicsTimeGetCurrent(&t0);

	while((ib = getBufferId(fc)) >= 0){
		epicsTimeGetCurrent(&t1);

		handle_burst(ib*2,   0);
		handle_burst(ib*2+1, nsam*nchan);

		if (fill_requested){
			fill_request_task();
			fill_requested = false;
		}
		t0 = t1;
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



/** Concrete Judgement class specialised by data type */
template <class ETYPE>
class acq400JudgementImpl : public acq400Judgement {
	ETYPE* RAW_MU;	/* raw [sample][chan] Mask Upper */
	ETYPE* RAW_ML;	/* raw [sample][chan] Mask Lower */
	ETYPE* CHN_MU;	/* chn [chan][sample] Mask Upper */
	ETYPE* CHN_ML;	/* chn [chan][sample] Mask Lower */
	ETYPE* RAW;

	static const ETYPE MAXVAL;
	static const ETYPE MINVAL;
	static const ETYPE MAXLIM;
	static const ETYPE MINLIM;
	static const asynParamType AATYPE;
	static const int SCALE;

	static int verbose;

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
				              driverName, __FUNCTION__, isam, ic, xx, xx > MAXLIM || xx < -MINLIM? "RAILED": "");

				if (xx > MAXLIM || xx < MINLIM){
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
	void handle_window_limit_change(int p_winx, epicsInt16* winx, int addr, epicsInt32 value)
	{
		if (value < FIRST_SAM) 	value = FIRST_SAM;
		if (value > nsam) 	value = nsam;

		if (addr == ADDR_WIN_ALL){
			for (int ic = 0; ic < nchan; ++ic){
				setIntegerParam(ic, p_winx, winx[ic] = value);
				//callParamCallbacks(p_winx, ic);                 // callParamCallbacks(list=P_ARAM, addr=CH); @@todo BLOWS!
				//callParamCallbacks(ic, p_winx);		  // @@todo REMOVE me .. doesn't BLOW but does nothing0
			}
		}else{
			winx[addr] = value;
		}
	}

	asynStatus write_ETYPE_Array(asynUser *pasynUser, ETYPE *value, size_t nElements)
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
			memcpy(&CHN_MU[addr*nsam], value, nsam*sizeof(ETYPE));
			fill_mask_chan(RAW_MU, addr, value);
		}else if (function == P_ML){
			memcpy(&CHN_ML[addr*nsam], value, nsam*sizeof(ETYPE));
			fill_mask_chan(RAW_ML, addr, value);
		}

		return(status);
	}
	void doMaskUpdateCallbacks(int ic){
		assert(0);
	}
	void doDataUpdateCallbacks(int ic){
		assert(0);
	}
	virtual void fill_request_task() {
		for (int isam = 0; isam < nsam; ++isam){
			for (int ic = 0; ic < nchan; ++ic){
				CHN_MU[ic*nsam+isam] = isam<WINL[ic] || isam>WINR[ic] ? 0: RAW_MU[isam*nchan+ic];
				CHN_ML[ic*nsam+isam] = isam<WINL[ic] || isam>WINR[ic] ? 0: RAW_ML[isam*nchan+ic];
			}
		}

		for (int ic = 0; ic< nchan; ic++){
			doMaskUpdateCallbacks(ic);
		}
	}

public:
	acq400JudgementImpl(const char* portName, int _nchan, int _nsam) :
		acq400Judgement(portName, _nchan, _nsam)
	{
		createParam(PS_MU,  AATYPE,    	&P_MU);
		createParam(PS_ML,  AATYPE,    	&P_ML);
		createParam(PS_RAW, AATYPE,    	&P_RAW);

		RAW_MU = new ETYPE[nsam*nchan];
		RAW_ML = new ETYPE[nsam*nchan];
		CHN_MU = new ETYPE[nchan*nsam];
		CHN_ML = new ETYPE[nchan*nsam];
		RAW    = new ETYPE[nsam*nchan];
	}


	bool calculate(ETYPE* raw, const ETYPE* mu, const ETYPE* ml)
	{
		memset(RESULT_FAIL+1, 0, sizeof(epicsInt8)*nchan);
		memset(FAIL_MASK32, 0, fail_mask_len*sizeof(epicsInt32));
		bool fail = false;

		for (int isam = 0; isam < nsam; ++isam){
			for (int ic = 0; ic < nchan; ++ic){
				int ib = isam*nchan+ic;
				ETYPE xx = isam > FIRST_SAM? raw[ib]: 0;        // keep the ES out of the output data..

				RAW[ic*nsam+isam] = xx;			 	// for plotting

				if (isam >= WINL[ic] && isam <= WINR[ic]){  	// make Judgment inside window
					if (xx > mu[ib] || xx < ml[ib]){
						FAIL_MASK32[ic/32] |= 1 << (ic&0x1f);
						RESULT_FAIL[ic+1] = 1;
						fail = true;
					}
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
		callParamCallbacks();
		if (verbose > 1){
			printf("%s vbn:%3d off:%d fail:%d\n", __FUNCTION__, vbn, offset, fail);
		}
		for(int m32 = 0; m32 < fail_mask_len; ++m32){
			setIntegerParam(m32, P_RESULT_MASK32, FAIL_MASK32[m32]);
			callParamCallbacks(m32);
		}


	}
	/** Called when asyn clients call pasynInt32->write(). */
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value)
	{
	    int function = pasynUser->reason;
	    int addr;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    const char* functionName = "writeInt32";

	    /* Set the parameter in the parameter library. */
	    status = setIntegerParam(function, value);

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    status = pasynManager->getAddr(pasynUser, &addr);
	    if(status!=asynSuccess) return status;

	    if (function == P_MASK_FROM_DATA) {
		    if (value){
			    fill_masks(pasynUser, (ETYPE*)Buffer::the_buffers[ib]->getBase(), (ETYPE)value*SCALE);
		    }else{
			    /* never going to fail these limits */
			    fill_mask(RAW_MU, MAXVAL);
			    fill_mask(RAW_ML, MINVAL);
		    }

		    fill_requested = true;
	    }else if (function == P_WINL){
		    handle_window_limit_change(P_WINL, WINL, addr, value);
		    fill_requested = true;
	    }else if (function == P_WINR){
		    handle_window_limit_change(P_WINR, WINR, addr, value);
		    fill_requested = true;
	    } else {
		    /* All other parameters just get set in parameter list, no need to
		     * act on them here */
	    }
	    status = callParamCallbacks();

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
		asynStatus status = asynError;

		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				"%s:%s: function=%d ERROR:%d\n",
				driverName, __FUNCTION__, function, status);
		return status;
	}
};

template<> const asynParamType acq400JudgementImpl<epicsInt16>::AATYPE = asynParamInt16Array;
template<> const asynParamType acq400JudgementImpl<epicsInt32>::AATYPE = asynParamInt32Array;
template<> const epicsInt16 acq400JudgementImpl<epicsInt16>::MAXVAL = 0x7fff;
template<> const epicsInt16 acq400JudgementImpl<epicsInt16>::MINVAL = 0x8000;
template<> const epicsInt32 acq400JudgementImpl<epicsInt32>::MAXVAL = 0x7fffff00;
template<> const epicsInt32 acq400JudgementImpl<epicsInt32>::MINVAL = 0x80000000;
template<> const epicsInt16 acq400JudgementImpl<epicsInt16>::MAXLIM = 0x7fe0;
template<> const epicsInt16 acq400JudgementImpl<epicsInt16>::MINLIM = 0x8010;
template<> const epicsInt32 acq400JudgementImpl<epicsInt32>::MAXLIM = 0x7ffffef0;
template<> const epicsInt32 acq400JudgementImpl<epicsInt32>::MINLIM = 0x80000010;

template<> const int acq400JudgementImpl<epicsInt16>::SCALE = 1;
template<> const int acq400JudgementImpl<epicsInt32>::SCALE = 256*256;   // scale 32 bit number to 16 bit code step.

template<class T> int acq400JudgementImpl<T>::verbose = ::getenv_default("acq400JudgementImpl_VERBOSE", 0);

template<>
asynStatus acq400JudgementImpl<epicsInt16>::writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
		size_t nElements)
{
	return write_ETYPE_Array(pasynUser, value, nElements);
}

template<>
asynStatus acq400JudgementImpl<epicsInt32>::writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
		size_t nElements)
{
	return write_ETYPE_Array(pasynUser, value, nElements);
}


template<>
void acq400JudgementImpl<epicsInt16>::doDataUpdateCallbacks(int ic)
{
	doCallbacksInt16Array(&RAW[ic*nsam], nsam, P_RAW, ic);
	doCallbacksInt8Array(RESULT_FAIL,   nchan+1, P_RESULT_FAIL, 0);
}

template<>
void acq400JudgementImpl<epicsInt32>::doDataUpdateCallbacks(int ic)
{
	doCallbacksInt32Array(&RAW[ic*nsam], nsam, P_RAW, ic);
	doCallbacksInt8Array(RESULT_FAIL,   nchan+1, P_RESULT_FAIL, 0);
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



/** factory() method: creates concrete class with specialized data type: either epicsInt16 or epicsInt32 */
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
