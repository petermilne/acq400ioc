/*
 * AcqHostDescr.h
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

#ifndef ACQHOSTDESCR_H_
#define ACQHOSTDESCR_H_

//#include "dbScan.h"

class AcqHostDescr {

protected:
	IOSCANPVT pvt;

	AcqHostDescr();
public:
	virtual ~AcqHostDescr();

	int getIoScan(IOSCANPVT *ppvt);
	virtual void onScan();

	static const char* makeName(const char* defn, char name[], int maxname);
};

#endif /* ACQHOSTDESCR_H_ */
