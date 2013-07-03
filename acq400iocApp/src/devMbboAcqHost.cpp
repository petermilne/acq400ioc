/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2011 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>

    http://www.d-tacq.com

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

/** @file devBoAcqHost.cpp Device support entry for MBBO records
 *
 *  Created on: May 3, 2012
 *      Author: pgm
 *      MBBO:
 *      INP = path to knob, write 0 or 1.
 *      specials:
 *		%k : knobs
 */

//#define DEBUG_ON

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "dbScan.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "mbboRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "AcqMbboHostDescr.h"
#include "AcqMbbiHostDescr.h"
#define acq200_debug acq_debug
#include "local.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2


static long init_record_mbbo(struct mbboRecord *record);
static long write_mbbo(struct mbboRecord *record);

struct DevMBBoAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_mbbo;
} devMbboAcqHost={
		5,
		NULL,
		NULL,
		(DEVSUPFUN)init_record_mbbo,
		NULL,
		(DEVSUPFUN)write_mbbo
};
epicsExportAddress(dset,devMbboAcqHost);

static long init_record_mbbo(struct mbboRecord *record)
{
	int status = 0;
#ifdef DEBUG_ON
  printf("devMbboAcqHost (init_record) called\n");
#endif
	switch(record->out.type){
	case INST_IO:
		dbg(1, "OUT:\"%s\"", record->out.value.instio.string);

		AcqMbbiHostDescr* finds_initial =
				AcqMbbiHostDescr::create(record->out.value.instio.string);
		int init_val;
		if (record->udf && finds_initial->getValue(init_val)){
			record->udf = false;
			record->rval = record->val = init_val;
			dbg(1, "%s:setting init_val:%d\n",
						record->out.value.instio.string, init_val);
		}else{
			record->udf = true;
		}
		delete finds_initial;
		record->dpvt = AcqMbboHostDescr::create(record->out.value.instio.string);
		if (record->dpvt == 0){
			status = S_db_badField;
		}

	}
	return status = 2;	/* don't convert */
}

static long write_mbbo(struct mbboRecord *record)
{
	int xx = record->val;

	dbg(2, "01 val %d", xx);
	if (reinterpret_cast<AcqMbboHostDescr*>(record->dpvt)->setValue(xx)){
		record->udf = false;
		return CONVERT;
	}else{
		return DO_NOT_CONVERT;
	}
}
