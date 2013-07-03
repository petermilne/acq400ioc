/*
 * AcqHostDevice.h
 *
 *  Created on: Feb 23, 2012
 *      Author: pgm
 */

#ifndef ACQHOSTDEVICE_H_
#define ACQHOSTDEVICE_H_

struct strCmp {
    bool operator()( const char* s1, const char* s2 ) const {
      return strcmp( s1, s2 ) < 0;
    }
};

class AcqHostDevice;
class AcqHostDescr;

typedef  map<const char*, AcqHostDevice*, strCmp> DeviceMap;
typedef  map<const char*, AcqHostDevice*, strCmp>::iterator DeviceMapIterator;

typedef map<int, AcqHostDescr*> ChannelMap;
typedef map<int, AcqHostDescr*>::iterator ChannelMapIterator;

class AcqHostDevice {

protected:
	char* fname;
	bool channel_data_valid;
	const char* thread_name;
	epicsThreadId monitor;

	void runMonitor();
	void addChannel(int ch);
	virtual void onChange();

	AcqHostDevice(const char* _fname);
public:
	ChannelMap channels;

	const char* getFname() const { return fname; }

	virtual ~AcqHostDevice();

	virtual bool getValue(int idx, int& value) {
		return false;
	}

	virtual void monitorFunction();

	static const char* ROOT;
};

#endif /* ACQHOSTDEVICE_H_ */
