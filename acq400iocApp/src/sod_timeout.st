/* sod_timeout.stt .. activate free running clock on timeout after trigger..
 * flush through samples when not enough sod clocks */


program sodTimeout
option +r;

int triggered; assign triggered to "{uut}:1:TRIGGERED";
monitor triggered;

double timeout; assign timeout to "{uut}:1:SOD_TIMEOUT";


int clk_src; assign clk_src to "{uut}:0:SIG:SRC:CLK:0";

#define INT_01M	4
#define FLUSH 	2

int verbose;

ss sodTimeout {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
		} state idle
	}
	
	state idle {
		entry {
			if (verbose) printf("idle\n");
		}
		when(triggered){
			
		} state busy
	}
	
	state busy {
		entry {
			if (verbose) printf("busy\n");
		}		
		when(!triggered) {
			
		} state idle
		
		when(delay(timeout)){			 						
			
		} state flush
	}
	
	state flush {
		entry {
			clk_src = INT_01M; pvPut(clk_src);
			if (verbose) printf("flush\n");
		}
		when(!triggered || delay(FLUSH)){
			
		} state idle
	}
}
