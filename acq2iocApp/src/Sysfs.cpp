/* ------------------------------------------------------------------------- */
/* Sysfs.cpp : SYSFS EPICS - implementatio Sysfs handlers device support     */
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

#define VERID	"Sysfs.cpp Copyright D-TACQ 2007 www.d-tacq.com $Revision: 1.14 $"

/** @file Sysfs.cpp Implementation of the Sysfs Device Support.
 */

#include <stdio.h>
#include <stdlib.h>

#include <epicsStdlib.h>

#include <list>
#include "stringtok.h"


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
#include "stringinRecord.h"
#include "stringoutRecord.h"
#include "waveformRecord.h"
#include "epicsExport.h"
#include "menuFtype.h"

#include "epicsThread.h"
#include "Sysfs.h"

#include <string>
#include <map>
#include <stdlib.h>
#include <iostream>

#define MAXNAME (sizeof((struct aiRecord*)0)->name)

#include <errno.h>

#define acq200_debug sysfs_debug
#include "local.h"
int sysfs_debug = 0;

struct ltstr
{
	bool operator()(const char* s1, const char* s2) const {
		return strcmp(s1, s2) < 0;
	}
};


class Filespec {
	void decode(const char* _filespec);

public:
	Tokenizer<list<string> > args;
	const char* path;
	const char* mode;
	const char* fmt;
	int is_command;

	Filespec(const char* filespec, 
			const char* _mode = "ra", const char* _fmt = "%d") :
		args(filespec),
		path("/dev/null"), mode(_mode), fmt(_fmt), is_command(0) {
		decode(filespec);

		dbg(1, "filespec:%s path:%s", filespec, path);
	}	
};


void Filespec::decode(const char* _filespec) {

	Tokenizer<list<string> > pathspec(args.list().begin()->c_str(), ",");

	int itok = 0;


	for (list<string>::const_iterator i = pathspec.list().begin(); 
	     i != pathspec.list().end(); ++i){
		switch(itok++){
		case 0: 			
			path = i->c_str();
			if (*path == '@'){
				++path;
			}
			if (path[0] == '#' && path[1] == '!'){
				path += 2;
				is_command = 1;
			}
			break;
		case 1:
			mode = i->c_str();
			break;
		case 2:
			fmt = i->c_str();
			break;
		default:
			err("rejecting extra token \"%s\"\n", i->c_str());
		}
	}

/* INP field is limited len (40b)
 * we have some conventions to limit the length:
/sys/module/acq200_mean/parameters/skip:
%m/acq200_mean/%p/skip
*/

	if (strstr(path, "%m")){
		char *mypath = new char[256];

		const char *ps = strstr(path, "%m");
		strcpy(mypath, "/sys/module");
		const char *pp = strstr(path, "%p");
		strncat(mypath, ps+2, pp-(ps+2));
		strcat(mypath, "parameters");
		strcat(mypath, pp+2);

		dbg(1, "  path: %s", path);
		dbg(1, "mypath: %s", mypath);

		path = mypath;
	}
}




/** virtual base class handles record read() and write(). */
class SysfsHandler: public RecordHandler{

protected:
	char* filespec;


	void decode(const char* _filespec);

	SysfsHandler(const char* _filespec) {
		filespec = new char[strlen(_filespec)+1];
		strcpy(filespec, _filespec);
	}

public:
	virtual ~SysfsHandler() {
		delete [] filespec;
	}
	virtual int read(RECORD record) {
		return -1;
	}
	virtual int write(RECORD record) {
		return -1;
	}

	static SysfsHandler* create(const char* filespec);
	static SysfsHandler* createAo(const char* filespec);
	static SysfsHandler* createBx(const char *filespec);
	static SysfsHandler* createWFAI(const char *filespec);
	static SysfsHandler* createStringin(const char *filespec);
	static SysfsHandler* createStringout(const char *filespec);
};



/** Read Only SysfsHandler. */
class SysfsHandlerRO : public SysfsHandler {
protected:
	SysfsHandlerRO(const char* _filespec) :
		SysfsHandler(_filespec){
		
	}
	
public:
	virtual int write(RECORD record){
		return -1;
	}
};

