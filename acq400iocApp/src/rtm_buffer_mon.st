/* rtm_buffer_mon.st
 * Optimise rtm_buffer such that BPB pulses occur per buffer
 * Heuristic for BPB: < 50Hz : 1, < 100Hz : 2, > 100Hz 4   NOT AUTOMATED
 * BLEN must be a multiple of BPB*SSB and 4096
 * SM may adjust RTM_TRANSLEN to achieve goal
 * Later, adust WF SIZE for clean plot 
 * */

program rtm_buffer_mon
option +r;

int maxblen; 		assign maxblen 		to "{uut}:0:MAXBLEN";
int blen;    		assign blen 		to "{uut}:0:BLEN";
int bpb;     		assign bpb		to "{uut}:0:BPB";
int rtm_translen; 	assign rtm_translen 	to "{uut}:1:RTM_TRANSLEN";		monitor rtm_translen;
int rgm;		assign rgm 		to "{uut}:1:RGM";			monitor rgm;
int optimise_bufferlen;	assign optimise_bufferlen to "{uut}:0:OPTIMISE_BUFFERLEN";


#define MENU_RTM	3

int verbose;

int rtm_translen0 = -1;
int rgm0 = -1;

ss rtm_buffer_mon {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
		} state wait_rgm_change
	}
	state wait_rgm_change {
		when (rgm != rgm0 && rgm != MENU_RTM){
			rgm0 = rgm;
		} state go_continuous
		
		when ((rgm != rgm0 && rgm == MENU_RTM) || rtm_translen != rtm_translen0){
			rgm0 = rgm;
			rtm_translen0 = rtm_translen;
		} state go_rtm
	}
	
	state go_continuous {
		when() {
			
		} state wait_rgm_change
	} 
	
	state go_rtm {
		when() {
			
		} state wait_rgm_change
	}
}