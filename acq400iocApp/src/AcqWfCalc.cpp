/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2011 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>

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

/** @file AcqWfCalc.cpp Calc record to give calibrated WF output
 * 
 *  Created on: Apr 12, 2012
 *      Author: pgm
 *      INPA : the raw waveform
 *      INPB : VMAX[NCHAN]
 *      INPC : VMIN[NCHAN]
 */

#include "local.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>


#include "alarm.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "dbScan.h"
#include "drvSup.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "aSubRecord.h"
#include "link.h"
#include "menuFtype.h"
#include "epicsExport.h"
#include "epicsThread.h"
#include "waveformRecord.h"

#include <registryFunction.h>
#include <epicsExport.h>


 #include <math.h>

#define MILLION	1000000

static long raw_to_uvolts(aSubRecord *prec) {
       long long yy;
       long *raw = (long *)prec->a;
       int len = prec->noa;
       long * cooked = (long *)prec->vala;

       for (int ii=0; ii <len; ii++) {
    	   yy = raw[ii] >> 8;
    	   yy *= 10*MILLION;
           cooked[ii] = (long)(yy >> 23);
       }

       return 0;
   }

static int report_done;
static int verbose;


#define OR_THRESHOLD 32

/* ARGS:
 * INPUTS:
 * INPA : const T raw[size}
 * INPB : long maxcode
 * INPC : float vmax
 * INPD : long threshold (distance from rail for alarm)
 * INPO : double AOFF if set
 * INPS : double ASLO if set
 * OUTPUTS:
 * VALA : float* cooked
 * optional:
 * VALB : long* alarm
 * VALC : float* min
 * VALD : float* max
 * VALE : float* mean
 * VALF : float* stddev
 * VALG : float* rms
 */




template <class T> double square(T raw) {
	double dr = static_cast<double>(raw);
	return dr*dr;
}


template <class T>
long raw_to_volts(aSubRecord *prec) {
	double yy;
	T *raw = (T *)prec->a;
	int len = prec->noa;
	float * cooked = (float *)prec->vala;
	long rmax = *(long*)prec->b;
	float vmax = *(float*)prec->c;
	double aoff = *(double*)prec->o;
	double aslo = *(double*)prec->s;
	long* p_thresh = (long*)prec->d;
	long* p_over_range = (long*)prec->valb;
	float* p_min = (float*)prec->valc;
	float* p_max = (float*)prec->vald;
	float* p_mean = (float*)prec->vale;
	float* p_stddev = (float*)prec->valf;
	float* p_rms = (float*)prec->valg;
	long min_value;
	long max_value;
	long over_range = 0;
	long alarm_threshold = rmax - (p_thresh? *p_thresh: OR_THRESHOLD);
	double sum = 0;
	double sumsq = 0;
	bool compute_squares = p_stddev != 0 || p_rms != 0;



	if (::verbose && ++report_done == 1){
		printf("aoff count: %u value: %p type: %d\n", prec->noo, prec->o, prec->fto);
		printf("aslo count: %u value: %p type: %d\n", prec->nos, prec->s, prec->fts);
		printf("raw_to_volts() ->b %p rmax %ld\n", prec->b, rmax);
		printf("raw_to_volts() ->c %p vmax %f\n", prec->c, vmax);
		printf("raw_to_volts() len:%d th:%ld p_over:%p\n",
				len, alarm_threshold, p_over_range);
	}

	if (::verbose && strstr(prec->name, ":01") != 0){
		printf("%s : aslo:%.6e aoff:%.6e\n", prec->name, aslo, aoff);
	}

	if (aslo*rmax > vmax*.8){
		aslo = vmax/rmax;
		if (report_done == 1){
			printf("%s range change overrides cal change aslo to %f\n",
					prec->name, aslo);
		}
	}

	min_value = raw[0];
	max_value = raw[0];

	for (int ii=0; ii <len; ii++) {
		T rx = raw [ii];
		if (sizeof(T) == 4) rx >>= 8;
		if (rx > max_value) max_value = rx;
		if (rx < min_value) min_value = rx;
		if (rx > alarm_threshold) 	over_range = 1;
		if (rx < -alarm_threshold) over_range = -1;
		if (compute_squares) sumsq += square<T>(rx);
		sum += rx;
		yy = rx*aslo + aoff;
		cooked[ii] = (float)yy;
	}

	if (p_over_range){
		*p_over_range = over_range;
	}
	if (p_max) 	*p_max = max_value*aslo + aoff;
	if (p_min) 	*p_min = min_value*aslo + aoff;
	if (p_mean) 	*p_mean = (sum*aslo)/len + aoff;

	if (p_rms)	*p_rms = sqrt(sumsq/len)*aslo + aoff;
	if (p_stddev)	*p_stddev = sqrt((sumsq - (sum*sum)/len)/len) * aslo;
	return 0;
}

long timebase(aSubRecord *prec) {
	long pre = *(long*)prec->a;
	long post = *(long*)prec->b;
	float dt = *(float*)prec->c;
	float * tb = (float *)prec->vala;
	long maxtb = prec->nova;
	long len = pre + post + 1;    // [pre .. 0 .. post]

	if (len > maxtb+1){
		printf("timebase ERROR maxtb:%ld len:%ld\n", maxtb, len);
	}
	if (len > maxtb) len = maxtb;

	for (int ii = 0; ii < len; ++ii){
		tb[ii] = (ii - pre)*dt;
	}
	return 0;
}

 static registryFunctionRef my_asub_Ref[] = {
       {"raw_to_uvolts", (REGISTRYFUNCTION) raw_to_uvolts},
       {"raw_to_volts_LONG",  (REGISTRYFUNCTION) raw_to_volts<long>},
       {"raw_to_volts_SHORT",  (REGISTRYFUNCTION) raw_to_volts<short>},
       {"timebase", (REGISTRYFUNCTION) timebase},
 };

 static void raw_to_uvolts_Registrar(void) {
	 const char* vs = getenv("ACQWFCALC_VERBOSE");
	 if (vs) verbose = atoi(vs);

	 registryFunctionRefAdd(my_asub_Ref, NELEMENTS(my_asub_Ref));
 }

 epicsExportRegistrar(raw_to_uvolts_Registrar);