class FileClosure {
protected:
	FILE *fp;

	FileClosure() {

	}
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

class PipeClosure: public FileClosure {

public:
	PipeClosure(const char* command, const char* mode) {
		fp = popen(command, mode);
	}
	~PipeClosure() {
		if (fp){
			pclose(fp);
			fp = 0;
		}
	}
};


/** read only, ascii data. */
class SimpleSysfsReader : public SysfsHandlerRO {
public:
	SimpleSysfsReader(const char* _filespec) :
		SysfsHandlerRO(_filespec) 
	{
		dbg(1, "SimpleSysfsReader() %s", _filespec);
	};
public:
	virtual int read(RECORD _record){
		aiRecord* record = (aiRecord*)_record;
		dbg(1, "SimpleSysfsReader::read() %s", filespec);
		FileClosure f(filespec, "r");

		if (f.getfp()){
			char myline[80];
			fgets(myline, 80, f.getfp());
			record->rval = strtol(myline,0,0);
			dbg(1, "SimpleSysfsReader::read() ans:%s", myline);
			return 0;
		}
		return -1;
	}
};


/** write only, ascii data. */
class SimpleSysfsWriter : public SysfsHandler {
	const char* fmt;
public:
	SimpleSysfsWriter(const char* _filespec, const char* _fmt) :
		SysfsHandler(_filespec) ,
		fmt(_fmt)
	{};
public:
	virtual int write(RECORD _record){
		aoRecord* record = (aoRecord*)_record;
		FileClosure f(filespec, "w");

		if (f.getfp()){
			fprintf(f.getfp(), fmt, record->rval);
			return 0;
		}
		return -1;
	}
};

/** reads binary data, T gives the size */
template <class T>
class BinarySysfsReader : public SysfsHandlerRO 
{
public:
	BinarySysfsReader(const char* _filespec) :
		SysfsHandlerRO(_filespec) 
	{};
		
public:		
	virtual int read(RECORD _record){
		aiRecord* record = (aiRecord*)_record;
	        FileClosure f(filespec, "r");
		T data;


		if (f.getfp()){
			fread(&data, sizeof(T), 1, f.getfp());
			record->rval = data;
			return 0;
		}

		return -1;
	}
};


template <class T>
class BinaryWaveformReader : public SysfsHandlerRO 
{
	int validate(waveformRecord* record);
public:
	BinaryWaveformReader(const char* _filespec) :
		SysfsHandlerRO(_filespec)
	{}
	
	virtual int read(RECORD _record) {
		waveformRecord* record = (waveformRecord*)_record;

		if (!validate(record)){
			printf("Validation Error\n");
			return -1;

		}
		FileClosure f(filespec, "r");
		
		if (f.getfp()){
			int nread = fread(record->bptr, sizeof(T),
					  record->nelm, f.getfp());
		
			record->nord = nread;
			return 0;
		}else{
			printf("Failed to open file\n");
			return -1;
		}
	}
};

template<>
int BinaryWaveformReader<short>::validate(waveformRecord* record)
{
	return record->ftvl == menuFtypeSHORT;
}
template<>
int BinaryWaveformReader<long>::validate(waveformRecord* record)
{
	return record->ftvl == menuFtypeLONG;
}
template<>
int BinaryWaveformReader<class T>::validate(waveformRecord* record)
{
	return 0;
}

/** command: string with executable + args; we return the output (how?). */
class CommandAscii : public SysfsHandler 
{
	const char* format;

	char buf[80];
public:
	CommandAscii(const char* _filespec, const char* _format) :
		SysfsHandler(_filespec),
		format(_format)
		{}

	
protected:
	virtual int exec(RECORD _record){
		//aiRecord* record = (aiRecord *)_record;
		/** does WRITING to an aiRecord make sense?? */
		PipeClosure pc(filespec, "r");

		if (pc.getfp()){
			fgets(buf, 80, pc.getfp());
			return 0;
		}else{
			err("command \"%s\" output \"%s\"\n", filespec, buf);
			return -1;
		}
	}	
public:
	virtual int read(RECORD _record){
		aiRecord* record = (aiRecord *)_record;
		int rc = exec(record);
		if (rc == 0){
			if (sscanf(buf, format, &record->rval) == 1){
				return 0;
			}else{
				return -1;
			}
		}else{
			return rc;
		}
	}
	virtual int write(RECORD _record){
		aiRecord* record = (aiRecord *)_record;
		return exec(record);
	}
};


class CommandAsciiString : public SysfsHandler
{
	const char* format;

