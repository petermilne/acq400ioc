/* sod_timeout.stt .. activate free running clock on timeout after trigger..
 * flush through samples when not enough sod clocks 
 * 
brute force flush .. use c, runs about 7kHz.
seq sodTimeout "uut=UUT,verbose=1,FLUSH_COUNT=N"
 * 
 * */


program sodTimeout
option +r;

int sample_count; assign sample_count to "{uut}:1:SIG:sample_count:COUNT";
monitor sample_count;

int sample_count_reset; assign sample_count_reset to "{uut}:1:SIG:sample_count:RESET";
double timeout; assign timeout to "{uut}:1:SOD_TIMEOUT";
monitor timeout;


int clk_src; assign clk_src to "{uut}:0:SIG:SRC:TRG:1";

int strig;  assign strig to "{uut}:SIG:SOFT_TRIGGER:w";

#define STRG	0
#define FLUSH_COUNT 10000

int verbose;


%{
#define O_WRONLY	00000001
int atoi(const char *nptr);
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

int flush_count = FLUSH_COUNT;
}%

ss sodTimeout {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			if (macValueGet("FLUSH_COUNT")){
				flush_count = atoi(macValueGet("FLUSH_COUNT"));
			}			
			sample_count_reset = 1; pvPut(sample_count_reset);
		} state idle
	}
	
	state idle {
		entry {
			if (verbose) printf("sodTimout::idle\n");
		}
		when(sample_count && timeout > 0){
			
		} state busy
	}
	
	state busy {
		entry {
			if (verbose) printf("sodTimout::busy\n");
		}		
		when(!sample_count) {
			
		} state idle
		
		when(delay(timeout)){			 						
			
		} state flush
	}
	
	state flush {
		entry {
			clk_src = STRG; pvPut(clk_src);
			if (verbose) printf("sodTimout::flush\n");
			soft_trigger(FLUSH_COUNT);
			sample_count_reset = 1; pvPut(sample_count_reset);
			if (verbose) printf("sodTimout::flush2\n");
			
		}
		
		when(){
			
		} state idle
	}
}
