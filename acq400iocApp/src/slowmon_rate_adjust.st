/* slowmon_rate_adjust.st
 * Set sets desired slowmon output rate.
 * */

program slowmon_rate_adjust
option +r;

string sync_role; assign sync_role 	to "{uut}:SYS:SYNC_ROLE";  monitor sync_role;

int slowmon_fs;   assign slowmon_fs 	to "{uut}:0:SLOWMON:FS"; monitor slowmon_fs;
int slowmon_fin;  assign slowmon_fin 	to "{uut}:0:SLOWMON:FIN"; 

int nacc_nacc; 	  assign nacc_nacc 	to "{uut}:0:NACC:NACC";
int nacc_start;	  assign nacc_start 	to "{uut}:0:NACC:START";
int nacc_shr;	  assign nacc_shr 	to "{uut}:0:NACC:SHR";
int nacc_pres;	  assign nacc_pres 	to "{uut}:0:NACC:PRES";
int nacc_pres_lim;assign nacc_pres_lim 	to "{uut}:0:NACC:PRES.HOPR";

#define NACC_LIM	256
#define SLOWMON_FIN_MAX 64


%{
int getfs(char* _sync_role) {
	int _fs;
	char mult = '\0';
	if (sscanf(_sync_role, "%*s %d%c", &_fs, &mult) > 1){
		switch (mult){
		case 'M':
			return _fs*1000000;
		case 'k':
			return _fs*1000;
		default:
			return _fs;
		}
	}
	return 0;
}

}%


string sync_role0 = { "not set" };


int slowmon_fs0 = -1;
int fs;

int verbose;

ss slowmon_rate_adjust {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			pvGet(nacc_pres_lim);
			macValueGet(sync_role);
			fs = getfs(sync_role);
			if (fs == 0){
				printf("ERROR: fs==0 sync_role:%s\n", sync_role);
				fs=100000;
			}
		} state wait_change
	} 
	
	state make_change {
		when() {
			if (fs > nacc_pres_lim * NACC_LIM * SLOWMON_FIN_MAX){
				printf("ERRROR: FS %d is too fast for available decimation: %d\n", fs, nacc_pres_lim * NACC_LIM * SLOWMON_FIN_MAX);
				nacc_pres = nacc_pres_lim;
				nacc_nacc = NACC_LIM;
				slowmon_fin = SLOWMON_FIN_MAX;
			}else{
				/* first, minimize software oversampling */ 
				for (slowmon_fin = SLOWMON_FIN_MAX;
						slowmon_fin>slowmon_fs && fs < nacc_pres_lim * NACC_LIM * slowmon_fin;
						slowmon_fin >>= 1){
					if (verbose){
						printf("slowmon_fin %d   fs %d < %d\n", slowmon_fin, fs, nacc_pres_lim * NACC_LIM * slowmon_fin);
					}
				}
				/* next, minimize prescale */
				for (nacc_pres  = nacc_pres_lim; 
				     nacc_pres>1 && fs < nacc_pres * NACC_LIM * slowmon_fin;
				     nacc_pres >>= 1){
					if (verbose){
						printf("nacc_pres %d   fs %d < %d\n", nacc_pres, fs, nacc_pres * NACC_LIM * slowmon_fin);
					}
				}
				for (nacc_nacc = NACC_LIM;
				     nacc_nacc>1 && fs < nacc_pres * nacc_nacc * slowmon_fin;
				     nacc_nacc >>= 1){
					if (verbose){
						printf("nacc_nacc %d   fs %d < %d\n", nacc_nacc, fs, nacc_pres * nacc_nacc * slowmon_fin);
					}
				}

			}
			for (nacc_shr = 0; 1<<nacc_shr < nacc_nacc; nacc_shr++){
				;
			}			
			pvPut(nacc_shr);
			pvPut(nacc_pres);
			pvPut(nacc_nacc);
			pvPut(slowmon_fin);
			
		} state wait_change
	}
	state wait_change {
		when(slowmon_fs != slowmon_fs0 || strcmp(sync_role, sync_role0)) {
			fs = getfs(sync_role);
			slowmon_fs0 = slowmon_fs;
		} state make_change
		
	}
}