	char buf[80];
public:
	CommandAsciiString(const char* _filespec, const char* _format) :
		SysfsHandler(_filespec),
		format(_format)
		{}


protected:
	virtual int exec(RECORD _record){
		//aiRecord* record = (aiRecord *)_record;
		/** does WRITING to an aiRecord make sense?? */
		PipeClosure pc(filespec, "r");

		if (pc.getfp()){
			fgets(buf, 80, pc.getfp());
			return 0;
		}else{
			err("command \"%s\" output \"%s\"\n", filespec, buf);
			return -1;
		}
	}
public:
	virtual int read(RECORD _record){
		stringinRecord* record = (stringinRecord *)_record;
		int rc = exec(record);
		dbg(1, "read: %d \"%s\"\n", rc, buf);
		if (rc == 0){
			if (sscanf(buf, "%40s", record->val) == 1){
				return 0;
			}else{
				return -1;
			}
		}else{
			return rc;
		}
	}
	virtual int write(RECORD _record){
		PipeClosure pc(filespec, "w");
		fputs(static_cast<stringoutRecord*>(_record)->val, pc.getfp());
		return -1;
	}
};

class SimpleSysfsStringHandler : public SysfsHandler
{
	const char* format;

	char buf[80];
public:
	SimpleSysfsStringHandler(const char* _filespec, const char* _fmt = "%40s") :
		SysfsHandler(_filespec),
		format(_fmt)
		{}


protected:
	virtual int exec(RECORD _record){
		//aiRecord* record = (aiRecord *)_record;
		/** does WRITING to an aiRecord make sense?? */
		FileClosure fc(filespec, "r");

		if (fc.getfp()){
			fgets(buf, 80, fc.getfp());
			return 0;
		}else{
			err("command \"%s\" output \"%s\"\n", filespec, buf);
			return -1;
		}
	}
public:
	virtual int read(RECORD _record){
		stringinRecord* record = (stringinRecord *)_record;
		int rc = exec(record);
		if (rc == 0){
			int ic = 0;

			for (; ic < 39; ++ic){
				char xx = buf[ic];
				switch(xx){
				case '\0':
				case '\n':
				case '\r':
					goto strcpy_done;
				default:
					record->val[ic] = xx;
				}
			}

	strcpy_done:
			record->val[ic] = '\0';
			return 0;
		}else{
			return rc;
		}
	}
	virtual int write(RECORD _record){
		stringoutRecord* record = (stringoutRecord *)_record;
		FileClosure f(filespec, "w");

		if (f.getfp()){
			fprintf(f.getfp(), format, record->val);
			return 0;
		}
		return -1;
	}
};
/** Bi, Bo : D-TACQ specialised class */

#define MAXBITS	32

class BxSysfsHandler : public SysfsHandler {
	char* bits;
	int ibit;
public:
	BxSysfsHandler(const char* _filespec, int _ibit) :
		SysfsHandler(_filespec), ibit(_ibit)
	{
		bits = new char[MAXBITS+1];
		for (int ibit = 0; ibit < MAXBITS; ++ibit){
			bits[ibit] = 'x';
		}
	}
public:
	virtual int read(RECORD _record){
		biRecord* record = (biRecord *)_record;
		FileClosure f(filespec, "r");
		
		if (!f.getfp()){
			return -1;
		}
		char * rc = fgets(bits, MAXBITS+1, f.getfp());

		dbg(2, "ibit%d: bits: \"%s\" x:%c", ibit, bits, bits[ibit]);

		if (rc != 0){
			bits[MAXBITS] = '\0';

			switch(bits[ibit]){
			case 'H':
			case '1':
				record->rval = 1;
				return 0;
			case 'L':
			case '0':
				record->rval = 0;
				return 0;
			default:
				return -1;
			}
		}else{
			return -1;
		}		
	}
	virtual int write(RECORD _record){
		boRecord* record = (boRecord *)_record;
		FileClosure f(filespec, "w");

		if (!f.getfp()){
			return -1;
		}
		bits[ibit] = record->rval? '1': '0';

		dbg(2, "ibit%d: bits \"%s\" f:%s", ibit, bits, filespec);

		fputs(bits, f.getfp());
		return 0;
	}
};

/** SysfsHandler factory function, parse _filespec to determine handler type.*/
SysfsHandler* SysfsHandler::create(const char* _filespec) 
{
	Filespec spec(_filespec);

	if (spec.is_command){
		/** construct command from path + original args */
		StringBuffer<128> sb(strlen(_filespec));
		strcpy(sb.getBuf(), spec.path);
		strcat(sb.getBuf(), " ");
		list<string>::const_iterator i = spec.args.list().begin();
		strcat(sb.getBuf(), strstr(_filespec, (++i)->c_str()));

		/** @todo assume these commands are all ascii */
		return new CommandAscii(sb.getBuf(), spec.fmt);
	}else{
		if (strcmp(spec.mode, "rb2") == 0){
			return new BinarySysfsReader<short>(spec.path);
		}else if (strcmp(spec.mode, "rb4") == 0){
			return new BinarySysfsReader<long>(spec.path);
		}else{
			dbg(1, " create SimpleSysfsReader %s %s", _filespec);
		        if (_filespec[0] == '@'){
		        	++_filespec;
		        }
			return new SimpleSysfsReader(_filespec);
		}
	}
}


SysfsHandler* SysfsHandler::createStringin(const char* _filespec)
{
	Filespec spec(_filespec);

	if (spec.is_command){
		/** construct command from path + original args */
		StringBuffer<128> sb(strlen(_filespec));
		strcpy(sb.getBuf(), spec.path);
		strcat(sb.getBuf(), " ");
		list<string>::const_iterator i = spec.args.list().begin();
		strcat(sb.getBuf(), strstr(_filespec, (++i)->c_str()));

		/** @todo assume these commands are all ascii */
		return new CommandAsciiString(sb.getBuf(), spec.fmt);
	}else{
		return new SimpleSysfsStringHandler(spec.path);
	}
}

SysfsHandler* SysfsHandler::createStringout(const char* _filespec)
/* This is all BOGUS, but live with it for now .. */
{
	Filespec spec(_filespec);

	if (spec.is_command){
		/** construct command from path + original args */
		StringBuffer<128> sb(strlen(_filespec));
		strcpy(sb.getBuf(), spec.path);
		strcat(sb.getBuf(), " ");
		list<string>::const_iterator i = spec.args.list().begin();
		strcat(sb.getBuf(), strstr(_filespec, (++i)->c_str()));

		/** @todo assume these commands are all ascii */
		return new CommandAsciiString(sb.getBuf(), spec.fmt);
	}else{
		return new SimpleSysfsStringHandler(spec.path, "%s");
	}
}
SysfsHandler* SysfsHandler::createAo(const char* _filespec) {
	Filespec spec(_filespec);

	return new SimpleSysfsWriter(spec.path, spec.fmt);
}

SysfsHandler* SysfsHandler::createBx(const char* _filespec) {
	Filespec spec(_filespec, "0");

	return new BxSysfsHandler(spec.path, atoi(spec.mode));
}


SysfsHandler* SysfsHandler::createWFAI(const char* _filespec) {
	Filespec spec(_filespec);

	dbg(2, "filespec:%s", _filespec);
	
	if (strcmp(spec.mode, "rb2") == 0){
		return new BinaryWaveformReader<short>(spec.path);
	}else if (strcmp(spec.mode, "rb4") == 0){
		return new BinaryWaveformReader<long>(spec.path);
	}else{
		printf("ERROR: bad mode \"%s\" in WFAI must be rb2 or rb4\n",
				spec.mode);
		return 0;
	}
}

/** Adapter Implementation. */
class SysfsAdapterImpl : public SysfsAdapter 
{
	SysfsHandler* handler;
	RTYPE rtype;
	
protected:
	/** objects created via SysfsAdapter::registerRecord(). */
	SysfsAdapterImpl(const char* _name, RTYPE _rtype) : 
		SysfsAdapter(_name), handler(0), rtype(_rtype)
	{
	}
protected:
/** lookup table of adapters, indexed by record name. */
	static std::map<const char*,SysfsAdapterImpl*,ltstr> adapters;

