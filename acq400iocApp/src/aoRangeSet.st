/*************************************************************************\
ao420_range_set.st : repeat transient on termination
rescale AO records depending on range.
\*************************************************************************/
program ao420_range_set
option +r;

int ref;	assign ref to "{UUT}:{SITE}:AO:REF";
int gain;	assign gain to "{UUT}:{SITE}:AO:GAIN:CH:{CID}";

double drvl;	assign drvl to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.DRVL";
double drvh;	assign drvh to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.DRVH";
double eslo;	assign eslo to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.ESLO";
double hopr;	assign hopr to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.HOPR";
double lopr;	assign lopr to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}.LOPR";
double val;	assign val  to "{UUT}:{SITE}:AO:SLOW_SET:CH:{CID}";

string model;	assign model to "{UUT}:{SITE}:MODEL";


monitor ref; 
monitor gain;

int ref0;
int gain0;

int fmc_set = 0;

ss gain_set
{
	state init {
		when() {		
			ref0 = ref;
			gain0 = gain;
			pvGet(model);
		
			/* FMC is always fixed 5V */
			if (!fmc_set && strncmp(model, "AO420FMC", 9) == 0){
				ref = 1;
				pvPut(ref);
				ref0 = !ref;
				fmc_set = 1;							
			}				
		} state run
	}
	state run 
	{
		when(ref != ref0 || gain != gain0){
			if (ref==0 && gain==0){
				drvl = lopr = -2.5;
				drvh = hopr = 2.4999;
			}else if (ref==1 && gain==1){
				drvl = lopr = -10;
				drvh = hopr = 9.9999;
			}else{
				drvl = lopr = -5;
				drvh = hopr = 4.9999;
			}
			eslo = (hopr - lopr) / 65535;
			pvPut(drvl);
			pvPut(drvh);
			pvPut(eslo);
			pvPut(hopr);
			pvPut(lopr);
			val = 0;
			pvPut(val);
		} state init
	}
}