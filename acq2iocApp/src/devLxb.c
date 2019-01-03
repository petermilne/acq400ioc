/* ------------------------------------------------------------------------- */
/* devLxb.c - device support for Linux BI device driver                      */
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

/** @file devLxb.c - device support for Linux binary device driver.

Linux binary device: device driver for multibit DIO
	NBITS
	read() - returns nbits data. first call returns state,
		subsequent calls return on COS. This can be interrupt or polled
		we don't care, the driver hides that.
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
#include	<biRecord.h>
#include	<mbbiRecord.h>
#include	<mbbiDirectRecord.h>

#include "epicsExport.h"

#define acq200_debug lxb_debug
#include "local.h"

int lxb_debug = 0;

#include "drvLxb.h"

static long init_bi(struct biRecord *pbi);
static long bi_ioinfo( int cmd, struct biRecord *pbi, IOSCANPVT *ppvt);
static long read_bi(struct biRecord *pbi);
static long init_mbbi(struct mbbiRecord *pmbbi);
static long mbbi_ioinfo( int cmd, struct mbbiRecord *pmbbi, IOSCANPVT *ppvt);
static long read_mbbi(struct mbbiRecord  *pmbbi);


typedef struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_write;
	} BINARYDSET;

BINARYDSET devLxb =     {6, NULL, NULL, init_bi,   bi_ioinfo,   read_bi};
BINARYDSET devLxbMbbi = {6, NULL, NULL, init_mbbi, mbbi_ioinfo, read_mbbi};

epicsExportAddress(dset,devLxb);
epicsExportAddress(dset,devLxbMbbi);


typedef struct LxbDescr {
	struct LxbDev* dev;
	int bit;
} LxbDescr;
	

static LxbDescr* newLxbDescr(LxbDescr *init_val)
{
	LxbDescr *gbd = (LxbDescr*)malloc(sizeof(LxbDescr));
	assert(gbd);
	*gbd = *init_val;
	return gbd;
}
long init_bi(struct biRecord *pbi)
{
	LxbDescr descr;
	char devname[MAXDEV];
	int status = 0;

	dbg(1, "01");

	switch(pbi->inp.type){
	case INST_IO:
		dbg(1, "INP:\"%s\"", pbi->inp.value.instio.string);
		if (sscanf(pbi->inp.value.instio.string, 
				"%s %d", 
				 devname, &descr.bit) == 2){

			dbg(1, "dev %s bit %d", devname, descr.bit);

			descr.dev = lxbCreate(devname);
			pbi->dpvt = newLxbDescr(&descr);
			pbi->val= pbi->rval = lxbGetBit(descr.dev, descr.bit);
			pbi->udf = FALSE;
		}else{
			err("bad INP dev want dev bit \"%s\"",
			    pbi->inp.value.instio.string);
			status = S_db_badField;
		}
		break;
	default:
		err("illegal INP field %d", pbi->inp.type);
		status = S_db_badField;
	}
	return status;
}

long bi_ioinfo( int cmd, struct biRecord *pbi, IOSCANPVT *ppvt)
{
	LxbDescr* descr = (LxbDescr*)pbi->dpvt;
	int rc;
	rc = lxbGetIoScanpvt(descr->dev, descr->bit, ppvt);
	return rc;
}

long read_bi(struct biRecord *pbi)
{
	LxbDescr *pdescr = (LxbDescr*)pbi->dpvt;
	pbi->rval = lxbGetBit(pdescr->dev, pdescr->bit);
	return CONVERT;
}

long init_mbbi(struct mbbiRecord *pmbbi)
{
	return 0;
}

long mbbi_ioinfo( int cmd, struct mbbiRecord *pmbbi, IOSCANPVT *ppvt )
{
	return 0;
}

long read_mbbi(struct mbbiRecord  *pmbbi)
{
	return 0;
}

