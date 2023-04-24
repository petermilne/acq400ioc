/* ppw_tick_scaler.st
 * Watch SITE:CLKDIV, set PPW DELAY and TOP DRVH accordingly
 * */

program ppw_tick_scaler
option +r;


double max_tick; assign max_tick to "{uut}:{site}:PPW:DELAY:MAX"; monitor max_tick;

#define NDX	6
#define NDX1	7

double delay_drvh[NDX1]; assign delay_drvh to {};    // index from 1. 
double top_drvh[NDX1];   assign top_drvh to {};
double prd_drvh[NDX1];	 assign prd_drvh to {};

char* uut;
char* port;
char* site;
char pvname[128];
double max_tick0;
int verbose;
int dx;

ss ppw_tick_scaler {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			uut = macValueGet("uut");			
			site = macValueGet("site");
			for (dx = 1; dx <= 6; ++dx){
				snprintf(pvname, 128, "%s:%s:PPW:%d:PULSE:%s.DRVH", uut, site, dx, "DELAY");
				pvAssign(delay_drvh[dx], pvname);
				snprintf(pvname, 128, "%s:%s:PPW:%d:PULSE:%s.DRVH", uut, site, dx, "WIDTH");
				pvAssign(top_drvh[dx], pvname);
				snprintf(pvname, 128, "%s:%s:PPW:%d:PULSE:%s.DRVH", uut, site, dx, "PERIOD");
				pvAssign(prd_drvh[dx], pvname);
			}
			pvGet(max_tick);
		} state set_all
	}
	state set_all {
		entry{			
			for (dx = 1; dx <= 6; ++dx){
				delay_drvh[dx] = max_tick; 	pvPut(delay_drvh[dx]);
				top_drvh[dx] = max_tick; 	pvPut(top_drvh[dx]);
				prd_drvh[dx] = max_tick;	pvPut(prd_drvh[dx]);
			}
			max_tick0 = max_tick;
		}
		when() {
			;
		} state wait_change
	}

	state wait_change {
		when(max_tick != max_tick0){
			
		} state set_all
	}
}
