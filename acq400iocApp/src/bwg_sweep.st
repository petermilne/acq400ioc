/* bwg_sweep.st : software sweep generator for CPSC2 BWG A, BWG B 4 channel arbs */

program bwg_sweep
option +r;
option +s;

#define STEP_MODE_LIN	0
#define STEP_MODE_LOG	1
#define MAXPOINTS	1024
#define DT_MIN		0.001



/* INPUTS */
int mode;	assign mode to "{uut}:BWG:{bank}:SWEEP:MODE"; 	/* OFF, SAW, RAMP */
monitor mode;



int step_mode;  assign step_mode to "{uut}:BWG:{bank}:SWEEP:STEPMODE";  /* LIN, LOG */
monitor step_mode;

double f1;      assign f1 to "{uut}:BWG:{bank}:SWEEP:F1";		/* Start Frequency */
monitor f1;
double f2;      assign f2 to "{uut}:BWG:{bank}:SWEEP:F2";		/* Stop Frequency */
monitor f2;

double period;	assign period to "{uut}:BWG:{bank}:SWEEP:PERIOD";	/* duration of sweep in s */
double dt;	assign dt to "{uut}:BWG:{bank}:SWEEP:DT";		/* tick time in s */

/* OUTPUT */
double fout;	assign fout to "{uut}:BWG:{bank}:NCO:FREQ";

int mode0;
int step_mode0;
double f1_0;
double f2_0;
double period_0;








%{

double fx[MAXPOINTS];
int nticks;
int tick;
int direction;

void make_lin_lut(double f1, double f2, int nticks)
{
	int ii;
	double step = (f2 - f1) / nticks;
	double fxx = f1;
	
	for (ii = 0; ii < nticks; ++ii, fxx += step){
		fx[ii] = fxx;
	}	
}
void make_log_lut(double f1, double f2, int nticks)
{
	/** @todo */
	make_lin_lut(f1, f2, nticks);
}
int make_lut(double f1, double f2, double period, int dt, int step_mode)
{	
	if ((nticks = period/dt) < MAXPOINTS){
		dt = period/MAXPOINTS;
	}else{
		while((nticks = period/dt) > MAXPOINTS){
			dt *= 2;
		}
	}
	
	if (step_mode == STEP_MODE_LIN){
		make_lin_lut(f1, f2, nticks);
	}else{
		make_log_lut(f1, f2, nticks);
	}
	tick = 0;
	direction = 1;
	return dt;
}

double update(int mode) 
{
	double xx = fx[tick];

	tick += direction;
	
	if (mode == 1){
		if (tick >= MAXPOINTS){
			tick = 0;
		}
	}else if (mode == 2){
		if (tick >= MAXPOINTS){
			direction = -1;
			tick = MAXPOINTS-1;
		}
		if (tick < 0){
			direction = 1;
			tick = 0;
		}
	}
	
	return xx; 
}
}%


ss bwg_sweep
{
	state init
	{
		entry {
			pvGet(mode); 		mode0 = mode;
			pvGet(step_mode); 	step_mode0 = step_mode;
			pvGet(f1); 		f1_0 = f1;
			pvGet(f2); 		f2_0 = f2;
			pvGet(period);		period_0 = period;
		}
		when(mode) {
			mode0 = mode;
			dt = make_lut(f1, f2, period, dt, step_mode);
			pvPut(dt);
		} state run		
	}
	
	state run
	{
		when (mode != mode0 || f1 != f1_0 || f2 != f2_0 || step_mode != step_mode0 ||period != period_0) {
			
		} state init
		
		when(delay(dt)){
			pvPut(fout, update(mode));
		} state run
	}
}