/*
 * AcqWfHostDescr.h
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

#ifndef ACQWFAWGHOSTDESCR_H_
#define ACQWFAWGHOSTDESCR_H_

#include "AcqHostDescr.h"

class AcqWfAWGHostDescr: public AcqHostDescr {

protected:
	AcqWfAWGHostDescr();
public:

	virtual ~AcqWfAWGHostDescr();

	static AcqWfAWGHostDescr* create(const char* inp);


	virtual int write(struct waveformRecord* pwf) = 0;
	virtual int read(struct waveformRecord* pwf) = 0;
};

#endif /* ACQWFAWGHOSTDESCR_H_ */
