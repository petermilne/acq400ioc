/* badled fixer - wat 2s after change, check LED is set correctly */


program badledfixer
option +r;
option +s;

%{
int _memcmp(void* a, void* b, int nb){
	int ii;
	for (ii = 0; ii < nb; ++ii){
		if (((char*)a)[ii] != ((char*)b)[ii]){
			return 1;
		}
	}
	return 0;
}
void _memcpy(void* to, void* from, int nb)
{
	int ii;
	for (ii = 0; ii < nb; ++ii){
		((char*)to)[ii] = ((char*)from)[ii];
	}	
}
}%

#define NPV	8

int setpoints[NPV];
int oldset[NPV];
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
		when(setpoints && _memcmp(setpoints, oldset, sizeof(setpoints))){
			_memcpy(oldset, setpoints, sizeof(setpoints));
		} state take5
	}
	
	state take5 {
		when(delay(7)){
			ii = 0;
		} state wait_a_sec
	}
	state wait_a_sec {		
		when(delay(0.5) && ii < NPV){

				for (; ii < NPV; ){
/*					
					pvGet(actuals[ii]);
					if (setpoints[ii] != actuals[ii]){
						pvPut(setpoints[ii]);
						break;
					}
*/
					printf("badledfixer:wait_a_sec %d put %d\n", ii, setpoints[ii]);
					pvPut(setpoints[ii++]);
					break;
				}
		} state wait_a_sec
		when (ii >= NPV){
			printf("badledfixer:wait_change\n");
		} state wait_change
	}	
}

