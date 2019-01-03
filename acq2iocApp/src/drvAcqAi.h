/* ------------------------------------------------------------------------- */
/* drvAcqAi.h - ACQ AI driver common defs                                    */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2008 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>
 *                      http://www.d-tacq.com/

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



#ifndef __drvAcqAi_H__
#define __drvAcqAi_H__

#include "epicsThread.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2

#define MAXCHAN	96
#define MAXDEV  80


typedef struct AcqDev {
	struct AcqDev* pnext;
	char device[MAXDEV];
	char threadname[MAXDEV];
	int fd;
	
	IOSCANPVT aiScan[MAXCHAN];	
	epicsThreadId monitor;
	int rval[MAXCHAN];		/* current value */
} AcqDev;

#define READSZ	(MAXCHAN*sizeof(int))

int acqReport(int interest);
int acqInit(void);



AcqDev* acqCreate(const char* device);

int acqGetAi(AcqDev *dev, int lchan);
int acqGetIoScanpvt(AcqDev* dev, int bit, IOSCANPVT* ppvt);


#endif /* __drvAcqAi_H__ */
