/* ------------------------------------------------------------------------- */
/* file FileMonitor.cpp                                                                 */
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

/** @file FileMonitor.cpp DESCR 
 *
 */
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "FileMonitor.h"

#define EVENT_SIZE  		(sizeof (struct inotify_event))
#define EVENT_BUF_LEN     	(1024 * ( EVENT_SIZE + 16 ))

class FileMonitorImpl: public FileMonitor {
	int fd;
	int wd;

public:
	~FileMonitorImpl();
	FileMonitorImpl(const char* fname);

	virtual void waitChange();
};

FileMonitorImpl::FileMonitorImpl(const char* fname)
{
	fd = inotify_init();
	if (fd < 0){
		perror("inotify_init");
		exit(1);
	}
	wd = inotify_add_watch( fd, fname, IN_CLOSE_WRITE|IN_DELETE);
	if (wd <= 0){
		perror("inotify_add_watch");
		exit(1);
	}
}

FileMonitorImpl::~FileMonitorImpl() {
	inotify_rm_watch( fd, wd );
	close( fd );
}

void FileMonitorImpl::waitChange()
{
	char buffer[EVENT_BUF_LEN];
	int length = read(fd, buffer, EVENT_BUF_LEN );
	  /*checking for error*/
	if ( length < 0 ) {
		perror( "read" );
	}

	return;
}

FileMonitor* FileMonitor::create(const char *fname)
{
	int fd = open(fname, O_RDONLY);
	if (fd < 0){
		perror(fname);
		return 0;
	}else{
		close(fd);
		return new FileMonitorImpl(fname);
	}
}

