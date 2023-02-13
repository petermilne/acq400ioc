/* ppw_tick_scaler.st
 * Watch SITE:CLKDIV, set PPW DELAY and TOP DRVH accordingly
 * */

program ppw_tick_scaler
option +r;

int present; assign present to "{uut}:MGT:SFP:{port}:PRESENT"; monitor present;
int tx_disable; assign tx_disable to "{uut}:MGT:SFP:{port}:TX_DISABLE";

int verbose;
int remove_threshold = 5;
int remove_count = 0;
int present_count = 0;
char* port;

/* Rx Pwr thresholds in uW */
#define TH_RX_LO	5
#define TH_RX_HI	10

ss ppw_tick_scaler {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
			if (macValueGet("remove_threshold")){
				remove_threshold = macValueGet("remove_threshold")[0] - '0';
			}
			port = macValueGet("port");
			tx_disable = 1; pvPut(tx_disable);
		} state wait_present
	}
	state wait_present {
		when(present){
			tx_disable = 0; pvPut(tx_disable);
			remove_count = 0;
			if (verbose) printf("%s:%s PRES:%d %s -> %s TX_DIS:%d %s\n", "acq2206_sfp_en", port, present, "wait_present", "wait_remove", tx_disable, tx_disable?"OFF":"ON");
		} state wait_remove
	}

	state wait_remove {
		when(!present && ++remove_count > remove_threshold){
			present_count = 0;
			tx_disable = 1; pvPut(tx_disable);
			if (verbose) printf("%s:%s PRES:%d %s -> %s TX_DIS:%d %s\n", "acq2206_sfp_en", port, present, "wait_remove", "wait_present", tx_disable, tx_disable?"OFF":"ON");
		} state wait_present
		when(!present){
			if (verbose) printf("%s:%s PRES:%d %s -> %s TX_DIS:%d %s\n", "acq2206_sfp_en", port, present, "wait_remove", "wait_remove_timeout", tx_disable, tx_disable?"OFF":"ON");
		} state wait_remove_timeout
		when(present){
			if (++present_count < 4 || present_count%16 == 0){
				if (verbose) printf("%s:%s PRES:%d %s -> %s TX_DIS:%d %s\n", "acq2206_sfp_en", port, present, "wait_remove", "wait_remove_timeout", tx_disable, tx_disable?"OFF":"ON");
			}
		} state wait_remove_timeout
	}
	state wait_remove_timeout {
		when(delay(1)){

		} state wait_remove
	}
}