	static int _dbLookup(const char *record_name, 
			const char* field_name, char value[], int vlen);
	static SysfsAdapter* getAdapter(const char* rname, const char* fname);
public:

	int hasHandler(void) { return handler != 0; }

	/** assign the handler. 
	 *  Handler definition is NOT KNOWN when adapter is created. */
	void setHandler(SysfsHandler* _handler) { handler = _handler; }
		
	virtual int read(void* record) {
		return handler->read(record);
	}
	virtual int write(void* record) {		
		return handler->write(record);
	}
	static int create(void* record, RTYPE rtype);

	virtual int signon() {
		adapters[name] = this;	
		return 0;
	}
	virtual int createHandler(const char* hspec) = 0;

	
	static SysfsAdapter* getAdapter(void *record, enum RTYPE rtype);
};

std::map<const char*,SysfsAdapterImpl*,ltstr> SysfsAdapterImpl::adapters;

int SysfsAdapterImpl::_dbLookup(const char *record_name, 
			const char* field_name, char value[], int vlen)
/** duplicates dbgf(), get the value from the INP field, Blatant crib from dbTest.c. */
{
	char name[80];
	long status;
	DBADDR addr;
	long nelems;
	long options = 0;

	snprintf(name, 80, "%s.%s", record_name, field_name);

	status = dbNameToAddr(name, &addr);
	if (status) {
		char em[80];
		sprintf(em, " dbNameToAddr(%s) failed", name);
		//errMessage(status, em);
		return(status);
	}
	nelems = MIN(addr.no_elements, vlen);

	if (addr.dbr_field_type == DBR_STRING){
		status=dbGetField(&addr, addr.dbr_field_type, 
				  value, &options, &nelems, NULL);
		return status;	
	}else{
		//errMessage(status, " field type NOT DBR_STRING");
		return -1;
	}
}

SysfsAdapter* SysfsAdapterImpl::getAdapter(const char* rname, const char* fname)
{
	char filespec[MAXNAME];
	int rc = _dbLookup(rname, fname, filespec, MAXNAME);		
	if (rc != 0){
		return 0;
	}else{
		SysfsAdapterImpl* adapter = SysfsAdapterImpl::adapters[rname];
		if (!adapter->hasHandler()){
			adapter->createHandler(filespec);
		}
		return adapter;
	}
}

class SysfsAdapterAI : public SysfsAdapterImpl {
	aiRecord *record;

public:
	SysfsAdapterAI(void* _record) :
		SysfsAdapterImpl(((aiRecord*)_record)->name, AI),
		record((aiRecord*)_record)	{
	}

public:
	virtual int signon() {
		dbg(1, "%s type %d", __func__, record->inp.type);
		dbg(1, "SysfsAdapterAI::signon() type %d", record->inp.type);
	
		switch (record->inp.type) {
		case INST_IO:
			record->udf = FALSE;
#if 0
			/* Make sure record processing does not perform any conversion*/
			record->linr = 0;
#endif
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record, 
					  " Illegal INP field");
			return S_db_badField;
		}
	}

