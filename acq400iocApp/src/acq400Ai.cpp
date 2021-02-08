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

#include "acq400Ai.h"
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

static const char *driverName="acq400AiAsynPortDriver";


#define SKIP	1		// skip ES


#define NANO		1000000000

void task_runner(void *drvPvt)
{
	acq400Ai *pPvt = (acq400Ai *)drvPvt;

	pPvt->task();
}


acq400Ai::acq400Ai(const char *portName, int _nsam, int _nchan, int _scan_ms):
	asynPortDriver(
		portName,
		_nchan, 														/* maxAddr */
		asynInt32Mask | asynFloat64Mask | asynDrvUserMask | asynEnumMask, 		/* Interface mask */
		asynInt32Mask,   						/* Interrupt mask */
		0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
		1, /* Autoconnect */
		0, /* Default priority */
		0) /* Default stack size*/,
		nsam(_nsam), nchan(_nchan),
		delta_ns(NANO),
		step(_nsam),
		buffer_start_sample(SKIP),mean_of_n0(0), acc(_nchan)
{
	memset(&t0, 0, sizeof(t0));
	memset(&t1, 0, sizeof(t1));

	asynStatus status = asynSuccess;
	createParam(PS_NCHAN,           asynParamInt32,         	&P_NCHAN);
	createParam(PS_NSAM,            asynParamInt32,         	&P_NSAM);
	createParam(PS_SCAN_FREQ,      	asynParamFloat64,         	&P_SCAN_FREQ);
	createParam(PS_FS,		asynParamFloat64,		&P_FS);
	createParam(PS_AI_CH,      	asynParamInt32,         	&P_AI_CH);
	createParam(PS_STEP,      	asynParamInt32,         	&P_STEP);
	createParam(PS_DELTA_NS,      	asynParamInt32,         	&P_DELTA_NS);
	createParam(PS_MEAN_OF_N,      	asynParamInt32,         	&P_MEAN_OF_N);

	setIntegerParam(P_NCHAN, 	nchan);
	setIntegerParam(P_NSAM, 	nsam);

	status = (asynStatus)(epicsThreadCreate("acq400AiTask",
			epicsThreadPriorityHigh,
	                epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)::task_runner,
	                this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", driverName, __FUNCTION__);
	        return;
	}
}

/* set EPICS TS assuming that the FIRST tick is at epoch seconds %0 nsec eg GPS system, others, well, don't really care */


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

asynStatus acq400Ai::updateTimeStamp(epicsTimeStamp& ts)
{
	//asynPortDriver::updateTimeStamp(&ts);
	setTimeStamp(&ts);
	epicsTimeStampAdd(ts, delta_ns);
	return asynSuccess;
}

bool isES(epicsInt32 *raw){
	unsigned *uraw = (unsigned*)raw;

	return uraw[0] == 0xaa55f154 && uraw[1] == 0xaa55f154 && uraw[2] == 0xaa55f154;

}

bool isCH01(epicsInt32 *raw) {
	unsigned *uraw = (unsigned*)raw;
	return (uraw[0]&0x00ff) == 0x20 && (uraw[1]&0x00ff) == 0x21;
}

