/* ------------------------------------------------------------------------- */
/* file AcqAiHostDescr.h                                                                 */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2011 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>
 *  Created on: Feb 19, 2012
 *      Author: pgm

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

/** @file AcqAiHostDescr.h DESCR 
 *
 */

#ifndef ACQAIHOSTDESCR_H_
#define ACQAIHOSTDESCR_H_

#include "AcqHostDescr.h"

class AcqAiHostDescr: public AcqHostDescr {

protected:
	AcqAiHostDescr();
public:
	virtual ~AcqAiHostDescr();

	static AcqAiHostDescr* create(const char* inp);

	virtual bool getValue(int &value) = 0;
};

#endif /* ACQAIHOSTDESCR_H_ */
