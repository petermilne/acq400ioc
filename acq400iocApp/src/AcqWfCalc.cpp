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

static long raw_to_volts(aSubRecord *prec) {

       double yy;
       long *raw = (long *)prec->a;
       int len = prec->noa;
       float * cooked = (float *)prec->vala;
       long rmax = *(long*)prec->valb;
       float vmax = *(float*)prec->valc;
       double ratio = vmax/rmax;


       if (report_done++ < 10){
	       printf("raw_to_volts() len:%d\n", len);
	       printf("raw_to_volts() rmax:%ld\n", rmax);
	       printf("raw_to_volts() vmax:%.2f\n", vmax);
       }

       if (rmax == 0){
	       if (report_done < 10){
		       printf("cheating, hardcode vmax, rmax");
	       }
	       ratio = 5.0/(1<<31);
       }

       for (int ii=0; ii <len; ii++) {
    	   yy = raw[ii];
    	   yy *= ratio;
           cooked[ii] = (float)yy;
           if (report_done < 10 && ii < 10){
        	   printf("raw_to_volts() raw:%08lx ratio:%.4f cooked:%.4f\n",
        	   	   raw[ii], ratio, cooked[ii]);
           }
       }

       return 0;
   }
 static registryFunctionRef my_asub_Ref[] = {
       {"raw_to_uvolts", (REGISTRYFUNCTION) raw_to_uvolts},
       {"raw_to_volts",  (REGISTRYFUNCTION) raw_to_volts},
 };

 static void raw_to_uvolts_Registrar(void) {
       registryFunctionRefAdd(my_asub_Ref, NELEMENTS(my_asub_Ref));
 }

 epicsExportRegistrar(raw_to_uvolts_Registrar);
