/* ------------------------------------------------------------------------- */
/* file FileMonitor.h                                                                 */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2011 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>
 *  Created on: Feb 22, 2012
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

/** @file FileMonitor.h DESCR 
 *
 */


#ifndef FILEMONITOR_H_
#define FILEMONITOR_H_

class FileMonitor {

protected:
	FileMonitor() {}
public:
	virtual ~FileMonitor() {}
	virtual void waitChange() = 0;

	static FileMonitor* create(const char* fname);
};

#endif /* FILEMONITOR_H_ */
