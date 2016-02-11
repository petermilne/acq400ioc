/* setting MB clock doesn't always work. So detect change requests and make sure
 * it comes right
 */
 
 


program setMB_ClockMonitor
option +r;


double Fs;	assign Fs to "{uut}:0:SIG:CLK_MB:SET";
double Fm; 	assign Fm to "{uut}:0:SIG:CLK_MB:FREQ";

monitor Fs;
monitor Fm;

double Fs0;
double Fm0;


%{
#include <math.h>
static int in_bounds(double x1, double x2, double th)
{
	if (x2 == 0){
		return 0;
	}
	return fabs(x1-x2)/x2 < th; 
}	
}%

int pollcat;

ss rate_set 
{
	state init {
		when() {
			pvGet(Fs);
			Fs0 = Fs;
		} state wait_change	
	} 
	
	state wait_change {
		when(Fs > 0 && Fs != Fs0 ){
			Fs0 = Fs;
			pollcat = 0;
		} state wait_actual			
	}
	
	state wait_actual {
		when (delay(1)){
			;
		} state monitor_actual
	}
	
	state monitor_actual {
		when(Fm != Fm0 && in_bounds(Fm0, Fs, 0.05)){
			Fm0 = Fm;
		} state wait_change 
		
		when(Fm != Fm0 && ++pollcat > 4){
			Fm0 = Fm;
			printf("WARNING: MB CLK Fs:%f != Fm :%f retry\n",
				Fs, Fm);			
			pvPut(Fs);
		} state wait_actual
	}
}