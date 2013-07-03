/*
 * AcqHostDescr.cpp
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

using namespace std;
#include <map>

#define acq200_debug acq_debug
#include "local.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "alarm.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "dbScan.h"
#include "drvSup.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "aiRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "epicsThread.h"

#include "AcqHostDevice.h"
#include "AcqAiHostDescr.h"
#include "FileMonitor.h"

#include "AcqHostDescr.h"

AcqHostDescr::AcqHostDescr() {
	scanIoInit(&pvt);
}

AcqHostDescr::~AcqHostDescr() {
	// TODO Auto-generated destructor stub
}

int AcqHostDescr::getIoScan(IOSCANPVT *ppvt)
{
	if (ppvt != 0 && pvt != 0){
		*ppvt = pvt;
		return 0;
	}else{
		return -1;
	}
}
void AcqHostDescr::onScan() {
	dbg(2, "scanIoRequest");
	scanIoRequest(pvt);
}

const char* AcqHostDescr::makeName(const char* defn, char name[], int maxname)
{
	const char* lhs = defn;
	const char* rhs = defn+3; /* next data */

	if (strncmp(lhs, "%g ", 3) == 0){
		snprintf(name, maxname, "/sys/class/gpio/%s/value", rhs);
	}else if (strncmp(lhs, "%l ", 3) == 0){
		snprintf(name, maxname, "/sys/class/leds/%s/brightness", rhs);
	}else if (strncmp(lhs, "%k ", 3) == 0){
		snprintf(name, maxname, "/dev/cpsc.0/%s", rhs);
	}else if (strncmp(lhs, "%p ", 3) == 0){
		char module[32];
		char pram[32];
		if (sscanf(rhs, "%s/%s", module, pram) == 2){
			snprintf(name, maxname,
					"/sys/module/%s/parameters/%s", module, pram);
		}else{
			strcpy(name, "/dev/null");
		}
	}else if (strncmp(lhs, "%s ", 3) == 0){
		snprintf(name, maxname, "/dev/shm/%s", rhs);
	}else{
		strncpy(name, lhs, maxname);
	}
	return name;
}
