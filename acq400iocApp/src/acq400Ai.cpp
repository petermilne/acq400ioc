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

void task_runner(void *drvPvt)
{
	acq400Ai *pPvt = (acq400Ai *)drvPvt;

	pPvt->task();
}


acq400Ai::acq400Ai(const char *portName, int _nsam, int _nchan, int _scan_ms):
	asynPortDriver(
		portName,
		_nchan, 														/* maxAddr */
		asynInt32Mask | asynDrvUserMask, 		/* Interface mask */
		asynInt32Mask,   				/* Interrupt mask */
		0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
		1, /* Autoconnect */
		0, /* Default priority */
		0) /* Default stack size*/,
		nsam(_nsam), nchan(_nchan), scan_ms(_scan_ms)
{
	asynStatus status = asynSuccess;
	createParam(PS_NCHAN,           asynParamInt32,         	&P_NCHAN);
	createParam(PS_NSAM,            asynParamInt32,         	&P_NSAM);
	createParam(PS_SCAN_MSEC,      	asynParamInt32,         	&P_SCAN_MSEC);
	createParam(PS_AI_CH,      	asynParamInt32,         	&P_AI_CH);

	setIntegerParam(P_NCHAN, 	nchan);
	setIntegerParam(P_NSAM, 	nsam);
	setIntegerParam(P_SCAN_MSEC, 	scan_ms);

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

void acq400Ai::outputSampleAt(epicsInt32* raw, int offset)
{
	int ii;
	epicsInt32* cursor = raw+offset;

	for (ii = 0; ii < nchan; ++ii){
		setIntegerParam(ii, P_AI_CH, cursor[ii]>>8);
		callParamCallbacks(ii);
	}
}
void acq400Ai::handleBuffer(int ib)
{
	epicsInt32* raw = (epicsInt32*)Buffer::the_buffers[ib]->getBase() + SKIP*nchan;

	outputSampleAt(raw, 0);
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
		handleBuffer(ib);
	}
	printf("%s:%s: exit on getBufferId failure\n", driverName, __FUNCTION__);
}

int acq400Ai::factory(const char *portName, int nsam, int nchan, int scan_ms)
{
    	new acq400Ai(portName, nsam, nchan, scan_ms);

    	return(asynSuccess);
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
