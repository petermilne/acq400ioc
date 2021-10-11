/* pg_trg_debounce.st
 * decide whether to set debounce bypass or not
 * */

program pg_trg_debounce
option +r;

int mode; assign mode to "{uut}:{site}:BYPASS_TRG_DEBOUNCE:MODE";
monitor mode;

int bypass_debounce; assign bypass_debounce to "{uut}:{site}:BYPASS_TRG_DEBOUNCE";

#define MODE_BP_AUTO 0
#define MODE_BP_ON   1
#define MODE_BP_OFF  2



#define STRG	0

#define FLUSH_PER_CALL	500

int verbose;
int sample_count_count0;

int mode0 = 99;
int site;

ss pg_trg_deb {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			site = macValueGet("site")[0] - '0';
		} state idle0
	}
	
	state idle0 {
		entry {
			
		}
		when(mode == MODE_BP_AUTO && mode0 != mode){
			
		} state auto_select
		
		when(mode == MODE_BP_ON && mode0 != mode){
			
		} state manual_on
		
		when(mode == MODE_BP_OFF && mode0 != mode){
			
		} state manual_off	
		exit {
			mode0 = mode;
		}
	}
	
	state manual_on {
		entry {
			bypass_debounce = 1;
			pvPut(bypass_debounce);
			if (verbose){
				printf("pg_trg_debounce %d manual_on\n", site);
			}
		}
		when() {
			
		} state idle0
	}
	
	state manual_off {
		entry {
			bypass_debounce = 0;
			pvPut(bypass_debounce);
			if (verbose){
				printf("pg_trg_debounce %d manual_off\n", site);
			}
		}
		when() {
			
		} state idle0
	}
	
	state auto_select {
		entry {
			if (verbose){
				printf("pg_trg_debounce %d auto_select\n", site);
			}
		}
		when() {
			
		} state idle0
	} 
}
