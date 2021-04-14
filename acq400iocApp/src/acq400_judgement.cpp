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
	createParam(PS_MU,                  asynParamInt16Array,    	&P_MU);
	createParam(PS_ML,                  asynParamInt16Array,    	&P_ML);
	createParam(PS_RAW,                 asynParamInt16Array,    	&P_RAW);
	createParam(PS_BN, 		    asynParamInt32, 		&P_BN);
	createParam(PS_RESULT_FAIL,	    asynParamInt32Array,    	&P_RESULT_FAIL);
	createParam(PS_OK,		    asynParamInt32,		&P_OK);

	createParam(PS_RESULT_MASK32,	    asynParamInt32,		&P_RESULT_MASK32);

	createParam(PS_SAMPLE_COUNT,	    asynParamInt32,		&P_SAMPLE_COUNT);
	createParam(PS_CLOCK_COUNT,	    asynParamInt32,		&P_CLOCK_COUNT);
	createParam(PS_SAMPLE_TIME,	    asynParamFloat64,		&P_SAMPLE_TIME);
	createParam(PS_BURST_COUNT, 	    asynParamInt32, 		&P_BURST_COUNT);
	createParam(PS_SAMPLE_DELTA_NS,     asynParamInt32, 		&P_SAMPLE_DELTA_NS);


	setIntegerParam(P_NCHAN, 			nchan);
	setIntegerParam(P_NSAM, 			nsam);
	setIntegerParam(P_MASK_FROM_DATA, 	0);

	RAW_MU = new epicsInt16[nsam*nchan];
	RAW_ML = new epicsInt16[nsam*nchan];
	CHN_MU = new epicsInt16[nchan*nsam];
	CHN_ML = new epicsInt16[nchan*nsam];
	RESULT_FAIL = new epicsInt8[nchan+1];		// index from 1, [0] is update %256
	FAIL_MASK32 = new epicsInt32[nchan/32];
	RAW = 0;   // get va from BUFFER

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



bool acq400Judgement::calculate(epicsInt16* raw, const epicsInt16* mu, const epicsInt16* ml)
{
	memset(RESULT_FAIL+1, 0, sizeof(epicsInt8)*nchan);
	memset(FAIL_MASK32, 0, sizeof(epicsInt32)*nchan/32);
	bool fail = false;

	for (int isam = 0; isam < nsam; ++isam){
		for (int ic = 0; ic < nchan; ++ic){
			int ib = isam*nchan+ic;

			if (isam >= FIRST_SAM){
				epicsInt16 xx = raw[ib];
				if (xx > mu[ib] || xx < ml[ib]){
					FAIL_MASK32[ic/32] |= 1 << (ic&0x1f);
					RESULT_FAIL[ic+1] = 1;
					fail = true;
				}
			}else{
				raw[ib] = 0;		// keep the ES out of the output data..
			}
		}
	}
	return fail;
}

void acq400Judgement::fill_masks(asynUser *pasynUser, epicsInt16* raw,  int threshold)
{
	epicsInt16 uplim = 0x7fff - threshold;
	epicsInt16 lolim = 0x8000 + threshold;

	for (int isam = FIRST_SAM; isam < nsam; ++isam){
		for (int ic = 0; ic < nchan; ++ic){
			int ib = isam*nchan+ic;
			epicsInt16 xx = raw[ib];

			if (isam < 4 && (ic < 4 || (ic > 32 && ic < 36))) asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			              "%s:%s: raw[%d][%d] = %0x4 %s\n",
			              driverName, __FUNCTION__, isam, ic, xx, xx > 32760 || xx < -32760? "RAILED": "");

			if (xx > 32760 || xx < -32760){
				// disable on railed signal (for testing with dummy module
				RAW_MU[ib] = 0x7fff;
				RAW_ML[ib] = 0x8000;
			}else{
				RAW_MU[ib] = xx>uplim? uplim: xx + threshold;
				RAW_ML[ib] = xx<lolim? lolim: xx - threshold;
			}

		}
	}
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
void acq400Judgement::fill_mask(epicsInt16* mask,  epicsInt16 value)
{
	for (int isam = 0; isam < nsam; ++isam){
		for (int ic = 0; ic < nchan; ++ic){
			mask[isam*nchan+ic] = value;
		}
	}
}

void acq400Judgement::fill_mask_chan(epicsInt16* mask,  int addr, epicsInt16* ch)
{
	for (int isam = 0; isam < nsam; ++isam){
		mask[isam*nchan+addr] = ch[isam];
	}
	fill_requested = true;
}

void acq400Judgement::handle_burst(int vbn, int offset)
{
	epicsInt16* raw = (epicsInt16*)Buffer::the_buffers[ib]->getBase()+offset;
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
			for (int ic=0; ic< nchan; ic++){
				doCallbacksInt16Array(&CHN_MU[ic*nsam], nsam, P_MU, ic);
				doCallbacksInt16Array(&CHN_ML[ic*nsam], nsam, P_ML, ic);
			}
			fill_requested = false;
		}
	}
	printf("%s:%s: exit on getBufferId failure\n", driverName, __FUNCTION__);
}

/** Called when asyn clients call pasynInt32->write().
  * This function sends a signal to the simTask thread if the value of P_Run has changed.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus acq400Judgement::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName;
    const char* functionName = "writeInt32";

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);

    if (function == P_MASK_FROM_DATA) {
    	if (value){
    		fill_masks(pasynUser, (epicsInt16*)Buffer::the_buffers[ib]->getBase(), value);
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

asynStatus acq400Judgement::writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
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


int acq400Judgement::factory(const char *portName, int maxPoints, int nchan)
{
    	new acq400Judgement(portName, maxPoints, nchan);

    	return(asynSuccess);
}


/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
int acq400JudgementConfigure(const char *portName, int maxPoints, int nchan)
{
	return acq400Judgement::factory(portName, maxPoints, nchan);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "max points",iocshArgInt};
static const iocshArg initArg2 = { "max chan",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0, &initArg1, &initArg2};
static const iocshFuncDef initFuncDef = {"acq400JudgementConfigure",3,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
	acq400JudgementConfigure(args[0].sval, args[1].ival, args[2].ival);
}

void acq400_judgementRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(acq400_judgementRegister);

}
