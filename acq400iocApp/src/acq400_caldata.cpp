/* ------------------------------------------------------------------------- */
/* acq400_caldata.cpp acq400ioc						     */
/*
 * acq400_caldata.cpp
 *
 *  Created on: 12 May 2015
 *      Author: pgm
 */

/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2015 Peter Milne, D-TACQ Solutions Ltd                    *
 *                      <peter dot milne at D hyphen TACQ dot com>           *
 *                                                                           *
 *  This program is free software; you can redistribute it and/or modify     *
 *  it under the terms of Version 2 of the GNU General Public License        *
 *  as published by the Free Software Foundation;                            *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program; if not, write to the Free Software              *
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include "tinyxml2.h"

extern "C" {
	void* acq400_openDoc(const char* docfile, int* nchan);
	int acq400_getChannel(void *prv, int ch, const char* sw, float* eslo, float* eoff, int nocal);
	int acq400_isData32(void* prv);
};

using namespace tinyxml2;

#define RETNULL do { printf("ERROR %d\n", __LINE__); return 0; } while(0)

#define RETERRNULL(node2, node1, key) \
	if (((node2) = (node1)->FirstChildElement(key)) == 0){\
		printf("acq400_openDoc() ERROR:%d\n", __LINE__); \
		return 0;\
	}

void* acq400_openDoc(const char* docfile, int* nchan)
{
	printf("acq400_openDoc(%s)\n", docfile);


	XMLDocument* doc = new XMLDocument;
	XMLError rc = doc->LoadFile(docfile);

	if (rc != XML_NO_ERROR){
		printf("ERROR:%d\n", rc);
		return 0;
	}
	XMLNode* node;

	RETERRNULL(node, doc, "ACQ");
	RETERRNULL(node, node, "AcqCalibration");
	RETERRNULL(node, node, "Info");
	RETERRNULL(node, node, "SerialNum");

	XMLText* snum = node->FirstChild()->ToText();


	RETERRNULL(node, doc, "ACQ");
	RETERRNULL(node, node, "AcqCalibration");
	RETERRNULL(node, node, "Data");
	rc = node->ToElement()->QueryIntAttribute("AICHAN", nchan);
	if (rc == XML_NO_ERROR){
		printf("Docfile:%s Serialnumber:%s AICHAN:%d\n",
				docfile, snum->Value(), *nchan);
		return doc;
	}else{
		printf("ERROR getting AICHAN %d\n", rc);
		return 0;
	}
}

static int set_values(XMLElement *chdef, float* eslo, float* eoff)
{
	if (chdef->QueryFloatAttribute("eslo", eslo) != 0) RETNULL;
	if (chdef->QueryFloatAttribute("eoff", eoff) != 0) RETNULL;
	return 1;
}
static int _acq400_getChannel(XMLNode *range, int ch, float* eslo, float* eoff, int nocal)
{
	if (!nocal){
		/* search calibrated */
		for (XMLNode *cch = range->FirstChildElement("Calibrated"); cch;
				cch = cch->NextSibling()){
			int this_ch = -1;
			if (cch->ToElement()->QueryIntAttribute("ch", &this_ch) == 0 && this_ch == ch){
				return set_values(cch->ToElement(), eslo, eoff);
			}
		}
	}
	XMLElement *nominal = range->FirstChildElement("Nominal");
	if (!nominal) RETNULL;
	return set_values(nominal, eslo, eoff);
	return 0;
}
int acq400_isData32(void* prv)
{
	XMLDocument* doc = static_cast<XMLDocument*>(prv);
	XMLNode* node;
	int code_max;

	RETERRNULL(node, doc, "ACQ");
	RETERRNULL(node, node, "AcqCalibration");
	RETERRNULL(node, node, "Data");
	XMLError rc =  node->ToElement()->QueryIntAttribute("code_max", &code_max);

	if (rc == XML_NO_ERROR){
		return code_max > 65535;
	}else{
		printf("ERROR getting code_max %d\n", rc);
		return 0;
	}
}
int acq400_getChannel(void *prv, int ch, const char* sw, float* eslo, float* eoff, int nocal)
/* returns >0 on success */
{
	XMLDocument* doc = static_cast<XMLDocument*>(prv);

	XMLNode* node;

	RETERRNULL(node, doc, "ACQ");
	RETERRNULL(node, node, "AcqCalibration");
	RETERRNULL(node, node, "Data");
	for (XMLNode *range = node->FirstChildElement("Range"); range;
			range = range->NextSibling()){
		const char* findkey =range->ToElement()->Attribute("sw");
		int nosw = sw==0 || findkey==0 || strcmp(findkey, "default")==0;
		if (nosw || strcmp(sw, findkey) == 0){
			return _acq400_getChannel(range, ch, eslo, eoff, nocal);
		}
	}
	printf("ERROR: range \"%s\" not found\n", sw);
	return -1;
}
