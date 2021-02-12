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

#define STRG	0

int verbose;


%{
#define O_WRONLY	00000001
int open(const char *pathname, int flags);
int write(int fd, const void *buf, unsigned count);
int close(int fd);

void _soft_trigger(char* enable)
{
	int fd = open("/dev/acq400.0.knobs/soft_trig", O_WRONLY);
	write(fd, enable, 2);
	close(fd);

}
void soft_trigger(int count)
{
	while(count--){
		_soft_trigger("1\n");
		_soft_trigger("0\n");
	}
}


}%

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
			
		} state flush
		
		when(delay(1)){			 						
			time_countdown -= 1;
			pvPut(time_countdown);
		} state busy
	}
	
	state flush {
		entry {
			clk_src = STRG; pvPut(clk_src);
			sprintf(state_str, "sodTimeout::flush"); pvPut(state_str);			
			if (verbose) printf("%s\n", state_str);
			soft_trigger(flush_count);			
		}
		
		when(delay(2)){
			
		} state flush2
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
