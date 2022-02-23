/* mgt_ssb.st
 * Set Sample Size Bytes for MGT Aggregator
 * */

program mgt_ssb
option +r;

int spad_len;		assign spad_len 	to "{uut}:{ch}:AGGREGATOR:SPAD:LEN"; monitor spad_len;
string comms_sitelist;	assign comms_sitelist 	to "{uut}:{ch}:AGGREGATOR:SITES";    monitor comms_sitelist;
string s0_sitelist;	assign s0_sitelist	to "{uut}:0:AGGREGATOR:SITES";	     monitor s0_sitelist;
int mgt_ssb;		assign mgt_ssb		to "{uut}:{ch}:AGGREGATOR:SSB";

string sitelist;	assign sitelist		to "{uut}:SITELIST";

int ssb[7];		assign ssb to {};	// index from 1

#define PNAME		"mgt_ssb "

int verbose;

int spad_len0 = -1;
string comms_sitelist0;


%{
int isdigit(int c) {
	return c >= '0' && c <= '9';
}
	
int calculate_ssb(string _comms_sitelist, string _s0_sitelist, int* _ssb, int _spad_len, int _verbose) {
	int ic;
	int ssb = _spad_len * 4;
	
	for (ic = 0; ic < strlen(_comms_sitelist); ++ic){
		if (_s0_sitelist[ic] == ','){
			continue;
		}
		if (_verbose) printf("calculate_ssb [%d] c:\"%s\" s0:\"%s\"\n", ic, _comms_sitelist, _s0_sitelist);
		if (strchr(_s0_sitelist, _comms_sitelist[ic]) == 0){
			if (_verbose) printf("truncate [%d] %c\n", ic, _comms_sitelist[ic]);
			_comms_sitelist[ic] = '\0';	// all sites in comms are in s0. truncate/break at first diff.
			if (_verbose) printf("truncate [%d] \"%s\"\n", ic, _comms_sitelist);
			break;
		}
		if (isdigit(_comms_sitelist[ic])){
			int site = _comms_sitelist[ic] - '0';
			int site_bytes = _ssb[site];			
			ssb += site_bytes;
			if (_verbose) printf("calculate_ssb %s site:%d site_bytes:%d ssb:%d\n", 
					_comms_sitelist, site, site_bytes, ssb);
		}
	}

	return ssb;
}

int isInSitelist(string _sitelist, char _site) {
	char ssel[4];
	ssel[0] = _site;
	ssel[1] = '=';
	ssel[2] = '\0';
	return strstr(_sitelist, ssel) != NULL;
}
}%

char* uut;
int site;
char pvname[80];

ss mgt_ssb {
	state init {
		when() {
			uut = macValueGet("uut");
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			pvGet(sitelist);
		} state wait_s0
	}
	
	state wait_s0 {
		when(strlen(sitelist)){
			for (site = '1'; site <= '6'; ++site){
				int isite = site-'0';
				if (isInSitelist(sitelist, site)){
					snprintf(pvname, sizeof(pvname), "%s:%d:SSB", uut, isite);
					if (verbose) printf("pvAssign \"%s\"\n", pvname);
					pvAssign(ssb[isite], pvname);
				}
			}			
		} state wait_change
	}
	
	state wait_change {
		when (spad_len != spad_len0 || strcmp(comms_sitelist, comms_sitelist0) != 0 ){
			for (site = '1'; site <= '6'; ++site){
				int isite = site-'0';
				if (strchr(s0_sitelist, site)){
					if (verbose) printf("pvGet ssb: %d\n", isite);
					pvGet(ssb[isite]);
					
					if (verbose) printf("pvGet s:%d ssb:%d\n", isite, ssb[isite]);
					if (ssb[isite] == 0){
						printf("WARNING: s:%d ZERO ssb\n", isite);
					}
				}
			}
			mgt_ssb = calculate_ssb(comms_sitelist, s0_sitelist, ssb, spad_len, verbose);
			if (verbose) printf("put comms_sitelist \"%s\"\n", comms_sitelist);
			pvPut(comms_sitelist);
			pvPut(mgt_ssb);
			
			spad_len0 = spad_len;
			strcpy(comms_sitelist0, comms_sitelist);
		} state wait_change
	}
}
