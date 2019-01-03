/* ------------------------------------------------------------------------- */
/* drvLxb.h - Linux BI driver common defs                                    */
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



#ifndef __drvLxb_H__
#define __drvLxb_H__

#include "epicsThread.h"

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2

#define MAXBITS	128
#define NREGS	(MAXBITS/32)
#define MAXDEV  80


typedef struct LxbDev {
	struct LxbDev *pnext;
	char device[MAXDEV];
	char threadname[MAXDEV];
	int fd;
	
	IOSCANPVT biScan[MAXBITS];	
	unsigned state[NREGS];
	epicsThreadId monitor;
} LxbDev;

#define READSZ	16

int lxbReport(int interest);
int lxbInit(void);



LxbDev* lxbCreate(const char* device);

int lxbGetBit(LxbDev *dev, int bit);
int lxbGetIoScanpvt(LxbDev* dev, int bit, IOSCANPVT* ppvt);


#endif /* __drvLxb_H__ */
