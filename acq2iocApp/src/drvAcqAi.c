/* ------------------------------------------------------------------------- */
/* drvAcqAi.c - Driver layer for ACQ AI device                               */
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

/** @file drvAcqAi.c - Driver layer for Acq Ai device.
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

#include "drvAcqAi.h"

static struct AcqDev* pConfigFirst;

struct drvet drvAcqAi = {
	2,
	(DRVSUPFUN) acqReport,
	(DRVSUPFUN) acqInit
};

epicsExportAddress(drvet, drvAcqAi);

int acqReport(int interest)
{
	info("");
	return 0;
}

int acqInit(void)
{
	return 0;
}


static void acqMonitor(void *parm)
{
	AcqDev* dev = (AcqDev*)parm;
	int rc;

	while((rc = read(dev->fd, dev->rval, READSZ)) >= 0){
		if (rc == 0){
			/* could be pipe disconnect ... try again */
			sleep(1);
		}else{
			int nregs = MAX(1,rc/sizeof(unsigned));
			int ii;
		
			for (ii = 0; ii < nregs; ++ii){
				scanIoRequest(dev->aiScan[ii]);
			}		
		}
	}
	perror("drop out");
}


struct AcqDev *acqCreate(const char* device)
{
	struct AcqDev *plist = pConfigFirst;
	struct AcqDev *plast = plist;
	struct AcqDev *pnew;
	int idev = 0;
	int ii;

	for (; plist; plist = plist->pnext){
		plast = plist;
		++idev;
		if (strcmp(plist->device, device) == 0){
			return plist;
		}
	}	
	
	pnew = calloc(1, sizeof(struct AcqDev));
	assert(pnew);
	strcpy(pnew->device, device);
	pnew->fd = open(pnew->device, O_RDONLY);
	if (pnew->fd < 0){
		err("failed to open device \"%s\"", pnew->device);
		exit(errno);
	}	

	for (ii = 0; ii < MAXCHAN; ++ii){
		scanIoInit(&pnew->aiScan[ii]);
	}
	if (plast){
		plast->pnext = pnew;
	}else{
		pConfigFirst = pnew;
	}

	sprintf(pnew->threadname, "acq_mon%d", idev);
	pnew->monitor = epicsThreadCreate(
			pnew->threadname, 50, 
			epicsThreadGetStackSize(epicsThreadStackSmall),
			acqMonitor, pnew);
	return pnew;
}


int acqGetIoScanpvt(AcqDev *pdev, int lchan, IOSCANPVT* ppvt)
{
	if (lchan >= 0 && lchan < MAXCHAN){
		
		*ppvt = pdev->aiScan[lchan];	
		return 0;
	}else{
		return -1;
	}
}

int acqGetAi(AcqDev *dev, int lchan)
{
	assert(IN_RANGE(lchan, 1, MAXCHAN));
	return dev->rval[lchan-1];
}
