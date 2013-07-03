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

/** @file devBoAcqHost.cpp Device support entry for Commands
 * 
 *  Created on: May 3, 2012
 *      Author: pgm
 *      BO:
 *      OUT = path to knob, write 0 or 1
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
#include "boRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "AcqBoCommandDescr.h"

#define acq200_debug acq_debug
#include "local.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2


static long init_record_bo(struct boRecord *record);
static long write_bo(struct boRecord *record);

struct DevBoAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_bo;
} devBoAcqCommand={
		5,
		NULL,
		NULL,
		(DEVSUPFUN)init_record_bo,
		NULL,
		(DEVSUPFUN)write_bo
};
epicsExportAddress(dset,devBoAcqCommand);

static long init_record_bo(struct boRecord *record)
{
	int status = 0;
#ifdef DEBUG_ON
  printf("devBiAcqHost (init_record) called\n");
#endif
	switch(record->out.type){
	case INST_IO:
		dbg(1, "OUT:\"%s\"", record->out.value.instio.string);

		record->udf = true;
		record->dpvt = AcqBoCommandDescr::create(record->out.value.instio.string);
		if (record->dpvt == 0){
			status = S_db_badField;
		}

	}
	return status;
}

static long write_bo(struct boRecord *record)
{
	int xx = record->rval;

	dbg(2, "01 %d", xx);
	if (reinterpret_cast<AcqBoCommandDescr*>(record->dpvt)->setValue(xx)){
		record->udf = false;
		return CONVERT;
	}else{
		record->udf = true;
		return DO_NOT_CONVERT;
	}
}
