/*************************************************************************\
acq43x_setSamplerate.st : set sample rate for ACQ43x

acq2006_014:SYS:CLK:FPMUX 	is enabled to give us some source CLK 1..100MHz
acq2006_014:0:SIG:SRC:CLK:0 	is set to EXT to give us a EXT CLK reading
acq2006_014:0:SIG:SRC:CLK:1	is to to MCLK to output on d1

INPUT: Fs

OUTPUTS:
acq2006_014:0:SIG:SRC:CLK:1 = MCLK
acq2006_014:0:SIG:CLK_MB:SET
acq2006_014:1:ADC_MODE
acq2006_014:1:CLKDIV


ALGORITHM:

Fs < 50 : MODE=HI_RES_512, else MODE=HI_SPEED_2
\*************************************************************************/
program acq43x_setSampleRate
option +r;

double Fs;	assign Fs  to "{uut}:1:ACQ43X_SAMPLE_RATE";
double fin;	assign fin to "{uut}:{fin}";
double Fm;	assign Fm  to "{uut}:0:SIG:CLK_MB:FREQ";

double Fmod;
double Fmbclk;	

int adc_mode;	assign adc_mode to "{uut}:1:ADC_MODE";
int clkdiv;	assign clkdiv 	to "{uut}:1:CLKDIV";
int fp_mux;	assign fp_mux 	to "{uut}:SYS:CLK:FPMUX";
int src_clk;	assign src_clk 	to "{uut}:0:SIG:SRC:CLK:0";
int mclk_out;	assign mclk_out to "{uut}:0:SIG:SRC:CLK:1";

int adc_clk_src; 	assign adc_clk_src to "{uut}:1:CLK";
int adc_clk_src_dx; 	assign adc_clk_src_dx to "{uut}:1:CLK:DX";

string info;	assign info to "{uut}:1:ACQ43X_SAMPLE_RATE:INFO";
string srcname; assign srcname to "{uut}:1:ACQ43X_SRCNAME";
string srcfreq; assign srcfreq to "{uut}:1:ACQ43X_SRCFREQ";

double clk_mb_set; assign clk_mb_set to "{uut}:0:SIG:CLK_MB:SET";

int state_not_idle; assign state_not_idle to "{uut}:MODE:TRANS_ACT:STATE_NOT_IDLE";

double clk_mb_fin;  assign clk_mb_fin to "{uut}:0:SIG:CLK_MB:FIN";

int HS_MAX=50000;

double S01_min = 18000000;
double S01_max = 37000000;
 
int adc_mode0;

double Fs0;

monitor Fs; 
monitor fin;
monitor fp_mux;

int retry;
int max_retry = 5;
int Fmin = 1000000;

char *uut;

