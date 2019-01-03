/* ------------------------------------------------------------------------- */
/* Sysfs.h : SYSFS EPICS - interface to Sysfs handlers device support        */
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


#ifndef __SYSFS_H__
#define __SYSFS_H__

/** @file Sysfs.h Sysfs relies on an encoded INP field.
 *  Encoding as follows:
 
@[#!]path[,ftype][,fmt] [args]

@ : literal '\@' - EPICS seems to need this
#!: path is a program, execute it

ftype: 
r|w    : read or write [implied default: r]
a      : ascii data (default)
b<N>   : binary data, N bytes (N =2,4 supported)

fmt:    : for ascii data, sscanf() format for conversion

args:  regular space separated command line arguments for #! case

WARNING: 
 - the INP record is limited to 40 chars including the @
so scope for formatting is VERY LIMITED.
 - @path,ftype,fmt may NOT contain spaces

Examples:
 - field(INP, "@/dev/mean/\$(cid) rb4) # \$(cid) : channel id 01..96, read 4 byte mean value
 - field(INP, "@#!sysmon -T 1")       # get the temperature by running sysmon, read ascii value
 - field(INP, "@#!acqcmd setArm")     # run the setArm command (using an aiRecord?).


- field(OUT, "@/dev/acq196/AO/\$(cid)"	# AO channel
- field(INP, "@/dev/rtmdio32/dio32,13"  # BI channel, bit 13
- field(OUT, "@/dev/rtmdio32/dio32,12"  # BO channel, bit 12

*/

/*
 * unfortunately aiRecord, aoRecord et al don't have a base class,
 * so we're forced into either using void* or a lot of repetition.
 * we choose void* to reduce the repetition...
 */

typedef void* RECORD;

enum RTYPE {
	AI, AO, BI, BO, WFAI, WFAO, stringin, stringout
};

class RecordHandler {
public:
	virtual int read(RECORD record) = 0;
	virtual int write(RECORD record) = 0;
};
/** factory class connects aiRecord instance to Device Support. */
class SysfsAdapter: public RecordHandler {
protected:
	const char* name;


	SysfsAdapter(const char* _name) : name(_name) {}
public:
	virtual ~SysfsAdapter() {}



	const char* getName(void) { return name; }

	/** create an adapter for record and store in lookup table. */
	static int create(RECORD record, RTYPE rtype);

	/** retrieve adapter from lookup table. */
	static SysfsAdapter* getAdapter(RECORD record, RTYPE rtype);     
};




#endif // SYSFS_H__
