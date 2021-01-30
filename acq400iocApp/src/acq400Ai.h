/*
 * acq400ai.h : pull 1..10Hz scalar updates from raw buffers. replaces AcqAiHost*
 *
 *  Created on: 30 Jan 2021
 *      Author: pgm
 */

#ifndef ACQ400IOCAPP_SRC_ACQ400AI_H_
#define ACQ400IOCAPP_SRC_ACQ400AI_H_

#include "asynPortDriver.h"

#define PS_NCHAN 		"NCHAN"				/* asynInt32, 		r/o */
#define PS_NSAM			"NSAM"				/* asynInt32,       	r/o */
#define PS_SCAN_MSEC		"SCAN_MS"			/* asynInt32,       	r/o */
#define PS_AI_CH		"AI"				/* asynInt32, per port  r/w */


class acq400Ai: public asynPortDriver {


protected:
	int P_NCHAN;
	int P_NSAM;
	int P_SCAN_MSEC;
	int P_AI_CH;

	const int nsam;
	const int nchan;
	int scan_ms;

	acq400Ai(const char *portName, int nsam, int nchan, int scan_ms);

	void handleBuffer(int ib);
	void outputSampleAt(epicsInt32* raw, int offset);
public:
	static int factory(const char *portName, int nsam, int nchan, int scan_ms);

	virtual void task();
};
#endif /* ACQ400IOCAPP_SRC_ACQ400AI_H_ */
