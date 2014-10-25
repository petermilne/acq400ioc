/*************************************************************************\
autoRepeat.st : repeat transient on termination
\*************************************************************************/
program autorepeat
option +r;
float v;
//assign v to "{user}:xxxExample";
//assign v to "acq1001_048:1:AI:CH:12";
assign v to "{host}:{site}:AI:CH:{ch}";
monitor v;

int tstate;
assign tstate to "{uut}:MODE:TRANS_ACT:STATE";
monitor tstate;

int repeat;
assign repeat to "{uut}:MODE:TRANSIENT:REPEAT";
monitor repeat;

int set_arm;
assign set_arm to "{uut}:MODE:TRANSIENT:SET_ARM";

int repeat_delay_ms;
assign repeat_delay_ms to "${uut}:MODE:TRANSIENT:DELAYMS";

double repeat_delay;

ss autorepeat
{
	state stop
	{
		when(tstate != 0){
			printf("running again\n");
		} state run
	}

	state run
	{
		when(tstate == 0){
			printf("transition to zero\n");	
		} state run_transition
	}
	
	state run_transition
	{
		when(repeat < 0){
			printf("autorepeat forever\n");
		} state rearm
		
		
		when(repeat > 1){
			repeat -= 1;	
			pvPut(repeat);
			printf("autorepeat %d\n", repeat);
		} state rearm
		
		when (repeat == 1){
			repeat = 0;
			pvPut(repeat);
			printf("autorepeat finished\n");
		} state stop
		
		when (repeat == 0){
			printf("autorepeat 0\n");
		} state stop
	}
	
	state rearm
	{
		entry {
      			pvGet(repeat_delay_ms);
      			repeat_delay = repeat_delay_ms/1000;
    		}
		when (delay(repeat_delay)) {
			printf("rearm\n");
			pvPut(set_arm);
		} state run
	}
}
