/* select_awg_distributor.st .. control AWG distributor setting */



program select_awg_distributor
option +r;

%% #include <string.h>

int mode; assign mode to "{uut}:1:AWG:DIST";
monitor mode;

int mode0;

string sitelist; 	assign sitelist to "{uut}:1:AWG:SITES";
string dclist;   	assign dclist to "{uut}:1:AWG:SITES:n";
string distributor;   	assign distributor to "{uut}:1:AWG:SITES:w";


int verbose;

ss selectAwgDistributor {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			pvGet(sitelist);
			pvGet(dclist);
			pvGet(distributor);
		} state idle
	}
	
	state idle {
		entry {
			if (verbose) printf("idle\n");
		}
		when(mode != mode0){
			mode0 = mode;
		} state action
	}
	
	state action {
		entry {
			if (mode == 0){
				strcpy(distributor, dclist); 
			}else{
				strcpy(distributor, sitelist);
			}
			pvPut(distributor);
			if (verbose) printf("action %s\n", distributor);
		}
		when() {
			
		} state idle
	}
}