	virtual int createHandler(const char* hspec) {
		dbg(1, "%s type hspec %s", __func__, hspec);
		setHandler(SysfsHandler::create(hspec));
		return 0;
	}
	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((aiRecord*)_record)->name, "INP");
	}
};


class SysfsAdapterStringin : public SysfsAdapterImpl {
	stringinRecord *record;

public:
	SysfsAdapterStringin(void* _record) :
		SysfsAdapterImpl(((stringinRecord*)_record)->name, stringin),
		record((stringinRecord*)_record)	{
	}

public:
	virtual int signon() {
		dbg(1, "type %d", record->inp.type);

		switch (record->inp.type) {
		case INST_IO:
			record->udf = FALSE;
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record,
					  " Illegal INP field");
			return S_db_badField;
		}
	}

	virtual int createHandler(const char* hspec) {
		setHandler(SysfsHandler::createStringin(hspec));
		return 0;
	}

	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((stringinRecord*)_record)->name, "INP");
	}
};

class SysfsAdapterStringout : public SysfsAdapterImpl {
	stringoutRecord *record;

public:
	SysfsAdapterStringout(void* _record) :
		SysfsAdapterImpl(((stringoutRecord*)_record)->name, stringout),
		record((stringoutRecord*)_record)	{
	}

public:
	virtual int signon() {
		dbg(1, "type %d", record->out.type);

		switch (record->out.type) {
		case INST_IO:
			record->udf = FALSE;
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record,
					  " Illegal INP field");
			return S_db_badField;
		}
	}

	virtual int createHandler(const char* hspec) {
		setHandler(SysfsHandler::createStringout(hspec));
		return 0;
	}
	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((stringoutRecord*)_record)->name, "OUT");
	}
};
class SysfsAdapterAO : public SysfsAdapterImpl {
	aoRecord *record;

public:
	SysfsAdapterAO(void* _record) :
		SysfsAdapterImpl(((aoRecord*)_record)->name, AI),
		record((aoRecord*)_record)	{
	}

public:
	virtual int signon() {
		dbg(1, "type %d", record->out.type);
	
		switch (record->out.type) {
		case INST_IO:
			record->udf = FALSE;
#if 0
			/* Make sure record processing does not perform any conversion*/
			record->linr = 0;
#endif
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record, 
					  " Illegal OUT field");
			return S_db_badField;
		}
	}

	virtual int createHandler(const char* hspec) {
		setHandler(SysfsHandler::createAo(hspec));
		return 0;
	}
	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((aoRecord*)_record)->name, "OUT");		
	}
};


