program on_delay
option +r;

// seq on_delay "pv=${HOST}:IOC_READY,val=1,delay=3,priority=0"

int pv; assign pv to "{pv}";


ss on_delay {
	state init {
		when(delay(*macValueGet("delay")-'0')){
			pv = *macValueGet("val")-'0';
			pvPut(pv);
			printf("set %s to %d\n", macValueGet("pv"), pv);
		} state done 		
	}
	
	state done {
		when(0) {
			printf("done\n");
		} state done
	} 
}

