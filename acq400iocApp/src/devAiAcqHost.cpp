/* ------------------------------------------------------------------------- */
/* file devAiAcqHost.cpp                                                     */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2011 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>
 *  Created on: Feb 19, 2012
 *      Author: pgm

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

/** @file devAiAcqHost.c DESCR 
 *
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
#include "aiRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "AcqAiHostDescr.h"

#define acq200_debug acq_debug
#include "local.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2

int acq_debug = 0;

/* Create the dset for devAiAcqHost */
static long init_ai(int after);
static long init_record(struct aiRecord *pai);
static long read_ai(struct aiRecord *pai);
static long ai_ioinfo( int cmd, struct aiRecord *pai, IOSCANPVT *ppvt);

struct DevAiAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_ai;
	DEVSUPFUN	special_linconv;
} devAiAcqHost = {
	6,
	NULL,
	(DEVSUPFUN)init_ai,
	(DEVSUPFUN)init_record,
	(DEVSUPFUN)ai_ioinfo,
	(DEVSUPFUN)read_ai,
	NULL
};

epicsExportAddress(dset,devAiAcqHost);

/************************************************************************/
/* Ai Record								*/
/*  INP = "hostname-or-ipaddress:data-number"				*/
/************************************************************************/

/* init_ai for debug */
static long init_ai(int after)
{
	if (getenv("ACQ_DEBUG")){
		acq_debug = atoi(getenv("ACQ_DEBUG"));
	}
#ifdef DEBUG_ON
  printf("devAiAcqHost (init) called, pass=%d\n", after);
#endif
  return(0);
}

static long init_record(struct aiRecord *pai)
{
	int status = 0;
#ifdef DEBUG_ON
  printf("devAiAcqHost (init_record) called\n");
#endif
	switch(pai->inp.type){
	case INST_IO:
		dbg(1, "INP:\"%s\"", pai->inp.value.instio.string);

		pai->dpvt = AcqAiHostDescr::create(pai->inp.value.instio.string);
		if (pai->dpvt == 0){
			status = S_db_badField;
		}
		pai->udf = true;
	}
	return status;
}

static long read_ai(struct aiRecord *pai)
{
	int xx;

	dbg(2, "01");
	if (reinterpret_cast<AcqAiHostDescr*>(pai->dpvt)->getValue(xx)){
		pai->rval = xx;
		pai->udf = false;
		return CONVERT;
	}else{
		return DO_NOT_CONVERT;
	}
}


long ai_ioinfo( int cmd, struct aiRecord *pai, IOSCANPVT *ppvt)
{
	dbg(2, "01");
	AcqAiHostDescr* descr = static_cast<AcqAiHostDescr*>(pai->dpvt);
	return descr->getIoScan(ppvt);
}