class SysfsAdapterBI : public SysfsAdapterImpl {
	biRecord *record;

public:
	SysfsAdapterBI(void* _record) :
		SysfsAdapterImpl(((biRecord*)_record)->name, BI),
		record((biRecord*)_record)	{
	}

public:
	virtual int signon() {
		dbg(1, "type %d", record->inp.type);
	
		switch (record->inp.type) {
		case INST_IO:
			record->udf = FALSE;
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record, 
					  " Illegal OUT field");
			return S_db_badField;
		}
	}

	virtual int createHandler(const char* hspec) {
		setHandler(SysfsHandler::createBx(hspec));
		return 0;
	}
	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((biRecord*)_record)->name, "INP");
	}
};

class SysfsAdapterBO : public SysfsAdapterImpl {
	boRecord *record;

public:
	SysfsAdapterBO(void* _record) :
		SysfsAdapterImpl(((boRecord*)_record)->name, BO),
		record((boRecord*)_record)	{
	}

public:
	virtual int signon() {
		dbg(1, "type %d", record->out.type);
	
		switch (record->out.type) {
		case INST_IO:
			record->udf = FALSE;
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record, 
					  " Illegal OUT field");
			return S_db_badField;
		}
	}

	virtual int createHandler(const char* hspec) {
		setHandler(SysfsHandler::createBx(hspec));
		return 0;
	}
	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((boRecord*)_record)->name, "OUT");
	}
};


class SysfsAdapterWFAI : public SysfsAdapterImpl {
	waveformRecord *record;

	static epicsThreadId stateWatcher;
	static void stateWatcherAction(void *parm);
	static void startWatcher();
	static int last_state;

public:
	SysfsAdapterWFAI(void* _record) :
	SysfsAdapterImpl(((waveformRecord*)_record)->name, WFAI),
		record((waveformRecord*)_record) {
			if (stateWatcher == 0){
				startWatcher();
			}
		}

