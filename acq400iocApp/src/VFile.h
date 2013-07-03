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

/** @file VFile.h DESCR
 * 
 *  Created on: May 3, 2012
 *      Author: pgm
 */

#ifndef VFILE_H_
#define VFILE_H_

class VFile {
	FILE *fp;
	const char* fmt;
public:
	VFile(const char* fname, const char* mode = "r", const char* fmt = "%d");
	virtual ~VFile();

	virtual bool readValue(int& value);
	virtual bool writeValue(int value);
};

#endif /* VFILE_H_ */
