/* badled fixer - wat 2s after change, check LED is set correctly */


program badledfixer
option +r;
option +s;

#define NPV	8

int setpoints[NPV]; 
assign setpoints to { 
	"{uut}:MOD:1:WARN", "{uut}:MOD:2:WARN", "{uut}:MOD:3:WARN",
	"{uut}:MOD:4:WARN", "{uut}:MOD:5:WARN", "{uut}:MOD:6:WARN",
	"{uut}:SYS:LED:ACT_G", "{uut}:SYS:LED:ACT_R"
}; 
monitor setpoints;

int actuals[NPV];
assign actuals to { 
	"{uut}:MOD:1:FAIL", "{uut}:MOD:2:FAIL", "{uut}:MOD:3:FAIL",
	"{uut}:MOD:4:FAIL", "{uut}:MOD:5:FAIL", "{uut}:MOD:6:FAIL",
	"{uut}:SYS:LED:ACT_G:r", "{uut}:SYS:LED:ACT_R:r"
}; 

int ii;

ss badledfixer {
	state wait_change {
		when(setpoints){
			
		} state take5
	}
	
	state take5 {
		when(delay(5)){
			ii = 0;
		} state wait_a_sec
	}
	state wait_a_sec {
		when(delay(0.5) && ii < NPV){
				for (; ii < NPV; ++ii){
					pvGet(actuals[ii]);
					if (setpoints[ii] != actuals[ii]){
						pvPut(setpoints[ii]);
						break;
					}
				}
		} state wait_a_sec
		when (ii >= NPV){
			
		} state wait_change
	}	
}

