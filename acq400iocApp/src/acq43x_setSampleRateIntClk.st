/*************************************************************************\
acq43x_setSampleRateIntClk.st : set sample rate for ACQ43x where no ZCLK


INPUT: Fs
100MHz 

OUTPUTS:

acq2006_014:1:ADC_MODE
acq2006_014:1:CLKDIV


ALGORITHM:

Fs < 50 : MODE=HI_RES_512, else MODE=HI_SPEED_2
\*************************************************************************/
program acq43x_setSampleRateIntClk
option +r;

double Fs;	assign Fs  to "{uut}:1:ACQ43X_SAMPLE_RATE";
double fin = 	100e6;

double Fmod;


int adc_mode;	assign adc_mode to "{uut}:1:ADC_MODE";
int clkdiv;	assign clkdiv 	to "{uut}:1:CLKDIV";

int adc_clk_src; 	assign adc_clk_src to "{uut}:1:CLK";


string info;	assign info to "{uut}:1:ACQ43X_SAMPLE_RATE:INFO";
string srcname; assign srcname to "{uut}:1:ACQ43X_SRCNAME";
string srcfreq; assign srcfreq to "{uut}:1:ACQ43X_SRCFREQ";

int state_not_idle; assign state_not_idle to "{uut}:MODE:TRANS_ACT:STATE_NOT_IDLE";

int HS_MAX=50000;

 
int adc_mode0;

double Fs0;

monitor Fs; 



ss rate_set
{
	state init {
		when() {
			printf("init select internal clock\n");
			adc_clk_src = 0;
			pvPut(adc_clk_src);
			pvGet(adc_mode);
			adc_mode0 = adc_mode;
			pvGet(Fs);
			Fs0 = Fs;
			sprintf(srcname, "INTCLK");
			pvPut(srcname);
			sprintf(srcfreq, "%.0e", fin);
			pvPut(srcfreq);
		} state run	
	}
	
	state run {
		when(Fs != Fs0 && Fs != 0){
			printf("acq43x_setSampleRateIntClk change detect\n");			
			pvGet(adc_clk_src);		
			
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
			printf("Fmod:%.0f adc_mode:%d fin:%f\n", Fmod, adc_mode, fin);

			for (clkdiv = 1; Fmod * clkdiv <= fin; ++clkdiv){
				;				
			}
			printf("Fmod:%.0f adc_mode:%d clkdiv set %d\n", 
							Fmod, adc_mode, clkdiv);
			pvPut(clkdiv);
			snprintf(info, 40,
				"set:%.0f %s fin:%.0e clkdiv:%d",
				Fs, adc_mode? "HR_512": "HS_256", 
				fin, clkdiv);
			pvPut(info);
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