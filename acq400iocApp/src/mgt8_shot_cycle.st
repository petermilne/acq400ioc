/*
 * mgt8_shot_cycle.st
 * User presses ARM ARM=1
 * Clear Counters, Set buffers, then
 * SET_ARM:w
 * Then wait STATE==IDLE
 * Then clear ARM
 * 
 * Also ABORT clears ARM
 */

program mgt8_shot_cycle
option +r;
option +s;

int set_arm;	assign set_arm 		to "{uut}:MODE:BLT:SET_ARM"; 	monitor set_arm;
int set_arm_w;	assign set_arm_w 	to "{uut}:MODE:BLT:SET_ARM:w";
int act_state; 	assign act_state 	to "{uut}:MODE:BLT:ACT:STATE"; monitor act_state;
int set_abort; 	assign set_abort 	to "{uut}:MODE:BLT:SET_ABORT"; monitor set_abort;
int set_nbuf; 	assign set_nbuf 	to "{uut}:MODE:BLT:BUFFERS";

#define NCTR	4
int ctr_resets[NCTR];
assign ctr_resets to {
	"{uut}:A:SIG:MGT:PUSH:BUFS:RESET",
	"{uut}:A:SIG:MGT:PUSH:BYTES:RESET",
	"{uut}:A:SIG:MGT:PULL:BUFS:RESET",
	"{uut}:A:SIG:MGT:PULL:BYTES:RESET"
};

int ii;
char* debug;

ss mgt8_shot_cycle {
	state init {
		when() {
			debug = macValueGet("debug");
			if (debug && debug[0] == '\0') debug = 0;
		} state wait_arm_demand
	}
	state wait_arm_demand {
		entry {
			set_arm = 0; pvPut(set_arm);
		}
		when(set_arm){
			if (debug) printf("%s => %s\n", "wait_arm_demand", "pre_arm");
		} state pre_arm
	}
	state pre_arm {
		entry {		
			for (ii = 0; ii < NCTR; ++ii){
				ctr_resets[ii] = 1; pvPut(ctr_resets[ii]); // self-clears
			}		
			pvGet(set_nbuf); pvPut(set_nbuf);
		}
		when(delay(0.25)){
			if (debug) printf("%s => %s\n", "pre_arm", "arm_action");
		} state arm_action
	}
	state arm_action {
		entry {

			set_arm_w = 1; pvPut(set_arm_w);
		}
		when(act_state != 0){
			if (debug) printf("%s => %s\n", "arm_action", "wait_complete");
		} state wait_complete
		when(set_abort){
			if (debug) printf("%s => %s\n", "arm_action", "wait_arm_demand");
		} state wait_arm_demand
	}
	state wait_complete {
		when(act_state == 0){
			if (debug) printf("%s => %s\n", "wait_complete", "wait_arm_demand");
		} state wait_arm_demand
		when(set_abort){
			if (debug) printf("%s => %s\n", "wait_complete", "wait_arm_demand");
		} state wait_arm_demand		
	}
}
