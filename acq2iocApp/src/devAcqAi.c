/* ------------------------------------------------------------------------- */
/* devAcqAi.c - device support for ACQ AI device driver                      */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2008 Peter Milne, D-TACQ Solutions Ltd
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

/** @file devAcqAi.c - device support for Linux binary device driver.

 ACQ AI: multichannel binary AI.
	NBITS
	read() - returns nchannels data
	write() - write nbits data.

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
#include	<aiRecord.h>

#include "epicsExport.h"

#define acq200_debug acq_debug
#include "local.h"

int acq_debug = 0;

#include "drvAcqAi.h"

static long init_ai(struct aiRecord *pai);
static long ai_ioinfo( int cmd, struct aiRecord *pai, IOSCANPVT *ppvt);
static long read_ai(struct aiRecord *pai);



typedef struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_write;
	DEVSUPFUN	special_linconv;
	} AIDSET;

AIDSET devAcqAi =     {6, NULL, NULL, init_ai,   ai_ioinfo,   read_ai, NULL};

epicsExportAddress(dset,devAcqAi);


typedef struct AcqDescr {
	struct AcqDev* dev;
	int lchan;
} AcqDescr;
	

static AcqDescr* newAcqDescr(AcqDescr *init_val)
{
	AcqDescr *gbd = (AcqDescr*)malloc(sizeof(AcqDescr));
	assert(gbd);
	*gbd = *init_val;
	return gbd;
}



long init_ai(struct aiRecord *pai)
{
	AcqDescr descr;
	char devname[MAXDEV];
	int status = 0;

	dbg(1, "01");

	switch(pai->inp.type){
	case INST_IO:
		dbg(1, "INP:\"%s\"", pai->inp.value.instio.string);
		if (sscanf(pai->inp.value.instio.string, 
				"%s %d", 
				 devname, &descr.lchan) == 2){

			dbg(1, "dev %s lchan %d", devname, descr.lchan);

			descr.dev = acqCreate(devname);
			pai->dpvt = newAcqDescr(&descr);
			pai->udf = TRUE;
		}else{
			err("bad INP dev want dev ait \"%s\"",
			    pai->inp.value.instio.string);
			status = S_db_badField;
		}
		break;
	default:
		err("illegal INP field %d", pai->inp.type);
		status = S_db_badField;
	}
	return status;
}

long ai_ioinfo( int cmd, struct aiRecord *pai, IOSCANPVT *ppvt)
{
	AcqDescr* descr = (AcqDescr*)pai->dpvt;
	int rc;
	rc = acqGetIoScanpvt(descr->dev, descr->lchan, ppvt);
	return rc;
}

long read_ai(struct aiRecord *pai)
{
	AcqDescr *pdescr = (AcqDescr*)pai->dpvt;
	pai->rval = acqGetAi(pdescr->dev, pdescr->lchan);
	pai->udf = FALSE;
	return CONVERT;
}


