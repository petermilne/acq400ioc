/* ------------------------------------------------------------------------- */
/* file devWfAWGHost.cpp                                                     */
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
    GNU General Public License for more detwfls.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/* ------------------------------------------------------------------------- */

/** @file devWfAcqHost.c device support for in-memory OUTPUT waveforms
 * RECORD MUST be PP. Then read() actually writes() to file. c'est la vie
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
#include "waveformRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "AcqWfAWGHostDescr.h"

#define acq200_debug wf_awg_debug
#include "local.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2

int wf_awg_debug = 0;

/* Create the dset for devWfAcqHost */
static long init_wf(int after);
static long init_wf_record(struct waveformRecord *pwf);
static long read_wf(struct waveformRecord *pwf);
static long write_wf(struct waveformRecord *pwf);
static long wf_ioinfo( int cmd, struct waveformRecord *pwf, IOSCANPVT *ppvt);

struct DevWfAWgAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_wf;
	DEVSUPFUN	special_linconv;
} devWfAwgAcqHost = {
	5,
	NULL,
	(DEVSUPFUN)init_wf,
	(DEVSUPFUN)init_wf_record,
	(DEVSUPFUN)wf_ioinfo,
	(DEVSUPFUN)write_wf,
	NULL
};

epicsExportAddress(dset,devWfAwgAcqHost);

struct DevWfRawAcqHost{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_wf;
	DEVSUPFUN	special_linconv;
} devWfRawAcqHost = {
	5,
	NULL,
	(DEVSUPFUN)init_wf,
	(DEVSUPFUN)init_wf_record,
	(DEVSUPFUN)wf_ioinfo,
	(DEVSUPFUN)read_wf,
	NULL
};

epicsExportAddress(dset,devWfRawAcqHost);

/************************************************************************/
/* Wf Record								*/
/*  INP = "hostname-or-ipaddress:data-number"				*/
/************************************************************************/

/* init_wf for debug */
static long init_wf(int after)
{
	if (getenv("WF_AWG_DEBUG")){
		wf_awg_debug = atoi(getenv("WF_AWG_DEBUG"));
	}
	dbg(1, "devWfAcqHost.get_ioint_info=%p", devWfAwgAcqHost.get_ioint_info);
	return(0);
}

static long init_wf_record(struct waveformRecord *pwf)
{
	int status = 0;
	dbg(1, "Hello");

	switch(pwf->inp.type){
	case INST_IO:
		dbg(1, "INP:\"%s\" pv_link options: %04x",
				pwf->inp.value.instio.string, pwf->inp.value.pv_link.pvlMask);

		pwf->dpvt = AcqWfAWGHostDescr::create(pwf->inp.value.instio.string);
		if (pwf->dpvt == 0){
			status = S_db_badField;
		}
		/* PP setting doesn't seem to reach this part ..
		if (!(pwf->inp.value.pv_link.pvlMask &pvlOptPP)){
			err("Must be PP for this to work!");
			status = S_db_badField;
		}
		*/
		pwf->udf = true;
	}
	return status;
}

static long write_wf(struct waveformRecord *pwf)
{
	dbg(2, "01 ... calling write() !! opts: %04x",
			pwf->inp.value.pv_link.pvlMask);
	AcqWfAWGHostDescr* descr = static_cast<AcqWfAWGHostDescr*>(pwf->dpvt);
	return descr->write(pwf);
}

static long read_wf(struct waveformRecord *pwf)
{
	AcqWfAWGHostDescr* descr = static_cast<AcqWfAWGHostDescr*>(pwf->dpvt);
	return descr->read(pwf);
}


long wf_ioinfo( int cmd, struct waveformRecord *pwf, IOSCANPVT *ppvt)
{
	dbg(2, "01");
	AcqWfAWGHostDescr* descr = static_cast<AcqWfAWGHostDescr*>(pwf->dpvt);
	return descr->getIoScan(ppvt);
}