void acq400Ai::handleBuffer(int ib)
{
	epicsInt32* raw = (epicsInt32*)Buffer::the_buffers[ib]->getBase();
	epicsTimeStamp ts = t0;
	int mean_of_n;
	getIntegerParam(P_MEAN_OF_N, &mean_of_n);
	int stride;
	int shr;

	for ( ; buffer_start_sample >= nsam; buffer_start_sample -= nsam){
		;
	}

	if (mean_of_n == 0){
		shr = 0;
		stride = nsam;
	}else{
		stride = nsam/(1<<mean_of_n) + 1;
		shr = mean_of_n;
	}
	if (mean_of_n != mean_of_n0){
		printf("stride:%d shr:%d\n", stride, shr);
		mean_of_n0 = mean_of_n;
	}

	for ( ; buffer_start_sample < nsam;  buffer_start_sample += step){
		if (buffer_start_sample == 0) buffer_start_sample = 1;

		epicsInt32* cursor = raw + buffer_start_sample*nchan;

		for (int isam = 0; isam < nsam; isam += stride, cursor += stride*nchan){
			while (isES(cursor)){
				cursor += stride*nchan;
				isam += 1;
				printf("WARNING: ib:%02d SKIP ES:%d\n", ib, isam);
			}
			int ff = 0;
			while(ff < nchan && !isCH01(cursor)){
				cursor += 1;
				++ff;
			}
			if (ff){
				printf("WARNING: ib:%02d ff:%d\n", ib, ff);
				if (ff == 64){
					goto hbAbort;
				}
			}
			for (int ic = 0; ic < nchan; ++ic){
				if (ic < 4 && (cursor[ic]&0x00ff) != (0x20+ic)){
					printf("ERROR ib:%02d isam:%d cursor:%08x\n", ib, isam, cursor[ic]);
					goto hbAbort;
				}
				acc.ch[ic] += cursor[ic] >> 8;
				//acc.ch[ic] = cursor[ic] >> 8;
			}
			if (++acc.nadd >= 1<<mean_of_n){
				updateTimeStamp(ts);
				if (ts.nsec == t0.nsec){
					printf("error, ts did not change\n");
				}
				for (int ic = 0; ic < nchan; ++ic){
					setIntegerParam(ic, P_AI_CH, acc.ch[ic]>>shr);
					callParamCallbacks(ic);
				}
				acc.clear();
			}
		}
	}
	return;

hbAbort:
	buffer_start_sample = 1;
	acc.clear();
	return;

}

asynStatus acq400Ai::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName;

	/* Set the parameter in the parameter library. */
	status = (asynStatus) setDoubleParam(function, value);

	/* Fetch the parameter string name for possible use in debugging */
	getParamName(function, &paramName);

	if (function == P_FS || function == P_SCAN_FREQ) {
		double fs = 0;
		double fscan = 0;
		getDoubleParam(P_FS, &fs);
		getDoubleParam(P_SCAN_FREQ, &fscan);
		if (fs == 0 || fscan == 0){
			printf("%s ERROR: fs:%f Fscan:%f\n", __FUNCTION__, fs, fscan);
			status = asynError;
		}else{
			step = fs/fscan;
			//printf("%s DEBUG: fs:%f Fscan:%f step:%u\n", __FUNCTION__, fs, fscan, step);

			if (step < 200){
				step = 200;
			}
			delta_ns = 1e9/fs*step;
			setIntegerParam(P_STEP, step);
			setIntegerParam(P_DELTA_NS, delta_ns);
		}
	}
	return status;
}

void acq400Ai::task()
{
	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);
	for (unsigned ii = 0; ii < Buffer::nbuffers; ++ii){
		Buffer::create(getRoot(0), Buffer::bufferlen);
	}
	int ib;

	while((ib = getBufferId(fc)) >= 0){
		t0 = t1; epicsTimeGetCurrent(&t1);
		if (t0.secPastEpoch == 0){
			continue;
		}
		handleBuffer(ib);
	}
	printf("%s:%s: exit on getBufferId failure\n", driverName, __FUNCTION__);
}

int acq400Ai::factory(const char *portName, int nsam, int nchan, int scan_ms)
{
    	new acq400Ai(portName, nsam, nchan, scan_ms);

    	return asynSuccess;
}



/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
int acq400AiConfigure(const char *portName, int nsam, int nchan, int scan_ms)
{
	return acq400Ai::factory(portName, nsam, nchan, scan_ms);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "nsam",iocshArgInt};
static const iocshArg initArg2 = { "nchan",iocshArgInt};
static const iocshArg initArg3 = { "scan_ms",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0, &initArg1, &initArg2, &initArg3};
static const iocshFuncDef initFuncDef = {"acq400AiConfigure",4,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
	acq400AiConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
}

void acq400aiRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(acq400aiRegister);

}