	virtual int signon() {
		switch(record->inp.type){
		case INST_IO:
			record->udf = FALSE;
			return SysfsAdapterImpl::signon();
		default:
			recGblRecordError(S_db_badField, record, 
					  " Illegal INP field");
			return S_db_badField;
		}
	}
	virtual int createHandler(const char* hspec){
		setHandler(SysfsHandler::createWFAI(hspec));
		return 0;
	}
	static SysfsAdapter* dbLookup(void *_record) {
		return getAdapter(((waveformRecord*)_record)->name, "INP");
	}


};

#define ACQSTATE	"/dev/acq200/tblocks/acqstate"
#define MAXLINE		80

void SysfsAdapterWFAI::stateWatcherAction(void *parm) {
	FILE *fp = fopen(ACQSTATE, "r");

	if (fp == 0){
		printf("ERROR: failed to open " ACQSTATE "\n");
		exit(errno);
	}

// 69283.26 2 ST_RUN	

	char buf[MAXLINE];
	int secs, decsecs, state;
	char sname[32];

	while(fgets(buf, MAXLINE, fp)){
		if (sscanf(buf, "%d.%d %d %s", 
			   &secs, &decsecs, &state, sname) == 4){

			dbg(1, "event %d posted\n", last_state*10 + state);

			post_event(last_state*10 + state);
			post_event(11);	/* generic state change evt */
			last_state = state;	
		}
	}

	printf("ERROR:fgets() failed\n");
	exit(errno);
}

void SysfsAdapterWFAI::startWatcher() {
	unsigned int stackSize = 
		epicsThreadGetStackSize(epicsThreadStackSmall);	

	stateWatcher = epicsThreadCreate(
		"swatch", 50, stackSize, stateWatcherAction, 0);
}

epicsThreadId SysfsAdapterWFAI::stateWatcher;
int SysfsAdapterWFAI::last_state;




int SysfsAdapterImpl::create(void* record, enum RTYPE rtype) {
	SysfsAdapterImpl* adapter;
	switch(rtype){
	case AI:
		adapter = new SysfsAdapterAI(record);	break;
	case AO:
		adapter = new SysfsAdapterAO(record); break;
	case BI:
		adapter = new SysfsAdapterBI(record); break;
	case BO:
		adapter = new SysfsAdapterBO(record); break;
	case WFAI:
		adapter = new SysfsAdapterWFAI(record); break;
	case stringin:
		adapter = new SysfsAdapterStringin(record); break;
	case stringout:
		adapter = new SysfsAdapterStringout(record); break;
	default:
		return -1;
	}

	int rc = adapter->signon();
	if (rc != 0){
		delete adapter;
	}
	return rc;
}


SysfsAdapter* SysfsAdapterImpl::getAdapter(void *record, enum RTYPE rtype) {
	switch(rtype){
	case AI:
		return SysfsAdapterAI::dbLookup(record);
	case AO:
		return SysfsAdapterAO::dbLookup(record);
	case BI:
		return SysfsAdapterBI::dbLookup(record);
	case BO:
		return SysfsAdapterBO::dbLookup(record);
	case WFAI:
		return SysfsAdapterWFAI::dbLookup(record);
	case stringin:
		return SysfsAdapterStringin::dbLookup(record);
	case stringout:
		return SysfsAdapterStringout::dbLookup(record);
	default:
		return 0;
	}
}




/** create an Adapter for record and store in lookup table. */
int SysfsAdapter::create(void* record, RTYPE rtype) 
{
	return SysfsAdapterImpl::create(record, rtype);
}

SysfsAdapter* SysfsAdapter::getAdapter(void* record, RTYPE rtype)
{
	return SysfsAdapterImpl::getAdapter(record, rtype);
}

class Ident {
public:
	Ident(void) {
		puts(VERID);
		if (getenv("ACQ200_DEBUG")){
			acq200_debug = atoi(getenv("ACQ200_DEBUG"));
		}
		dbg(1, "debug level set %d", acq200_debug);
	}
};

static Ident __ident;
