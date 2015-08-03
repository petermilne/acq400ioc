/*************************************************************************\
autoRepeat.st : repeat transient on termination
\*************************************************************************/
program autoRepeat
option +r;

int tstate;
assign tstate to "{uut}:MODE:TRANS_ACT:STATE";
monitor tstate;

int repeat;
assign repeat to "{uut}:MODE:TRANSIENT:REPEAT";
monitor repeat;

int set_arm;
assign set_arm to "{uut}:MODE:TRANSIENT:SET_ARM";

int repeat_delay_ms;
assign repeat_delay_ms to "{uut}:MODE:TRANSIENT:DELAYMS";

double repeat_delay;

int set_abort;
assign set_abort to "{uut}:MODE:TRANSIENT:SET_ABORT";
monitor set_abort;

int shot_complete;
assign shot_complete to "{uut}:SHOT_COMPLETE";
monitor shot_complete;

int shot_complete0;

int continuous;
assign continuous to "{uut}:MODE:CONTINUOUS";
monitor continuous;



ss autorepeat
{
	state stop
	{
		when(tstate != 0){
			printf("stop=>run\n");
		} state run
	}

	state run
	{
		when(set_abort > 0){
			set_abort = 0;
			pvPut(set_abort);
			printf("run=>stop_transition ABORT\n");
		} state stop_transition
		
		when(tstate == 0){
			printf("run=>run_transition\n");	
		} state run_transition
	}
	
	state stop_transition 
	{
		when(tstate == 0){
			printf("stop_transition=>stop\n");
		} state stop
	}
	
	state run_transition
	{
		when(repeat < 0){
			printf("run_transition=>rearm autorepeat forever\n");
		} state rearm
				
		when(repeat > 1){
			repeat -= 1;	
			pvPut(repeat);
			printf("run_transition=>rearm autorepeat %d\n", repeat);
		} state rearm
		
		when (repeat == 1){
			repeat = 0;
			pvPut(repeat);
			printf("run_transition=>stop autorepeat finished\n");
		} state stop
		
		when (repeat == 0){
			printf("run_transition=>stop autorepeat 0\n");
		} state stop
	}
	
	state rearm
	{
		entry {
      			pvGet(repeat_delay_ms);
      			repeat_delay = repeat_delay_ms/1000;
      			pvGet(shot_complete);
      			shot_complete0 = shot_complete;
    		}
		when(set_abort > 0){
			set_abort = 0;
			pvPut(set_abort);
			printf("run=>stop_transition ABORT\n");
		} state stop_transition
		    		
		when (delay(repeat_delay)) {
			printf("rearm=>waitrun\n");
			pvPut(set_arm);
		} state waitrun
	}
	
	state waitrun 
	{
		when(shot_complete != shot_complete0){
			printf("wait_run->run_transition\n");
		} state run_transition
		
		when(tstate != 0){
			printf("wait_run=>run\n");
		} state run
	}
}


/* avoid spurious restart last transient repeat when continuous is stopped */


ss monitor_continuous 
{
	state mc1
	{
		when(continuous > 0){
			pvGet(repeat);
			if (repeat){
				printf("continuous start detected, set repeat to zero to false restart\n");
				repeat = 0;
				pvPut(repeat);
			}
		} state mc2
	}
	
	state mc2
	{
		when(continuous == 0){
			;
		} state mc1
	}
}