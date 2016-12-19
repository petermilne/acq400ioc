/* pvmirror.st : on pv change, reflect change at mirror */

program pvmirror
option +r;
option +s;	/* keep local variable copies */

int src;	 assign src to "{uut}:{mirrorpv}";
string dst_base; assign dst_base to "{uut}:MIRROR_HOST";
int mirror_en;   assign mirror_en to "{uut}:MIRROR_EN";

int dst;	 assign dst to ""; /* dynamic assign */

monitor src;
monitor mirror_en;

int src0;
int mirror_en0;



%{
char dst_name[128];

static char* dstName(char* base, char* mpv) {	
	snprintf(dst_name, 128, "%s:%s", base, mpv);
	return dst_name;
}

static void show_pvPut(int value) {
	printf("pvPut %s = %d\n", dst_name, value);
}
}%

ss MirrorOnChange {
	state init {
		when() {
			pvGet(src);
			src0 = src;
			pvGet(mirror_en);
			mirror_en0 = mirror_en;
		} state wait_change
	} 
	
	state wait_change {
		when(src != src0 && mirror_en == 1){			
			src0 = src;
		} state make_change
		
		when(mirror_en != mirror_en0 && mirror_en == 1){
			mirror_en0 = mirror_en;
		} state make_change
	}
	
	state make_change {
		when() {			
			if (!pvAssigned(dst)){
				pvGet(dst_base);
				pvAssign(dst, dstName(dst_base, macValueGet("mirrorpv")));
			}
			dst = src;			
			show_pvPut(dst);
			src0 = src;
		} state wait_change
	}
}