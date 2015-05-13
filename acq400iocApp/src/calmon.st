
// seq calmon "uut=acq1001_064,site=1"
// seq calmon "uut=acq1001_064,site=2"
// dbpr acq1001_064:1:ADC_MODE
// dbpr acq1001_064:1:AI:CH:01 2

program calmon
option +r;

// nb : always site 1. That's a consequence of Master/Slave
int hi_res_mode; assign hi_res_mode to "{uut}:1:ADC_MODE";
monitor hi_res_mode;
int old_mode;

char* doc;
float eslo[33];	assign eslo to {};	// index from 1
float eoff[33]; assign eoff to {};
char* SW[2] = { "0", "1" };

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
			pvGet(hi_res_mode);
			old_mode = hi_res_mode;
			uut = macValueGet("uut"); 
			if (macValueGet("site") != 0){
				site = macValueGet("site");
			}
			sprintf(fname, "/dev/sites/%s/caldef.xml",  site);
			doc = acq400_openDoc(fname);
			
			for (ch = 1; ch <= 32; ++ch){				
				snprintf(pvname, 80, "%s:%s:AI:CH:%02d", uut, site, ch);				
				snprintf(pvfield, 100, "%s.%s", pvname, "ESLO");
				pvAssign(eslo[ch], pvfield);
				snprintf(pvfield, 100, "%s.%s", pvname, "EOFF");
				pvAssign(eoff[ch], pvfield);
			}
		} state make_change
	}
	
	state make_change {
		when() {
			for (ch = 1; ch <= 32; ++ch){		
				if (acq400_getChannel(doc, ch, SW[hi_res_mode], eslo+ch, eoff+ch, 0) == 1){ 
					//printf("ch:%2d eslo:%.6e eoff:%.6e\n", ch, eslo[ch], eoff[ch]);
					pvPut(eslo[ch]);
					pvPut(eoff[ch]);
				}else{
					printf("ERROR return from acq400_getChannel()\n");
				}
			}
		} state wait_change
	}
	
	state wait_change {
		when(hi_res_mode != old_mode){		
			printf("change detected hi_res_mode=>%d\n", hi_res_mode);
			pvGet(hi_res_mode);
			old_mode = hi_res_mode; 
		} state make_change
	}
}