/* ------------------------------------------------------------------------- */
/* ao32cal_linearity.h - library computes 4th order compensation for AO32CPCI*/
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2009 Peter Milne, D-TACQ Solutions Ltd
 *                      <Peter dot Milne at D hyphen TACQ dot com>

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


#ifndef __AO32CAL_LINEARITY_H__	
#define  __AO32CAL_LINEARITY_H__


/** abstract class allows clients to calibrate AO data. */
class Ao32CalLinearizer {

protected:
	const int slot;
	const char* serial;

	Ao32CalLinearizer(int _slot);

public:
	virtual ~Ao32CalLinearizer() {}
	const char *getSerial(void) const { return serial; }
	int hasCalData(void) const;

	virtual int isCooked(void) const = 0;
	virtual int isUnipolar(int channel) const = 0;
	virtual unsigned getResbits() const = 0;
	virtual int convert(int channel, double volts) const = 0;

	void writeRaw(int channel, int raw) const;
	void writeVolts(int channel, double volts) const {
		writeRaw(channel, convert(channel, volts));
	}

	/** factory method: */
	static Ao32CalLinearizer *create(int slot);
};



#endif //  __AO32CAL_LINEARITY_H__	
