/*
 * AcqHostDevice.cpp
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

#include "AcqHostDescr.h"
#include "FileMonitor.h"

#include "AcqHostDevice.h"

const char* AcqHostDevice::ROOT = "/dev/shm";


AcqHostDevice::AcqHostDevice(const char* _fname)  :
	channel_data_valid(false), thread_name(_fname) {
	fname = new char[strlen(ROOT)+strlen(_fname)+2];
	sprintf(fname, "%s/%s", ROOT, _fname);

	dbg(1, "new device %s", fname);

	runMonitor();
}

AcqHostDevice::~AcqHostDevice() {
	// TODO Auto-generated destructor stub
}


static void monitorAnchorFun(void* dev)
{
	static_cast<AcqHostDevice*>(dev)->monitorFunction();
}

void AcqHostDevice::monitorFunction()
{
	FileMonitor* monitor;
	int errcount = 0;

	while((monitor = FileMonitor::create(fname)) == 0){
		if ((++errcount&0xf) == 1){
			err("waiting for file %s", fname);
		}
		usleep(100000);
	}

	while(1){
		monitor->waitChange();

		onChange();
	}
}


void AcqHostDevice::runMonitor()
{
	monitor = epicsThreadCreate(
				thread_name, 50,
				epicsThreadGetStackSize(epicsThreadStackSmall),
				monitorAnchorFun, (void*)this);
}

void AcqHostDevice::onChange()
{
	dbg(1, "01 %s clients:%d", fname, (int)channels.size());
	for (ChannelMapIterator it = channels.begin();
			it != channels.end(); it++){
		it->second->onScan();
	}
}
