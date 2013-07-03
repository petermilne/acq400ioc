/*
 * AcqWfHostDescr.h
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

#ifndef ACQWFHOSTDESCR_H_
#define ACQWFHOSTDESCR_H_

#include "AcqHostDescr.h"

class AcqWfHostDescr: public AcqHostDescr {

protected:
	AcqWfHostDescr();
public:

	virtual ~AcqWfHostDescr();

	static AcqWfHostDescr* create(const char* inp);


	virtual int read(struct waveformRecord* pwf) = 0;
};

#endif /* ACQWFHOSTDESCR_H_ */
