/*
 * AcqWfHostDescr.cpp
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

using namespace std;
#include <map>

#define acq200_debug wf_debug
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
#include "menuFtype.h"
#include "epicsExport.h"
#include "epicsThread.h"
#include "waveformRecord.h"

#include "AcqHostDevice.h"
#include "AcqAiHostDescr.h"

#include "AcqHostDescr.h"
#include "AcqWfHostDescr.h"


class AcqWfHostDevice: public AcqHostDevice {
	static DeviceMap devices;

	AcqWfHostDevice(const char* _fname) :
		AcqHostDevice(_fname) {}

public:
	virtual ~AcqWfHostDevice() {}

	static AcqWfHostDevice* _create(const char* fname){
		DeviceMapIterator found = devices.find(fname);
		if (found != devices.end()){
			delete [] fname;
			return static_cast<AcqWfHostDevice*>(found->second);
		}else{
			AcqWfHostDevice* dev = new AcqWfHostDevice(fname);
			devices[fname] = dev;
			return dev;
		}
	}
	static AcqWfHostDevice* create(const char* _fname) {
		char *fname = new char[strlen(_fname)+1];
		strcpy(fname, _fname);
		return _create(fname);
	}
};

DeviceMap  AcqWfHostDevice::devices;


AcqWfHostDescr::AcqWfHostDescr() : AcqHostDescr() {
	// TODO Auto-generated constructor stub

}

AcqWfHostDescr::~AcqWfHostDescr() {
	// TODO Auto-generated destructor stub
}

#define INP_FMT "%s %d"

class FileClosure {
	FILE *fp;

public:
	FileClosure(const char* fname, const char* mode) {
		fp = fopen(fname, mode);
	}
	~FileClosure() {
		if (fp){
			fclose(fp);
		}
	}
	FILE *getfp() { return fp; }
};

class ConcreteAcqWfHostDescr: public AcqWfHostDescr {
	AcqWfHostDevice* dev;
	char *ch_fname;
	bool error_reported;
public:
	ConcreteAcqWfHostDescr(int cid, AcqWfHostDevice* _dev, char* blockname):
		AcqWfHostDescr(), dev(_dev), error_reported(false)
	{
		int idx = ((cid-1)&(32-1)) + 1;
		char local_fname[128];
		int len = sprintf(local_fname, "%s/%s/CH%02d", \
						AcqHostDevice::ROOT, blockname, idx);

		assert(len < 128);
		ch_fname = new char[len+1];
		strcpy(ch_fname, local_fname);

		dbg(1, "idx:%d ch_fname:%s dirfile:%s", idx, ch_fname, dev->getFname());
	}
	virtual ~ConcreteAcqWfHostDescr() {
		delete [] ch_fname;
	}
	virtual int read(struct waveformRecord* pwf) {
		FileClosure f(ch_fname, "r");


		if (f.getfp()){
			int word_size = sizeof(short);

			switch(pwf->ftvl){
			case menuFtypeSHORT:
			case menuFtypeUSHORT:
				word_size = sizeof(short); break;
			case menuFtypeLONG:
			case menuFtypeULONG:
				word_size = sizeof(long); break;
			case menuFtypeCHAR:
			case menuFtypeUCHAR:
				word_size = 1; break;
			}

			dbg(1, "read \"%s\" %p nelm %d word_size:%d",
						ch_fname, pwf->bptr, pwf->nelm, word_size);

			int nread = fread(pwf->bptr, word_size, pwf->nelm, f.getfp());

			if (wf_debug > 2){
				FILE* fpdump = popen("hexdump", "w");
				fwrite(pwf->bptr, word_size, 8, fpdump);
				pclose(fpdump);
			}
			pwf->nord = nread;
			dbg(1, "read done: %d", nread);
			return 0;
		}else{
			if (!error_reported){
				printf("Failed to open file %s\n", ch_fname);
				error_reported = true;
			}
			return -1;
		}
	}
};
AcqWfHostDescr* AcqWfHostDescr::create(const char* inp)
{
	char dirfile[80];
	char fname[80];
	int cid;

	if (sscanf(inp, INP_FMT, dirfile, &cid) != 2){
		err("sscanf \"%s\" failed in \"%s\"", INP_FMT, inp);
		return 0;
	}
	sprintf(fname, "%s.fin", dirfile);
	AcqWfHostDevice* dev = AcqWfHostDevice::create(fname);
	AcqWfHostDescr* descr = new ConcreteAcqWfHostDescr(cid, dev, dirfile);

	dev->channels[cid] = descr;
	return descr;
}
