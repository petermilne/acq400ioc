
// seq calmon "uut=acq1001_064,site=1"
// seq calmon "uut=acq1001_064,site=2"
// dbpr acq1001_064:1:ADC_MODE
// dbpr acq1001_064:1:AI:CH:01 2
// acq1001_064:1:AI:CH:01
// acq1001_064:1:AI:TW:01:V
// acq1001_064:1:AI:WF:01:V

program calmon
option +r;

// nb : always site 1. That's a consequence of Master/Slave
int hi_res_mode; assign hi_res_mode to "{uut}:1:ADC_MODE";
monitor hi_res_mode;
int hi_res_mode0;

int adc_cal_mode; assign adc_cal_mode to "{uut}:1:ADC_CAL";
monitor adc_cal_mode;
int adc_cal_mode0;


float eslo[33];		assign eslo to {};	// index from 1
float eoff[33]; 	assign eoff to {};

char* SW[2] = { "0", "1" };
char* doc;
char fname[80];
char* uut;
char* site = "1";		// default site 1, should override 
int ch;
char pvname[80];
char pvfield[100];
%%extern void* acq400_openDoc(const char* docfile);
%%extern int acq400_getChannel(void *prv, int ch, const char* sw, float* eslo, float* eoff, int nocal);

ss calmon {
	state init {
		when() {
			printf("calmon R1000 site:%s\n", site);
			pvGet(hi_res_mode);  hi_res_mode0 = hi_res_mode;
			pvGet(adc_cal_mode); adc_cal_mode0 = adc_cal_mode;
			uut = macValueGet("uut"); 
			if (macValueGet("site") != 0){
				site = macValueGet("site");
			}
			sprintf(fname, "/dev/sites/%s/caldef.xml",  site);
			doc = acq400_openDoc(fname);
		} state init2
	}
	state init2 {
		when(doc == 0) {
			printf("ERROR failed to read caldef.xml\n");
		} state error
		
		when(doc != 0){
			for (ch = 1; ch <= 32; ++ch){				
				snprintf(pvname, 80, "%s:%s:AI:CH:%02d", uut, site, ch);						
				snprintf(pvfield, 100, "%s.%s", pvname, "ESLO");
				pvAssign(eslo[ch], pvfield);
				if (ch==1) printf("assign %s\n",pvfield);
				
				snprintf(pvfield, 100, "%s.%s", pvname, "EOFF");
				pvAssign(eoff[ch], pvfield);
				if (ch==1) printf("assign %s\n",pvfield);
			}
		} state make_change		
	}
	
	state error {	
		when(delay(1000)) {
		
		} state error
	}
	
	state make_change {
		when() {
			pvGet(adc_cal_mode);
			if (adc_cal_mode != 0){
				printf("make_change site=%s %s\n", site, adc_cal_mode==1? "caldef": "calibrated");
				for (ch = 1; ch <= 32; ++ch){		
					if (acq400_getChannel(doc, ch, SW[hi_res_mode], &eslo[ch], &eoff[ch], adc_cal_mode==1) == 1){
						if (ch==1||ch==32){
							printf("site:%s ch:%2d eslo:%.6e eoff:%.6e\n", 
										site, ch, eslo[ch], eoff[ch]);
						}
						pvPut(eslo[ch]);
                                                pvPut(eoff[ch]);
					}else{
						printf("ERROR return from acq400_getChannel()\n");
					}
				}
			}else{
				printf("make_change: STUB\n");
			}
		} state wait_change
	}
	
	state wait_change {
		when(hi_res_mode != hi_res_mode0 || adc_cal_mode != adc_cal_mode0){
			if (hi_res_mode != hi_res_mode0){		
				printf("change detected hi_res_mode=>%d\n", hi_res_mode);
				hi_res_mode0 = hi_res_mode;				
			}
			if (adc_cal_mode != adc_cal_mode0){
				printf("cal mode changed=>%d\n", adc_cal_mode);
				adc_cal_mode0 = adc_cal_mode;
			}			 
		} state make_change
	}
}