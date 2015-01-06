/*
onSetTransient.st : ensures correct trg/event0 for transient condition

INPUT
acq1001_049:MODE:TRANSIENT
acq2006_016:MODE:TRANSIENT:PRE

OUTPUT
acq2006_016:1:TRG:DX		0 (hard) or 1 (soft)
acq2006_016:1:EVENT0		0 (PRE==0), 1 (PRE!=0)
*/

program onSetTransient
option +r;

int transient;	assign transient to 	"{uut}:MODE:TRANSIENT";
int pre; 	assign pre to 		"{uut}:MODE:TRANSIENT:PRE";
int soft_trig;	assign soft_trig to 	"{uut}:1:TRG:DX";
int event;	assign event to		"{uut}:1:EVENT0";

monitor transient;
monitor pre;

int pre0;

ss onSetTransient {
	state init {
		when(transient != 0) {
			pvGet(pre);
			pre0 = pre;
		} state action
	}
	
	state wait_pre {
		when(pre != pre0){
			pre0 = pre;	
		} state action
	}
	
	state action {
		when() {
			if (pre == 0){
				event = 0;	pvPut(event);
				soft_trig = 0;	pvPut(soft_trig);
			}else{
				event = 1;	pvPut(event);
				soft_trig = 1;	pvPut(soft_trig);
			}
		} state wait_pre
	} 
}

