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
static char* dstName(char* base, char* mpv) {
	static char name[128];
	snprintf(name, 128, "%s:%s", base, mpv);
	return name;
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
		when(src != src0 && mirror_en0){
			src0 = src;
		} state make_change
		
		when(mirror_en != mirror_en0 && mirror_en){
			mirror_en0 = mirror_en;
		} state make_change
	}
	
	state make_change {
		when() {
			src0 = src;
			if (!pvAssigned(dst)){
				pvGet(dst_base);
				pvAssign(dst, dstName(dst_base, macValueGet("mirrorpv")));
			}
			dst = src;
			pvPut(dst);
			src0 = src;
		} state wait_change
	}
}