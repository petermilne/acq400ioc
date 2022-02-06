/* mgt_ssb.st
 * Set Sample Size Bytes for MGT Aggregator
 * */

program mgt_ssb
option +r;

int spad_len;		assign spad_len 	to "{uut}:{ch}:AGGREGATOR:SPAD:LEN"; monitor spad_len;
string comms_sitelist;	assign comms_sitelist 	to "{uut}:{ch}:AGGREGATOR:SITES";    monitor comms_sitelist;
string s0_sitelist;	assign s0_sitelist	to "{uut}:0:AGGREGATOR:SITES";
int mgt_ssb;		assign mgt_ssb		to "{uut}:{ch}:AGGREGATOR:SSB";

int nchan[7];		assign nchan to {};	// index from 1
int data32[7];		assign data32 to {};	// index from 1


#define PNAME		"mgt_ssb "


int verbose;

int spad_len0 = -1;
string comms_sitelist0;


%{
int isdigit(int c) {
	return c >= '0' && c <= '9';
}
	
int calculate_ssb(string _comms_sitelist, string _s0_sitelist, int* _nchan, int* _data32, int _spad_len) {
	int ic;
	int ssb = 0;
	
	for (ic = 0; ic < strlen(_comms_sitelist); ++ic){
		if (strchr(_s0_sitelist, _comms_sitelist[ic]) == 0){
			_comms_sitelist[ic] = '\0';	// all sites in comms are in s0. truncate/break at first diff.
			break;
		}
		if (isdigit(_comms_sitelist[ic])){
			int site = _comms_sitelist[ic] - '0';
			ssb += _nchan[site] * (_data32[site]? 4: 2);
		}
	}

	ssb += _spad_len * 4;
	return ssb;
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
			for (site = 0; site <= 6; ++site){
				snprintf(pvname, sizeof(pvname), "%s:%d:NCHAN", uut, site);
				pvAssign(nchan[site], pvname);
				snprintf(pvname, sizeof(pvname), "%s:%d:data32", uut, site);
				pvAssign(data32[site], pvname);
			}
		} state wait_change
	}
	state wait_change {
		when (spad_len != spad_len0 || strcmp(comms_sitelist, comms_sitelist0) != 0 ){
			for (site = 0; site <= 6; ++site){
				pvGet(nchan[site]);
				pvGet(data32[site]);
			}
			pvGet(s0_sitelist);
			mgt_ssb = calculate_ssb(comms_sitelist, s0_sitelist, nchan, data32, spad_len);
			pvPut(comms_sitelist);
			pvPut(mgt_ssb);
			
			spad_len0 = spad_len;
		} state wait_change
	}
}
