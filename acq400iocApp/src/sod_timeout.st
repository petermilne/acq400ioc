/* sod_timeout.stt .. activate free running clock on timeout after trigger..
 * flush through samples when not enough sod clocks 
 * seq sodTimeout "uut=UUT,verbose=1,FLUSH_COUNT=N"
 * 
 * 
 * sample_count : proxy for TRIGGERED (TRIGGERED is ARM in SOD Mode)
 * 
 * 
 * wait(TRIGGERED)
 * 
 * delay(SOD_TIMEOUT)
 * 
 * flush(FLUSH_COUNT)
 * ... reset sample_count to avoid an endless loop 
 * */


program sodTimeout
option +r;

int sample_count_active; assign sample_count_active to "{uut}:1:SIG:sample_count:ACTIVE";
monitor sample_count_active;

int sample_count_count; assign sample_count_count to "{uut}:1:SIG:sample_count:COUNT";
monitor sample_count_count;

int transient_state; assign transient_state to "{uut}:MODE:TRANS_ACT:STATE";
monitor transient_state;

int sc_reset; assign sc_reset to "{uut}:1:SIG:sample_count:RESET";
double timeout; assign timeout to "{uut}:1:SOD_TIMEOUT:TIMEOUT";
monitor timeout;

int time_countdown; assign time_countdown to "{uut}:1:SOD_TIMEOUT:COUNTDOWN";


int clk_src; assign clk_src to "{uut}:0:SIG:SRC:TRG:1";

int flush_count; assign flush_count to "{uut}:1:SOD_TIMEOUT:FLUSH_COUNT";

string state_str; assign state_str to "{uut}:1:SOD_TIMEOUT:STATE";

int soft_trigger; assign soft_trigger to "{uut}:SIG:SOFT_TRIGGER:w";

#define STRG	0

#define FLUSH_PER_CALL	500

int verbose;
int sample_count_count0;

ss sodTimeout {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}			
		} state idle0
	}
	
	state idle0 {
		entry {
			sc_reset = 1; pvPut(sc_reset);
			pvGet(flush_count);
			time_countdown = 0; pvPut(time_countdown);
						
			sprintf(state_str, "sodTimeout::idle"); pvPut(state_str); 
			if (verbose) printf("%s\n", state_str);
		}
		when(delay(1)){
			
		} state idle1
	}
		
	state idle1 {
		when(sample_count_active && sample_count_count >= 1 && timeout > 0){
			time_countdown = timeout; pvPut(time_countdown);
			
			if (verbose) printf("sodTimeout::busy sample_count_active:%d\n", sample_count_active);
		} state busy
	}
	
	state busy {
		entry {	
			sprintf(state_str, "sodTimeout::busy:%d", time_countdown); pvPut(state_str);			
			if (verbose) printf("%s\n", state_str);
		}		
		when(!sample_count_active && transient_state == 0) {
			
		} state idle0
		
		when(time_countdown <= 0){
			clk_src = STRG; pvPut(clk_src);
			pvGet(sample_count_count);
			sample_count_count0 = sample_count_count;
			sprintf(state_str, "sodTimeout::flush"); pvPut(state_str);
		} state flush
		
		when(delay(1)){			 						
			time_countdown -= 1;
			pvPut(time_countdown);
		} state busy
	}
	
	state flush {
		option -e; /* Do entry{} every time ... */
		entry {			
			//if (verbose) printf("%s\n", state_str);
			soft_trigger = FLUSH_PER_CALL; pvPut(soft_trigger);
		}
			
		when(transient_state == 0 || transient_state > 3) {
			
		} state idle0
		
		when(sample_count_count - sample_count_count0 > flush_count){
			
		} state flush2
		
		when(delay(0.02)){
			
		} state flush		
	}
	
	state flush2 {
		entry {
			sprintf(state_str, "sodTimeout::flush2"); pvPut(state_str);			
			if (verbose) printf("%s\n", state_str);
		}
		when(delay(1) && (transient_state == 0 || transient_state > 3)){
			
		} state idle0
		
		when(delay(3) && transient_state > 1 && transient_state <= 3){
			
		} state flush
	}
}
