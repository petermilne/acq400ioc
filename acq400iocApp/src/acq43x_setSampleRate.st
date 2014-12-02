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
double fin;	assign fin to "{uut}:0:SIG:CLK_EXT:FREQ";
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

double clk_mb_set; assign clk_mb_set to "{uut}:0:SIG:CLK_MB:SET";

int state_not_idle; assign state_not_idle to "{uut}:MODE:TRANS_ACT:STATE_NOT_IDLE";

int HS_MAX=50000;

double S01_min = 16000000;
double S01_max = 35000000;
 
int adc_mode0;

double Fs0;

monitor Fs; 
monitor fin;


ss rate_set
{
	state init {
		when() {
			printf("init\n");
			pvGet(adc_mode);
			adc_mode0 = adc_mode;
			pvGet(Fs);
			Fs0 = Fs;
		} state run	
	}
	
	state run {
		when(Fs != Fs0 && Fs != 0){
			printf("acq43x_setSampleRate change detect\n");
			if (Fs0 == 0){
				mclk_out = 0;
				pvPut(mclk_out, SYNC);
				adc_clk_src = 1;	/* EXT */
				pvPut(adc_clk_src, SYNC);
				adc_clk_src_dx = 1;	/* d1 : MCLK */
				pvPut(adc_clk_src_dx, SYNC);				
				printf("first time init done\n");
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
						
			printf("acq43x_setSampleRate set:%.0f mode:%s clk_mb_set:%.0f clkdiv:%d\n",
				Fs, adc_mode? "HR_512": "HS_256", 
				clk_mb_set, clkdiv);
				
		} state wait_fin
	}
			
	state wait_fin {
		when(fin < 1000000){
			printf("acq43x_setSampleRate error needs Fin > 1M, currently %.1f\n",
				fin);	
			Fs = Fs0;
			pvPut(Fs);
		} state run
		
		when(fin >= 1000000){
		
		} state action		
	}

	state action {
		when() {
			printf("action: mode:%d clkdiv:%d mb:%.0f\n", 
				adc_mode, clkdiv, clk_mb_set);
			pvPut(adc_mode);
			pvPut(clkdiv);
			pvPut(clk_mb_set);
			Fs0 = Fs;
		} state run
	} 
	
	state error {
		when() {
			printf("acq43x_setSampleRate: ERROR: unable to change ADC_MODE while running\n");
			Fs = Fs0;
			pvPut(Fs);
		} state run		
	} 
}