

program calmon
option +r;

int hi_res_mode; assign hi_res_mode to "{uut}:1:ADC_MODE";
int old_mode;

char* doc;
float eslo = -8;
float eoff = -9;
char* SW[2] = { "0", "1" };

int ch;
%%extern void* acq400_openDoc(const char* docfile);
%%extern int acq400_getChannel(void *prv, int ch, const char* sw, float* eslo, float* eoff, int nocal);


ss calmon {
	state init {
		when() {
			pvGet(hi_res_mode);
			old_mode = hi_res_mode; 
			doc = acq400_openDoc("/dev/sites/1/caldef.xml");
	
			for (ch = 1; ch <= 32; ++ch){		
				if (acq400_getChannel(doc, ch, SW[hi_res_mode], &eslo, &eoff, 0) == 1){ 
					printf("ch:%2d eslo:%.6e eoff:%.6e\n", ch, eslo, eoff);
				}else{
					printf("ERROR return from acq400_getChannel()\n");
				}
			}
		} state wait_change
	}
	
	state wait_change {
		when(hi_res_mode != old_mode){		
			printf("change detected\n");
			pvGet(hi_res_mode);
			old_mode = hi_res_mode; 
		} state init
	}
}