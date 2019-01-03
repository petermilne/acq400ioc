/* ------------------------------------------------------------------------- */
/* sysfs_ds.cpp : SYSFS EPICS device support                                 */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2005 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <epicsStdlib.h>

#include "devLib.h"
#include "dbAccess.h"
#include "dbScan.h"
#include "callback.h"
#include "cvtTable.h"
#include "link.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "drvSup.h"
#include "dbCommon.h"
#include "aiRecord.h"
#include "aoRecord.h"
#include "biRecord.h"
#include "boRecord.h"
#include "epicsExport.h"

/** @file sysfs_ds.cpp top level file for Sysfs Device Support. */



static long acq200Init( void *);
static long acq200Report( void *);
static long acq200InitRecordAi( void * );
static long acq200ReadAi( void * );
static long acq200InitRecordAo( void * );
static long acq200WriteAo( void * );
static long acq200InitRecordBi( void * );
static long acq200ReadBi( void * );
static long acq200InitRecordBo( void * );
static long acq200WriteBo( void * );
static long acq200InitRecordWfAi(void*);
static long acq200ReadWfAi(void*);
static long acq200InitRecordStringin( void * );
static long acq200ReadStringin( void * );
static long acq200InitRecordStringout( void * );
static long acq200WriteStringout( void * );


extern "C" {
/** Device Support Entry Table. */
	struct {
		long number;
		DEVSUPFUN report;
		DEVSUPFUN init;
		DEVSUPFUN init_record;
		DEVSUPFUN get_ioint_info;
		DEVSUPFUN read;
		DEVSUPFUN special_linconv;
	} devSysfsAi = {
		6,
		acq200Report,
		acq200Init,
		acq200InitRecordAi,
		NULL,
		acq200ReadAi,
		NULL
	};
	epicsExportAddress(dset,devSysfsAi);

	struct {
		long number;
		DEVSUPFUN report;
		DEVSUPFUN init;
		DEVSUPFUN init_record;
		DEVSUPFUN get_ioint_info;
		DEVSUPFUN write_ao;
		DEVSUPFUN special_linconv;
	} devSysfsAo = {
		6,
		acq200Report,
		acq200Init,
		acq200InitRecordAo,
		NULL,
		acq200WriteAo,
		NULL
	};
	epicsExportAddress(dset,devSysfsAo);

	struct {
		long		number;
		DEVSUPFUN	report;
		DEVSUPFUN	init;
		DEVSUPFUN	init_record;
		DEVSUPFUN	get_ioint_info;
		DEVSUPFUN	read_bi;
	}devSysfsBi={
		5,
		NULL,
		NULL,
		acq200InitRecordBi,
		NULL,
		acq200ReadBi
	};	
	epicsExportAddress(dset,devSysfsBi);

	struct {
		long		number;
		DEVSUPFUN	report;
		DEVSUPFUN	init;
		DEVSUPFUN	init_record;
		DEVSUPFUN	get_ioint_info;
		DEVSUPFUN	write_bo;
	}devSysfsBo={
		5,
		NULL,
		NULL,
		acq200InitRecordBo,
		NULL,
		acq200WriteBo
	};	
	epicsExportAddress(dset,devSysfsBo);

	struct {
		long		number;
		DEVSUPFUN	report;
		DEVSUPFUN	init;
		DEVSUPFUN	init_record;
		DEVSUPFUN       get_ioint_info;
		DEVSUPFUN	read_wf;
	} devSysfsWfAi={
		5,
		NULL,
		NULL,
		acq200InitRecordWfAi,
		NULL,
		acq200ReadWfAi
	};
	epicsExportAddress(dset,devSysfsWfAi);

	struct {
		long number;
		DEVSUPFUN report;
		DEVSUPFUN init;
		DEVSUPFUN init_record;
		DEVSUPFUN get_ioint_info;
		DEVSUPFUN read_stringin;
	} devSysfsStringin = {
		5,
		acq200Report,
		acq200Init,
		acq200InitRecordStringin,
		NULL,
		acq200ReadStringin
	};
	epicsExportAddress(dset,devSysfsStringin);

	struct {
		long number;
		DEVSUPFUN report;
		DEVSUPFUN init;
		DEVSUPFUN init_record;
		DEVSUPFUN get_ioint_info;
		DEVSUPFUN write_stringout;
	} devSysfsStringout = {
		5,
		acq200Report,
		acq200Init,
		acq200InitRecordStringout,
		NULL,
		acq200WriteStringout,
	};
	epicsExportAddress(dset,devSysfsStringout);
};

static long acq200Init(void* arg)
{
	return 0;
}
static long acq200Report( void* arg )
{
	struct aiRecord * precord = (struct aiRecord*)arg;
	printf("acq200Report( %p )\n", precord);
	return 0;
}




#include "Sysfs.h"


static long acq200InitRecordAi( void* arg )
{
	return SysfsAdapter::create(arg, AI);
}

static long acq200InitRecordAo( void* arg )
{
	return SysfsAdapter::create(arg, AO);
}
static long acq200InitRecordBi( void* arg )
{
	return SysfsAdapter::create(arg, BI);
}
static long acq200InitRecordBo( void* arg )
{
	return SysfsAdapter::create(arg, BO);
}
static long acq200InitRecordWfAi( void * arg )
{
	return SysfsAdapter::create(arg, WFAI);
}
static long acq200InitRecordStringin( void* arg )
{
	return SysfsAdapter::create(arg, stringin);
}
static long acq200InitRecordStringout( void* arg )
{
	return SysfsAdapter::create(arg, stringout);
}


static long acq200ReadAi( void* arg )
/** lookup adaptor and call its read method. */
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, AI);

	if (adapter){
		adapter->read(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}

static long acq200WriteAo(void *arg)
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, AO);

	if (adapter){
		adapter->write(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}

static long acq200ReadBi( void* arg )
/** lookup adaptor and call its read method. */
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, BI);

	if (adapter){
		adapter->read(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}

static long acq200WriteBo(void *arg)
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, BO);

	if (adapter){
		adapter->write(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}

static long acq200ReadWfAi(void *arg)
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, WFAI);

	if (adapter){
		adapter->read(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}

static long acq200ReadStringin( void* arg )
/** lookup adaptor and call its read method. */
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, stringin);

	if (adapter){
		adapter->read(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}
static long acq200WriteStringout( void* arg )
/** lookup adaptor and call its read method. */
{
	SysfsAdapter* adapter = SysfsAdapter::getAdapter(arg, stringout);

	if (adapter){
		adapter->write(arg);
	}else{
		printf("ERROR, failed to get handler\n");
	}
	return 0;
}

const char *sysfs_ds_version = __FILE__ " " __DATE__ " " __TIME__ " 1";

class ID {
public:
		ID() { printf("%s\n", sysfs_ds_version); }
};


ID id;

