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

/** @file devBiAcqHost.cpp Device support entry for BI records
 * 
 *  Created on: May 3, 2012
 *      Author: pgm
 *      BI:
 *      INP = path to knob, write 0 or 1.
 *      specials:
 *      %g KNOB : /sys/class/gpio/KNOB/value
 *      eg %g gpio100
 *      %l KNOB : /sys/class/leds/KNOB/brightness {1|0}
 *      eg %l cpsc:1:green:idle
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
#include "biRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "AcqBiHostDescr.h"

#define acq200_debug acq_debug
#include "local.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2


static long init_record_bi(struct biRecord *record);
static long read_bi(struct biRecord *record);

struct DevBiAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_bi;
} devBiAcqHost={
		5,
		NULL,
		NULL,
		(DEVSUPFUN)init_record_bi,
		NULL,
		(DEVSUPFUN)read_bi
};
epicsExportAddress(dset,devBiAcqHost);

static long init_record_bi(struct biRecord *record)
{
	int status = 0;
#ifdef DEBUG_ON
  printf("devBiAcqHost (init_record) called\n");
#endif
	switch(record->inp.type){
	case INST_IO:
		dbg(1, "INP:\"%s\"", record->inp.value.instio.string);
		AcqHostDescr* descr = AcqBiHostDescr::create(record->inp.value.instio.string);
		record->dpvt = descr;
		if (record->dpvt == 0){
			status = S_db_badField;
		}
		/* @@todo : does not compile ?
		if (record->scan == menuScanI_O_Intr && record->evnt != 0){
			descr->enableWatcher(record->evnt);
		}
		*/
		record->udf = true;
	}

	return status;
}

static long read_bi(struct biRecord *record)
{
	int xx;

	dbg(2, "01");
	if (reinterpret_cast<AcqBiHostDescr*>(record->dpvt)->getValue(xx)){
		record->rval = xx;
		record->udf = false;
		return CONVERT;
	}else{
		return DO_NOT_CONVERT;
	}
}

