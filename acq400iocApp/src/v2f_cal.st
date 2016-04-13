// seq v2f_cal "uut=acq1001_064,och=1"

/*
 och : output channel : run one sm per output
 CSF : code scale factor in V
 DSF : DDS Scale factor in Hz?. 
 
CSF = VRANGE/ncodes(16B|24B)
DSF = f(16B|24B, 1M|10M)
CAL_OFF = EOFF/CSF + UFO/*DSF
CAL_GRAD = DSF*ESLO/CSF
*/


program v2f_cal
option +r;



int vsite = 1;
int fsite = 2;

float ext_eslo[33]; 	assign ext_eslo to "{uut}:1:AI:CAL:ESLO";
float ext_eoff[33]; 	assign ext_eoff to "{uut}:1:AI:CAL:EOFF";

monitor ext_eslo;
monitor ext_eoff;

int src; 		assign src to "{uut}:2:V2F:SRC:{och}";
int hf; 		assign hf to "{uut}:2:V2F:HIGH_FREQ";
int ufo;		assign ufo to "{uut}:2:V2F:FREQ_OFFSET";

monitor src;
monitor hf;
monitor ufo;

int slo;		assign slo to "{uut}:2:V2F:SLO:{och}";
int off;		assign off to "{uut}:2:V2F:OFF:{och}";

int src0;
int hf0;
int ufo0;

float VR = 20;		/* should be set on load */
int is24b = 1;		/* should be set on load */


float eoff0;
float eslo0;

float CSF;


// [16b|24b][lf|hf]
int DSF[2][2] = {
	{ 21475492, 	214754919 	},
	{ 83886, 	838861		}
};

ss v2f_cal {
	state init {
		entry {
			CSF = (VR/(1<<(is24b?24:16)));
			pvGet(src);
			pvGet(hf);
			pvGet(ufo);
			pvGet(ext_eslo);
			pvGet(ext_eoff);
		}
	
		when() {
	
		} state setValues
	}
	
	state setValues {
		entry {
			int dsf = DSF[is24b][hf];
			if (!hf && ufo > 2) ufo = 2;
	

			off = ext_eoff[src]/CSF + ufo*dsf;  
			slo = ext_eslo[src]/CSF * dsf;
	
		printf("v2f_cal(%s)\n", macValueGet("och"));
		printf("	src %d hf %d ufo %d\n", src, hf, ufo);
		printf("off: %10d := %e/%e + %d*%d\n", off, ext_eoff[src], CSF, ufo, dsf);
		printf("slo: %10d := %e/%e * %d\n", slo, ext_eslo[src], CSF, dsf);
		
			src0 = src;
			hf0 = hf;
			ufo0 = ufo;
			eoff0 = ext_eoff[src];
			eslo0 = ext_eslo[src];
	
			pvPut(off);
			pvPut(slo);
		}
		when() {
		
		} state wait_change
	}
	
	state wait_change {
		when(src != src0 || hf != hf0 || ufo != ufo0 || 
			ext_eoff[src] != eoff0 || ext_eslo[src] != eslo0){

		} state setValues
	}
		
}