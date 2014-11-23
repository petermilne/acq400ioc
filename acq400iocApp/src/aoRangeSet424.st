/*************************************************************************\
ao424_range_set.st : repeat transient on termination
rescale AO records depending on range.
\*************************************************************************/
program ao424_range_set
option +r;

int range;	assign range to "{UUT}:{SITE}:RANGE:{CID}";
double drvl;	assign drvl to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.DRVL";
double drvh;	assign drvh to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.DRVH";
double eslo;	assign eslo to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.ESLO";
double hopr;	assign hopr to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.HOPR";
double lopr;	assign lopr to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.LOPR";
double val;	assign val  to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}";

string model;	assign model to "{UUT}:{SITE}:MODEL";


monitor range;

/* force initial update */
int range0 = -1;

%{
	static void getGain(int range, double* drvl, double* drvh)
	{
		switch(range){
			case 0: *drvl = 0; 	*drvh = 4.9999;	break; 
			case 1: *drvl = 0; 	*drvh = 9.9999;	break;
			case 2: *drvl = -5; 	*drvh = 4.9999;	break;
			case 3: *drvl = -10; 	*drvh = 9.9999;	break;
			case 4: *drvl = -2.5;	*drvh = 2.4999;	break;
			case 5: *drvl = -2.5;	*drvh = 7.4999;	break;
		}
	}
}%


ss gain_set
{
	state update 
	{
		when(range != range0){
			getGain(range, &drvl, &drvh);
			hopr = drvh;
			lopr = drvl;
			eslo = (hopr - lopr) / 65535;
			pvPut(drvl);
			pvPut(drvh);
			pvPut(eslo);
			pvPut(hopr);
			pvPut(lopr);
			val = 0;
			pvPut(val);
			range0 = range;
		} state update
	}
}