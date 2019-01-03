/* ------------------------------------------------------------------------- */
/* drvLxb.c - Driver layer for Linux BI device                               */
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

/** @file drvLxb.c - Driver layer for Linux BI device.
- $Revision: 1.2 $

*/
#include <drvSup.h>
#include <dbScan.h>

#include "epicsExport.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


#define acq200_debug lxb_debug
#include "local.h"

#include "drvLxb.h"

static struct LxbDev *pConfigFirst = NULL;
static int idev = 0;

struct drvet drvLxb = {
	2,
	(DRVSUPFUN) lxbReport,
	(DRVSUPFUN) lxbInit
};

epicsExportAddress(drvet, drvLxb);

int lxbReport(int interest)
{
	info("");
	return 0;
}

int lxbInit(void)
{
	return 0;
}

static void ringChanges(LxbDev *dev, int reg, unsigned mask)
{
	int ibit;

	for (ibit = 0; ibit < 32; ++ibit){
		if (((1<<ibit)&mask) != 0){
			scanIoRequest(dev->biScan[reg*32+ibit]);
		}
	}
}

static void lxbMonitor(void *parm)
{
	LxbDev* dev = (LxbDev*)parm;
	unsigned newstate[NREGS];
	int rc;

	while((rc = read(dev->fd, newstate, READSZ)) > 0){
		unsigned cos[NREGS];
		int nregs = MAX(1,rc/sizeof(unsigned));
		int ii;
		

		for (ii = 0; ii < nregs; ++ii){
			cos[ii] = newstate[ii] ^ dev->state[ii];
			if (cos[ii]){
				dev->state[ii] = newstate[ii];
				ringChanges(dev, ii, cos[ii]);
			}
		}		
	}
	perror("drop out");
}

struct LxbDev *lxbCreate(const char* device)
{
	struct LxbDev *plist = pConfigFirst;
	struct LxbDev *plast = plist;
	struct LxbDev *pnew;
	int idev = 0;
	int ii;

	for (; plist; plist = plist->pnext){
		plast = plist;
		++idev;
		if (strcmp(plist->device, device) == 0){
			return plist;
		}
	}	
	
	pnew = calloc(1, sizeof(struct LxbDev));
	assert(pnew);
	strcpy(pnew->device, device);
	pnew->fd = open(pnew->device, O_RDONLY);
	if (pnew->fd < 0){
		err("failed to open device \"%s\"", pnew->device);
		exit(errno);
	}	
	read(pnew->fd, &pnew->state, READSZ);


			
	for (ii = 0; ii < MAXBITS; ++ii){
		scanIoInit(&pnew->biScan[ii]);
	}
	if (plast){
		plast->pnext = pnew;
	}else{
		pConfigFirst = pnew;
	}

	sprintf(pnew->threadname, "lxb_mon%d", idev);
	pnew->monitor = epicsThreadCreate(
			pnew->threadname, 50, 
			epicsThreadGetStackSize(epicsThreadStackSmall),
			lxbMonitor, pnew);
	return pnew;
}


int lxbGetIoScanpvt(LxbDev *pdev, int bit, IOSCANPVT* ppvt)
{
	if (bit >= 0 && bit < MAXBITS){
		*ppvt = pdev->biScan[bit];	
		return 0;
	}else{
		return -1;
	}
}

int lxbGetBit(LxbDev *dev, int bit)
{
	int word = bit/32;
	unsigned mask = 1<<(bit - word*32);

	return (dev->state[word] & mask) != 0;
}
