/* ------------------------------------------------------------------------- */
/* file AcqAiHostDescr.cpp                                                                 */
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

/** @file AcqAiHostDescr.cpp DESCR 
 *
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

#define INP_FMT "%s %d"

#define MAXCHAN	32
#define MAXCHAN1	(MAXCHAN+1)		/* index from 1 */

#define MAXCHANSZ	(MAXCHAN+1*sizeof(int))



class AcqAiHostDevice: public AcqHostDevice {
	static DeviceMap devices;

	int nchan;
	int *channel_data;

	void getChannelData();

	AcqAiHostDevice(const char* _fname) : AcqHostDevice(_fname) {
		channel_data = new int[nchan = MAXCHAN1];
		getChannelData();
	}

public:
	virtual ~AcqAiHostDevice() {}

	static AcqAiHostDevice* _create(const char* fname){
		DeviceMapIterator found = devices.find(fname);
		if (found != devices.end()){
			delete [] fname;
			return static_cast<AcqAiHostDevice*>(found->second);
		}else{
			AcqAiHostDevice* dev = new AcqAiHostDevice(fname);
			devices[fname] = dev;
			return dev;
		}
	}
	static AcqAiHostDevice* create(const char* _fname) {
		char *fname = new char[strlen(_fname)+1];
		strcpy(fname, _fname);
		return _create(fname);
	}

	bool getValue(int idx, int& value){
		if (channel_data_valid){
			value = channel_data[idx];
			return true;
		}else{
			return false;
		}
	}

	virtual void onChange() {
		getChannelData();
		AcqHostDevice::onChange();
	}

	void reportNchan(int cid){
		if (cid >= nchan){
			delete [] channel_data;
			/* force round up to next 32x so we don't do it too often */
			nchan = (cid/32 + 1)*32;
			channel_data = new int[nchan];
		}
	}
};

void AcqAiHostDevice::getChannelData()
{
	FILE *fp = fopen(fname, "r");
	if (fp == 0){
		perror(fname);
	}else{
		fread(channel_data, sizeof(int), MAXCHAN+1, fp);
		channel_data_valid = true;
		fclose(fp);
	}
}



DeviceMap  AcqAiHostDevice::devices;

AcqAiHostDescr::AcqAiHostDescr() : AcqHostDescr() {
	// TODO Auto-generated constructor stub

}

AcqAiHostDescr::~AcqAiHostDescr() {
	// TODO Auto-generated destructor stub
}

class ConcreteAcqAiHostDescr: public AcqAiHostDescr {
	AcqAiHostDevice* dev;
	const int idx;
public:
	ConcreteAcqAiHostDescr(int cid, AcqAiHostDevice* _dev):
		AcqAiHostDescr(),
		dev(_dev),
		idx(cid)
	{}
	virtual bool getValue(int& xx) {
		dev->getValue(idx, xx);
		dbg(1, "value:%d", xx);
		return true;
	}
};
AcqAiHostDescr* AcqAiHostDescr::create(const char* inp)
/* inp "@fname/cid" */
{
	char fname[80];
	int cid;

	if (sscanf(inp, INP_FMT, fname, &cid) != 2){
		err("sscanf \"%s\" failed in \"%s\"", INP_FMT, inp);
		return 0;
	}
	AcqAiHostDevice* dev = AcqAiHostDevice::create(fname);
	dev->reportNchan(cid);
	AcqAiHostDescr* descr = new ConcreteAcqAiHostDescr(cid, dev);

	dev->channels[cid] = descr;
	return descr;
}
