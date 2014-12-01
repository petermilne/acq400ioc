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

double Fs;	assign Fs  to "{uut}:1:ACQ43x_SAMPLE_RATE";
double fin;	assign fin to "{uut}:0:SIG:CLK_EXT:FREQ";
double Fm;	assign Fm  to "{uut}:0:SIG:CLK_MB:FREQ";

double Fmod;
double Fmbclk;	

int adc_mode;	assign adc_mode to "{uut}:1:ADC_MODE";
int clkdiv;	assign clkdiv 	to "{uut}:1:CLKDIV";
int fp_mux;	assign fp_mux 	to "{uut}:SYS:CLK:FPMUX";
int src_clk;	assign src_clk 	to "{uut}:0:SIG:SRC:CLK:0";
int mclk_out;	assign mclk_out to "{uut}:0:SIG:SRC:CLK:1";

double clk_mb_set; assign clk_mb_set to "{uut}:0:SIG:CLK_MB:SET";

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
			pvGet(adc_mode);
			adc_mode0 = adc_mode;
			pvGet(Fs);
			Fs0 = Fs;
			mclk_out = 0;
			pvPut(mclk_out);
		} state run	
	}
	
	state run 
	{
		when(Fs != Fs0){
			Fs0 = Fs;
			adc_mode = Fs < HS_MAX;
			if (adc_mode != adc_mode0){
				pvPut(adc_mode);
				adc_mode0 = adc_mode;
			}
			Fmod = Fs * (adc_mode? 512: 256);
			for (clkdiv = 1; (clk_mb_set = Fmod * clkdiv) < S01_min; 
					++clkdiv){
			}		
		} state wait_fin
	}
			
	state wait_fin {
		when(fin < 1000000){
			printf("acq43x_setSamplerate %f wait > 1000000\n", fin);	
		} state action
	}

	state action {
		when() {
			pvPut(adc_mode);
			pvPut(clkdiv);
			pvPut(clk_mb_set);
		} state run
	} 
}