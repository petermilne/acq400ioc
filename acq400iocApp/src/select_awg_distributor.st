/* select_awg_distributor.st .. control AWG distributor setting */



program select_awg_distributor
option +r;

%% #include <string.h>

int mode; 		assign mode 		to "{UUT}:{SITE}:AWG:DIST";
monitor mode;

int mode0;

string sitelist; 	assign sitelist 	to "{UUT}:{SITE}:AWG:SITES";
string dclist;   	assign dclist 		to "{UUT}:{SITE}:AWG:SITES:n";
string distributor;   	assign distributor 	to "{UUT}:{SITE}:AWG:SITES:w";


int verbose;

ss selectAwgDistributor {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}	
			pvGet(dclist);
			pvGet(distributor);
			pvGet(mode);
		} state action
	}
	
	state idle {
		entry {
			if (verbose) printf("idle\n");
		}
		when(mode != mode0){
			
		} state action
	}
	
	state action {
		entry {
			if (mode == 0){
				strcpy(distributor, dclist); 
			}else{
				pvGet(sitelist);
				strcpy(distributor, sitelist);
			}
			pvPut(distributor);
			if (verbose) printf("action %s\n", distributor);
		}
		when() {
			mode0 = mode;
		} state idle
	}
}
