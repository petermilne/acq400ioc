/*************************************************************************\
auto_soft_trigger.stt : automatic soft trigger.
\*************************************************************************/
program autoSoftTrigger
option +r;

double delay_s;
assign delay_s to "{uut}:AUTO:SOFT_TRIGGER:DELAY";

int count;
assign count to "{uut}:AUTO:SOFT_TRIGGER:COUNT";

int enable;
assign enable to "{uut}:AUTO:SOFT_TRIGGER:EN";
monitor enable;



int trigger;
assign trigger to "{uut}:SIG:SOFT_TRIGGER:w";


ss autoSoftTrigger
{
	state init
	{
		when(enable != 0){
			pvGet(delay_s);
			pvGet(count);
		} state run
	}
	
	state run
	{
		when(enable == 0 || (count && --count == 0)){
			enable = 0; pvPut(count);
		} state init
		
		when(delay(delay_s)){
			trigger = !trigger; pvPut(trigger);
		} state run
	}
}
