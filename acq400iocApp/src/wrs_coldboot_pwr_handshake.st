/* wrs_coldboot_pwr_handshake.st
 * WRS doesn't like powering up while we shine a light at it.
 * 
 * Strategy:
 * - monitor RXPWR, if RXPWR goes down:
 * - TXDISABLE
 * - wait for RXPWR to come back on WRS reboot
 * - TXENABLE
 * 
 * We monitor COUNT to add some debounce in case of a rogue bad reading. Speed is NOT of the essence.
 * */

program wrs_coldboot_pwr_handshake
option +r;

int rxpower; assign rxpower to "{uut}:SFP:3:RXPWR"; monitor rxpower;
int upcount; assign upcount to "{uut}:SFP:3:RXPWR:COUNT"; monitor upcount;
int txdisable; assign txdisable to "{uut}:11:SFP_TX_DISABLE";

int verbose;

int upcount0;

/* Rx Pwr thresholds in uW */
#define TH_RX_LO	5
#define TH_RX_HI	10

ss wrs_coldboot_pwr_handshake {
	state init {
		when() {
			if (macValueGet("verbose")){
				verbose = macValueGet("verbose")[0] - '0';
			}
		} state wait_rxpwr_down_0
	}
	state wait_rxpwr_down_0 {
		when(upcount > 0 && rxpower < TH_RX_LO){
			upcount0 = upcount;
		} state wait_rxpwr_down_1
	}
	
	state wait_rxpwr_down_1 {		
		when(upcount != upcount0 && rxpower < TH_RX_LO){
			txdisable = 1; pvPut(txdisable);
			printf("wrs_coldboot_pwr_handshake rxpower:%d %s\n", rxpower, "TXDISABLE");
		} state wait_rxpwr_up_0
		
		when(upcount != upcount0 && rxpower >= TH_RX_LO){			
			printf("wrs_coldboot_pwr_handshake rxpower:%d %s GLITCH\n", rxpower, "TXDISABLE");
		} state wait_rxpwr_down_0		
	}
	
	state wait_rxpwr_up_0 {
		when(rxpower > TH_RX_HI){
			upcount0 = upcount;
		} state wait_rxpwr_up_1
	}
	
	state wait_rxpwr_up_1 {
		when(upcount != upcount0 && rxpower > TH_RX_HI){
			txdisable = 0;  pvPut(txdisable);
			printf("wrs_coldboot_pwr_handshake rxpower:%d %s\n", rxpower, "TXDISABLE OFF");
		} state wait_rxpwr_down_0
		
		when(upcount != upcount0 && rxpower <= TH_RX_HI){
			printf("wrs_coldboot_pwr_handshake rxpower:%d %s GLITCH\n", rxpower, "TXDISABLE OFF");
		} state wait_rxpwr_up_0
	}	
}