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
#define PS_SCAN_FREQ		"SCAN_FREQ"			/* asynFloat64,       	r/w */
#define PS_FS			"FS"				/* asynFloat64		r/w */
#define PS_AI_CH		"AI"				/* asynInt32, per port  r/w */
#define PS_STEP			"STEP"				/* asynInt32, data set step r/o */
#define PS_DELTA_NS		"DELTA_NS"			/* asynInt32, time tick r/o */
#define PS_MEAN_OF_N		"MEAN_OF_N"			/* asynInt32, mean of n r/w n : 1<<{0,1,2,3,4,5,6,7,8} */


class Acc {
	const int nchan;
public:
	int *ch;
	int nadd;


	Acc(int _nchan): nchan(_nchan) {
		ch = new int[nchan];
	}
	void clear() {
		memset(ch, 0, nchan*sizeof(int));
		nadd = 0;
	}
};

class acq400Ai: public asynPortDriver {

protected:
	int P_NCHAN;
	int P_NSAM;
	int P_SCAN_FREQ;
	int P_FS;
	int P_AI_CH;
	int P_STEP;
	int P_DELTA_NS;
	int P_MEAN_OF_N;

	const int nsam;
	const int nchan;
	int scan_ms;
	unsigned delta_ns;

	int step;
	int buffer_start_sample;
	epicsTimeStamp t0, t1;



	int mean_of_n0;

	Acc acc;

	acq400Ai(const char *portName, int nsam, int nchan, int scan_ms);

	void handleBuffer(int ib);
public:
	static int factory(const char *portName, int nsam, int nchan, int scan_ms);

	virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

	virtual asynStatus updateTimeStamp(epicsTimeStamp& ts);

	virtual void task();
};
#endif /* ACQ400IOCAPP_SRC_ACQ400AI_H_ */
