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

/** @file devLiAcqHost.cpp DESCR
 * 
 *  Created on: May 3, 2012
 *      Author: pgm
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
#include "longinRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "AcqLiHostDescr.h"

#define acq200_debug acq_debug
#include "local.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2


/* Create the dset for devAiAcqHost */
static long init_li(int after);
static long init_record(struct longinRecord *record);
static long read_li(struct longinRecord *record);

struct DevAiAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_ai;
	DEVSUPFUN	special_linconv;
} devLiAcqHost = {
	6,
	NULL,
	(DEVSUPFUN)init_li,
	(DEVSUPFUN)init_record,
	NULL,
	(DEVSUPFUN)read_li,
};

epicsExportAddress(dset,devLiAcqHost);

/************************************************************************/
/* Ai Record								*/
/*  INP = "hostname-or-ipaddress:data-number"				*/
/************************************************************************/

/* init_ai for debug */
static long init_li(int after)
{
	return(0);
}

static long init_record(struct longinRecord *record)
{
	int status = 0;
#ifdef DEBUG_ON
  printf("devAiAcqHost (init_record) called\n");
#endif
	switch(record->inp.type){
	case INST_IO:
		dbg(1, "INP:\"%s\"", record->inp.value.instio.string);

		record->dpvt = AcqLiHostDescr::create(record->inp.value.instio.string);
		if (record->dpvt == 0){
			status = S_db_badField;
		}
		record->udf = true;
	}
	return status;
}

static long read_li(struct longinRecord *record)
{
	int xx;

	dbg(2, "01");
	if (reinterpret_cast<AcqLiHostDescr*>(record->dpvt)->getValue(xx)){
		record->val = xx;
		record->udf = false;
		return CONVERT;
	}else{
		return DO_NOT_CONVERT;
	}
}

