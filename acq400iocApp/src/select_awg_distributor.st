/* select_awg_distributor.st .. control AWG distributor setting */



program select_awg_distributor
option +r;

%% #include <string.h>

int mode; 		assign mode 		to "{UUT}:{SITE}:AWG:DIST";
monitor mode;

int mode0;

string sitelist; 	assign sitelist 	to "{UUT}:{SITE}:AWG:SITES";
monitor sitelist;

string sitelist0;

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
			pvGet(sitelist);
			strcpy(sitelist0, sitelist);
		} state action
	}
	
	state idle {
		entry {
			if (verbose) printf("idle\n");
		}
		
		when(strcmp(sitelist, sitelist0)){
		
		} state sitelist_change
		
		when(mode != mode0){
			
		} state action
	}
	
	state sitelist_change {
		entry {
			if (verbose) printf("sitelist_change %s -> %s\n", sitelist0, sitelist);
			strcpy(sitelist0, sitelist);
			if (strcmp(sitelist, "none") == 0){
				if (mode != 0){
					mode0 = mode = 0;
					pvPut(mode);
				}
			}else{
				if (mode == 0){
					mode0 = mode = 1;
					pvPut(mode);
				}
			}
		}
		when() {
			
		} state idle
		
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
