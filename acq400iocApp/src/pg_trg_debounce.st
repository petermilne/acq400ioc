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

int bypass_debounce; assign bypass_debounce to "{uut}:{site}:BYPASS_TRG_DEBOUNCE";

int trg_dx; assign trg_dx to "{uut}:{site}:TRG:DX";
monitor trg_dx;

int trg_d0_src; assign trg_d0_src to "{uut}:0:SIG:SRC:TRG:0";
monitor trg_d0_src;

#define TRG_D0_WRTT0 5

int trg_d1_src; assign trg_d1_src to "{uut}:0:SIG:SRC:TRG:1";
monitor trg_d1_src;

#define TRG_D1_WRTT1 6

#define TRGIN(site)	((site)+1)

int verbose;

/* monitor, previous state */
int trg_dx0 = 99;
int trg_d0_src0 = 99;
int trg_d1_src0 = 99;


int site;
char* message;

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
			if (verbose){
				printf("pg_trg_debounce %d idle0\n", site);
			}
		}
		when(mode == MODE_BP_AUTO){
			
		} state auto_select
		
		when(mode == MODE_BP_ON){
			
		} state manual_on
		
		when(mode == MODE_BP_OFF ){
			
		} state manual_off		
	}
	
	state manual_on {
		entry {
			bypass_debounce = 1;
			pvPut(bypass_debounce);
			if (verbose){
				printf("pg_trg_debounce %d manual_on\n", site);
			}
		}
		when(mode != MODE_BP_ON) {
			
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
		when(mode != MODE_BP_OFF) {
			
		} state idle0
	}
	
	state auto_select {
		entry {
			if (verbose){
				printf("pg_trg_debounce %d auto_select\n", site);
			}
		}
		when(mode != MODE_BP_AUTO){
			
		} state idle0
		when(trg_dx != trg_dx0 || (trg_dx==0 && trg_d0_src != trg_d0_src0) || (trg_dx==1 && trg_d1_src != trg_d1_src0)) {
			if (trg_dx == TRGIN(site)){
				message = "TRGIN";
				bypass_debounce = 0;
			}else if (trg_dx == 0){
				if (trg_d0_src == TRG_D0_WRTT0){
					message = "d0+WRTT0";
					bypass_debounce = 1;
				}else{
					message = "d0";
					bypass_debounce = 0;
				}
			}else if (trg_dx == 1){
				if (trg_d1_src == TRG_D1_WRTT1){
					message = "d1+WRTT1";
					bypass_debounce = 1;
				}else{
					message = "d1";
					bypass_debounce = 0;
				}
			}else{
				message = "MOD";
				bypass_debounce = 1;
			}
			pvPut(bypass_debounce);
			if (verbose) printf("pg_trg_debounce condition:%s bypass:%d\n", message, bypass_debounce);
			
		} state idle0
		
		exit {
			trg_dx0 = trg_dx;
			trg_d0_src0 = trg_d0_src;
			trg_d1_src0 = trg_d1_src;
		}
	} 
}
