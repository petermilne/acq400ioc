program ai_vector
option +r;

// Aggregates all AI:CH:xx into one WF


// SRC  acq1001_325:1:AI:CH:01 .. 32
// DEST acq1001_325:1:AI:CHX


float src[33]; assign src to {};	// index from 1
float dst[33];  // index from 1

char *src_fmt = "{uut}:{site}:AI:CH:%02d";

assign dst to "{uut}:{site}:AI:CHX";

int ch;
int nchan = 32;
char pvname[80];

ss ai_vector {
    state init {
    	when() {
            for (ch = 1; ch <= nchan; ++ch){
            	snprintf(pvname, 80, src_fmt, ch);
            	pvAssign(src[ch], pvname);
            }
            pvMonitor(src[1]);
        } state connect_timeout
    } 
     
    state connect_timeout {
	when (pvConnectCount() == pvAssignCount()) {
	} state wait_change

	when (delay(10)) {
	} state connect_timeout
    }    
    state wait_change {
    	when(src[1] != src[0]){
    	    for (ch = 1; ch <= nchan; ++ch){
    	    	pvGet(src[ch]);
    	    	dst[ch] = src[ch];
    	    	pvPut(dst);
    	    }
    	    src[0] = src[1];
    	} state wait_change
    }
}