ss rate_set
{
	state init {
		when() {
			uut = macValueGet("uut");			
			if (strncmp(uut, "acq2106", 6) == 0){
				S01_min = 1000000;
			}
			printf("init model: %s  S01_min %.0f\n", uut, S01_min);
			pvGet(adc_mode);
			adc_mode0 = adc_mode;
			pvGet(Fs);
			Fs0 = Fs;
			sprintf(srcname, "MBCLK");
			pvPut(srcname);
		} state run	
	}
	
	state run {
		when(Fs != Fs0 && Fs != 0){
			printf("acq43x_setSampleRate change detect\n");
			pvGet(mclk_out);
			pvGet(adc_clk_src);
			pvGet(adc_clk_src_dx);
			
			if (Fs0 == 0 || mclk_out != 0){
				mclk_out = 0;
				pvPut(mclk_out, SYNC);
			}
			if (Fs0 == 0 || adc_clk_src != 1){
				adc_clk_src = 1;	/* EXT */
				pvPut(adc_clk_src, SYNC);			
			}
			if (Fs0 == 0 || adc_clk_src_dx != 1){
				adc_clk_src_dx = 1;	/* d1 : MCLK */
				pvPut(adc_clk_src_dx, SYNC);								
			}
			printf("run %.0f != %.0f\n", Fs, Fs0);						
			
			adc_mode = Fs < HS_MAX;
			if (adc_mode != adc_mode0){
				pvGet(state_not_idle);
				if (state_not_idle) {
					state error;
				} 
				printf("pvPut adc_mode :%d\n", adc_mode);
				pvPut(adc_mode);
				adc_mode0 = adc_mode;
			}
			Fmod = Fs * (adc_mode? 512: 256);
			printf("Fmod:%.0f adc_mode:%d\n", Fmod, adc_mode);
			for (clkdiv = 1; (clk_mb_set = Fmod * clkdiv) < S01_min; 
					++clkdiv){
				;				
			}						
			printf("fp_mux:%d\n", fp_mux);
		} state check_mux
	}
	
	state check_mux {
		when(delay(1) && fp_mux == 0 && retry < max_retry) {
			snprintf(info, 40, "ERROR: FPMUX not set");
			pvPut(info);
			printf("acq43x_setSampleRate %s\n", info);
			++retry;
		} state check_mux
		
		when (fp_mux == 0 && retry >= max_retry){
			snprintf(info, 40, "set default FPMUX");
			pvPut(info);
			printf("acq43x_setSampleRate %s\n", info);
			fp_mux = 3;	/* ZCLK */
			pvPut(fp_mux);
		} state check_mux
		
		when(fp_mux != 0){
			retry = 0;
		} state wait_fin
	}
			
	state wait_fin {
		when(fin < Fmin && ++retry < max_retry){
			snprintf(info, 40, 
				"WARNING needs Fin > %d, currently %.1f retry%d/%d", Fmin, fin, retry, max_retry);
			pvPut(info);
			printf("acq43x_setSampleRate %s\n", info);
		} state wait_fin_retry
		
		when(fin < Fmin){
			snprintf(info, 40, 
				"ERROR needs Fin > %d, currently %.1f", Fmin, fin);
			pvPut(info);
			printf("acq43x_setSampleRate %s\n", info);
			Fs = Fs0;
			pvPut(Fs);
		} state run
		
		when(fin >= 1000000){
			
		} state have_clock		
	}
	
	state wait_fin_retry {
		when(delay(1)){
		
		} state wait_fin
	}

	state have_clock {
		when(fp_mux == 1){		/* XCLK */
			if (fin > 32000000 && fin < 34000000) {
				clk_mb_fin = 32768000;
			}else if (fin > 48000000 && fin < 50000000){
				clk_mb_fin = 49157000;
			}else{
				printf("WARNING: going with non standard XCLK %.1f\n", clk_mb_fin);
			}
		} state action
		
		when(fp_mux == 2){ 	/* FPCLK */
			printf("pvGet clk_mb_fin=%.1f\n", clk_mb_fin);
			pvGet(clk_mb_fin);
			printf("going with FPCLK=%.1f\n", clk_mb_fin);
		} state action
		
		when (fp_mux == 3){ 	/* ZCLK */
			if (fin > 33000000 && fin < 34000000){
				clk_mb_fin = 33333000;
			}else{
				clk_mb_fin = fin;
				printf("WARNING: strange ZCLK %.1f\n", clk_mb_fin);
			}
		} state action
	}
	state action {
		when() {
			snprintf(info, 40,
				"set:%.0f mode:%s clk_mb_set:%.0f clkdiv:%d",
				Fs, adc_mode? "HR_512": "HS_256", 
				clk_mb_set, clkdiv);
			pvPut(info);
			printf("acq43x_setSampleRate %s\n", info);
				
			pvPut(adc_mode);
			pvPut(clkdiv);
			pvPut(clk_mb_fin, SYNC);
			pvPut(clk_mb_set);
			sprintf(srcfreq, "%.0e", clk_mb_set);
			pvPut(srcfreq);	
			Fs0 = Fs;
		} state run
	} 
	
	state error {
		when() {
			snprintf(info, 40, 
				"ERROR: unable to change ADC_MODE while running");
			pvPut(info);
			printf("acq43x_setSampleRate: %s\n", info);
			Fs = Fs0;
			pvPut(Fs);
		} state run		
	} 
}