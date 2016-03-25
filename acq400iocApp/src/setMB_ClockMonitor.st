/* setting MB clock doesn't always work. So detect change requests and make sure
 * it comes right
 */
 
 


program setMB_ClockMonitor
option +r;


double Fs;	assign Fs to "{uut}:0:SIG:CLK_MB:SET";
double Fm; 	assign Fm to "{uut}:0:SIG:CLK_MB:FREQ";
int is_ready;	assign is_ready to "{uut}:0:SIG:CLK_MB:READY";

monitor Fs;
monitor Fm;

double Fs0;
double Fm0;

double TH = 0.05;

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
			printf("setMB_ClockMonitor th:%.2f\n", TH);
			pvGet(Fs);
			Fs0 = Fs;
			is_ready = 0; pvPut(is_ready);
		} state wait_actual	
	} 
		
	state wait_actual {
		when (delay(1)){
			;
		} state monitor_actual
	}
	
	state monitor_actual {
		when(Fs > 0 && Fs != Fs0 ){
			Fs0 = Fs;
			pollcat = 0;
		} state wait_actual
			
		when(Fs > 0 && Fm != Fm0 && in_bounds(Fm, Fs, TH)){
			Fm0 = Fm;
			is_ready = 1; pvPut(is_ready);
		} state wait_actual 
		
		when(Fs > 0 && Fm != Fm0 && !in_bounds(Fm, Fs, TH)){
			if (++pollcat > 4){
				is_ready = 0; pvPut(is_ready);
				Fm0 = Fm;
				printf("WARNING: MB CLK Fs:%.2f != Fm:%.2f retry\n",
					Fs, Fm);			
				pvPut(Fs);
				pollcat = 0;
			}
		} state wait_actual
	}
}