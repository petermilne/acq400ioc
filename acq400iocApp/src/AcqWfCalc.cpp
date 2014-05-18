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


#define OR_THRESHOLD 32

/* ARGS:
 * INPUTS:
 * INPA : const T raw[size}
 * INPB : long maxcode
 * INPC : float vmax
 * INPD : long threshold (distance from rail for alarm)
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




template <class T> long square(T raw) {
	return raw*raw;
}

// avoid overflow by truncation
template<long> long square(long raw) {
	raw >>= 16;
	return raw*raw;
}

template <class T>
long raw_to_volts(aSubRecord *prec) {
	double yy;
	T *raw = (T *)prec->a;
	int len = prec->noa;
	float * cooked = (float *)prec->vala;
	long rmax = *(long*)prec->b;
	float vmax = *(float*)prec->c;
	double ratio = vmax/rmax;
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
	long long sum = 0;
	long long sumsq = 0;
	bool compute_squares = p_stddev != 0 || p_rms != 0;

	if (++report_done == 1){
		printf("raw_to_volts() ->b %p rmax %ld\n", prec->b, rmax);
		printf("raw_to_volts() ->c %p vmax %f\n", prec->c, vmax);
		printf("raw_to_volts() len:%d th:%ld p_over:%p\n",
				len, alarm_threshold, p_over_range);
	}
	if (rmax == 0){
		ratio = 10.0/(sizeof(T)==4? INT_MAX: SHRT_MAX);

		if (report_done < 2){
			printf("cheating, hardcode vmax, rmax ratio %f\n", ratio);
		}
	}

	min_value = raw[0];
	max_value = raw[0];

	for (int ii=0; ii <len; ii++) {
		if (raw[ii] > max_value) max_value = raw[ii];
		if (raw[ii] < min_value) min_value = raw[ii];
		if (raw[ii] > alarm_threshold) 	over_range = 1;
		if (raw[ii] < -alarm_threshold) over_range = -1;
		if (compute_squares) sumsq += square<T>(raw[ii]);
		sum += raw[ii];

		yy = raw[ii];
		yy *= ratio;
		cooked[ii] = (float)yy;
	}

	if (p_over_range){
		*p_over_range = over_range;
	}
	if (p_max) 	*p_max = max_value * ratio;
	if (p_min) 	*p_min = min_value * ratio;
	if (p_mean) 	*p_mean = sum*ratio/len;

	if (p_rms)	*p_rms = sqrt((double)sumsq/len) * ratio;
	if (p_stddev)	*p_stddev = sqrt(((double)sumsq - (double)sum*sum/len)/len) * ratio;
	return 0;
}

 static registryFunctionRef my_asub_Ref[] = {
       {"raw_to_uvolts", (REGISTRYFUNCTION) raw_to_uvolts},
       {"raw_to_volts_LONG",  (REGISTRYFUNCTION) raw_to_volts<long>},
       {"raw_to_volts_SHORT",  (REGISTRYFUNCTION) raw_to_volts<short>},
 };

 static void raw_to_uvolts_Registrar(void) {
       registryFunctionRefAdd(my_asub_Ref, NELEMENTS(my_asub_Ref));
 }

 epicsExportRegistrar(raw_to_uvolts_Registrar);
