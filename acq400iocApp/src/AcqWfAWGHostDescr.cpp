/*
 * AcqWfHostDescr.cpp
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

using namespace std;
#include <map>

#define acq200_debug wf_awg_debug
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
#include "AcqWfAWGHostDescr.h"


AcqWfAWGHostDescr::AcqWfAWGHostDescr() : AcqHostDescr() {
	// TODO Auto-generated constructor stub

}

AcqWfAWGHostDescr::~AcqWfAWGHostDescr() {
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

class ConcreteAcqWfAWGHostDescr: public AcqWfAWGHostDescr {
	char ch_fname[128];

	static int getWordSize(struct waveformRecord* pwf){
		int word_size = sizeof(short);

		switch(pwf->ftvl){
		case menuFtypeLONG:
		case menuFtypeULONG:
			word_size = sizeof(long); break;
		case menuFtypeCHAR:
		case menuFtypeUCHAR:
			word_size = 1; break;
		}
		return word_size;
	}
public:
	ConcreteAcqWfAWGHostDescr(char* fname):
		AcqWfAWGHostDescr()
	{
		strncpy(ch_fname, fname, 127);
	}
	virtual ~ConcreteAcqWfAWGHostDescr() {
		delete [] ch_fname;
	}
	virtual int write(struct waveformRecord* pwf) {
		dbg(1, "01: ch_fname \"%s\"", ch_fname);
		FileClosure f(ch_fname, "w");

		if (f.getfp()){
			int word_size = getWordSize(pwf);

			dbg(1, "write \"%s\" %p nelm %d word_size:%d",
						ch_fname, pwf->bptr, pwf->nelm, word_size);

			int nwrite = fwrite(pwf->bptr, word_size, pwf->nelm, f.getfp());

			if (wf_awg_debug > 2){
				FILE* fpdump = popen("hexdump", "w");
				fwrite(pwf->bptr, word_size, nwrite, fpdump);
				pclose(fpdump);
			}
			return 0;
		}else{
			printf("Failed to open file %s\n", ch_fname);
			return -1;
		}
	}
	virtual int read(struct waveformRecord* pwf) {
		FileClosure f(ch_fname, "r");


		if (f.getfp()){
			int word_size = getWordSize(pwf);

			dbg(1, "read \"%s\" %p nelm %d word_size:%d",
						ch_fname, pwf->bptr, pwf->nelm, word_size);

			int nread = fread(pwf->bptr, word_size, pwf->nelm, f.getfp());

			pwf->nord = nread;
			dbg(1, "read done: %d", nread);
			return 0;
		}else{
			printf("Failed to open file %s\n", ch_fname);
			return -1;
		}
	}
};
AcqWfAWGHostDescr* AcqWfAWGHostDescr::create(const char* inp)
{
	char fname[80];

	if (sscanf(inp, "%s", fname) != 1){
		err("sscanf failed");
	}else{
		dbg(1, "fname set \"%s\"", fname);
	}
	AcqWfAWGHostDescr* descr = new ConcreteAcqWfAWGHostDescr(fname);
	return descr;
}
