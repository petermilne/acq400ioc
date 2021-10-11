/* pg_trg_debounce.st
 * decide whether to set debounce bypass or not
 * */

program pg_trg_debounce
option +r;

int mode; assign mode to "{uut}:{site}:BYPASS_TRG_DEBOUNCE:MODE";
monitor mode;

#define MODE_BP_AUTO 0
#define MODE_BP_ON   1
#define MODE_BP_OFF  2



#define STRG	0

#define FLUSH_PER_CALL	500

int verbose;
int sample_count_count0;

ss pg_trg_deb {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}			
		} state idle0
	}
	
	state idle0 {
		entry {
			
		}
		when(mode == MODE_BP_AUTO){
			
		} state auto_select
		
		when(mode == MODE_BP_ON){
			
		} state manual_on
		
		when(mode == MODE_BP_OFF){
			
		} state manual_off	
	}
	
	state manual_on {
		entry {
			
		}
		when() {
			
		} state idle0
	}
	
	state manual_off {
		entry {
			
		}
		when() {
			
		} state idle0
	}
	
	state auto_select {
		when() {
			
		} state idle0
	} 
}
