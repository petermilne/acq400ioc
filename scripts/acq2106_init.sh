#!/bin/sh

if [ "x$1" = "xcreate" ]; then
	ln -s /usr/local/epics/acq2106_set_ext_clk /etc/acq400/0
	ln -s /usr/local/epics/acq2106_set_int_clk /etc/acq400/0
	exit 0
fi


