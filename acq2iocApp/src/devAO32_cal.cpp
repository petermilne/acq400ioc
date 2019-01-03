/* ------------------------------------------------------------------------- */
/* devAO32_cal.cpp - device support AO32CPCI-ER with calibration             */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2009 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>
 *                      http://www.d-tacq.com/

    This program is free software; you can redistribute it and/or modify
    it under the terms of Version 2 of the GNU General Public License 
    as published by the Free Software Foundation;

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/* ------------------------------------------------------------------------- */

/** @file devAO32_cal.cpp -device support AO32CPCI-ER with calibration  .

- provides channelized access to each of AO01..AO32
- uses calibration library to convert Volts to Raw
- Standard EPICS LINEAR, BREAK cal doesn't apply - external routine provides
  4th order polynomial fit.
*/

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	<alarm.h>
#include	<cvtTable.h>
#include	<dbDefs.h>
#include        <devLib.h>
#include	<dbAccess.h>
#include        <recSup.h>
#include	<devSup.h>
#include	<dbScan.h>
#include	<link.h>
#include	<aoRecord.h>

#include "epicsExport.h"

#define ao32_cal_debug acq_debug
#include "local.h"

static int acq_debug = 0;

static long ao32_calReport(void *);
static long ao32_calInit(void *);
static long ao32_calInitRecordAo(void *);
static long ao32_calWriteAo(void *);


extern "C" {
	struct {
		long number;
		DEVSUPFUN report;
		DEVSUPFUN init;
		DEVSUPFUN init_record;
		DEVSUPFUN get_ioint_info;
		DEVSUPFUN write_ao;
		DEVSUPFUN special_linconv;
	} devAO32_cal = {
		6,
		ao32_calReport,
		ao32_calInit,
		ao32_calInitRecordAo,
		NULL,
		ao32_calWriteAo,
		NULL
	};
	epicsExportAddress(dset,devAO32_cal);

	int G_verbose;
	int acq200_debug;
};


#ifdef _ARM_NWFP_
#include "ao32cal_linearity.h"
/* Ao32CalLinearizer lib ONLY available for ARM so far */

struct AO32Dev {
	int slot;
	int channel;
	Ao32CalLinearizer* cal;
};

static long ao32_calInit(void* arg)
{
	long pass = (long)arg;
	printf("ao32_calInit( %ld)\n", pass);
	return 0;
}
static long ao32_calReport( void* arg )
{
	struct aiRecord * precord = (struct aiRecord*)arg;
	printf("ao32_calReport( %p )\n", precord);
	return 0;
}

static long ao32_calInitRecordAo( void* arg )
{
	struct aoRecord* aor = (struct aoRecord*)arg;
	AO32Dev *descr = new AO32Dev;
	int status = 0;

	switch(aor->out.type){
	case INST_IO:
		dbg(1, "OUT:\"%s\"", aor->out.value.instio.string);
		if (sscanf(aor->out.value.instio.string, 
				"%d/%d", 
				 &descr->slot, &descr->channel) == 2){

			dbg(1, "slot %d channel %d", 
				descr->slot, descr->channel);

			descr->cal = Ao32CalLinearizer::create(descr->slot);
			aor->dpvt = descr;
			aor->udf = TRUE;
			aor->linr = 0;
		}else{
			err("bad OUT dev want dev ait \"%s\"",
			    aor->out.value.instio.string);
			status = S_db_badField;
		}
		break;
	default:
		err("illegal OUT field %d", aor->out.type);
		status = S_db_badField;
	}
	if (status != 0){
		delete descr;
	}
	return status;
}

static long ao32_calWriteAo(void *arg)
{
	struct aoRecord* aor = (struct aoRecord*)arg;
	AO32Dev* descr = (AO32Dev*)aor->dpvt;
	
	aor->rval = descr->cal->convert(descr->channel, aor->val);
	descr->cal->writeRaw(descr->channel, aor->rval);
	return 0;
}

#else

static long ao32_calInit(void* arg)
{
	long pass = (long)arg;
	printf("ao32_calInit( %ld)\n", pass);
	return 0;
}
static long ao32_calReport( void* arg )
{
	struct aiRecord * precord = (struct aiRecord*)arg;
	printf("ao32_calReport( %p )\n", precord);
	return 0;
}

static long ao32_calInitRecordAo( void* arg )
{
	return 0;
}

static long ao32_calWriteAo(void *arg)
{
	return 0;
}

#endif // _ARM_NWFP_
