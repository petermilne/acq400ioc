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

/** @file VFile.cpp DESCR
 * 
 *  Created on: May 3, 2012
 *      Author: pgm
 */

#include <stdio.h>
#include <stdlib.h>
#include "VFile.h"

VFile::VFile(const char* fname, const char* mode, const char* _fmt) :
	fmt(_fmt) {
	fp = fopen(fname, mode);
	if (fp == 0){
		perror(fname);
		exit(1);
	}
}

VFile::~VFile() {
	fclose(fp);
}

bool VFile::readValue(int& value)
{
	char stuff[80];
	if (fgets(stuff, 80, fp)){
		int xx;
		int rc = sscanf(stuff, fmt, &xx);
		if (rc){
			value = xx;
			return true;
		}
	}

	return false;
}

bool VFile::writeValue(int value)
{
	return fprintf(fp, fmt, value) > 0;
}
