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


static const char *driverName="acq164AsynPortDriver";


void task_runner(void *drvPvt)
{
	acq400Judgement *pPvt = (acq400Judgement *)drvPvt;

    pPvt->task();
}

acq400Judgement::acq400Judgement(const char* portName, int _nchan, int _nsam):
		asynPortDriver(portName,
		                    _nchan, 														/* maxAddr */
		                    asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynEnumMask | asynInt8ArrayMask | asynDrvUserMask, /* Interface mask */
		                    asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynEnumMask | asynInt8ArrayMask,  				 /* Interrupt mask */
		                    0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
		                    1, /* Autoconnect */
		                    0, /* Default priority */
		                    0) /* Default stack size*/,
		nchan(_nchan), nsam(_nsam)
{
	asynStatus status = asynSuccess;

	createParam(PS_NCHAN,               asynParamInt32,         &P_NCHAN);
	createParam(PS_NSAM,                asynParamInt32,         &P_NSAM);
	createParam(PS_MASK_FROM_DATA,      asynParamInt32,         &P_MASK_FROM_DATA);
	createParam(PS_MU,                  asynParamInt16Array,    &P_MU);
	createParam(PS_ML,                  asynParamInt16Array,    &P_ML);
	createParam(PS_RAW,                 asynParamInt16Array,    &P_RAW);
	createParam(PS_BN, 					asynParamInt32, 		&P_BN);

	setIntegerParam(P_NCHAN, 			nchan);
	setIntegerParam(P_NSAM, 			nsam);
	setIntegerParam(P_MASK_FROM_DATA, 	0);

	RAW_MU = new epicsInt16[nsam*nchan];
	RAW_ML = new epicsInt16[nsam*nchan];
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


int getBufferId(int fc) {
	char buf[80];
	int rc = read(fc, buf, 80);

	if (rc > 0){
		buf[rc] = '\0';
		unsigned ib = strtoul(buf, 0, 10);
		assert(ib >= 0);
//		assert(ib <= Buffer::nbuffers);
		return ib;
	}else{
		return rc<0 ? rc: -1;
	}
}

void acq400Judgement::task()
{
	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);
	int ib;

	while((ib = getBufferId(fc)) >= 0){

		setIntegerParam(P_BN, ib);
		callParamCallbacks();
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
    	/* make mask from data */
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
