/*
mgtStopOnLinkDown.st : stop streaming if link goes down
*/

program mgtStopOnLinkDown
option +r;


int aggA;
assign aggA to "{uut}:A:AGGREGATOR";
monitor aggA;

int aggB;
assign aggB to "{uut}:B:AGGREGATOR";
monitor aggB;

int linkA_up0;
int linkA_up;
assign linkA_up to "{uut}:A:AURORA:UP";
monitor linkA_up;

int linkB_up0;
int linkB_up;
assign linkB_up to "{uut}:B:AURORA:UP";
monitor linkB_up;

%%#include <stdlib.h>
%{
static void _stop_stream()
{
	system("/usr/local/bin/streamtonowhered stop");
}
}%


ss mgtStopOnLinkDown
{
	state init {
		when() {
			pvGet(linkA_up);
			linkA_up0 = linkA_up;
			
			pvGet(linkB_up);
			linkB_up0 = linkA_up;	
		} state wait_link_change
	} 
	
	state wait_link_change {
		when (linkA_up != linkA_up0 && linkA_up == 0){
			printf("linkA DOWN, stop streaming\n");
		} state stop_stream
				
		when(linkA_up != linkA_up0){
			linkA_up0 = linkA_up;
		} state wait_link_change
		
		when (linkB_up != linkB_up0 && linkB_up == 0){
			printf("linkB DOWN, stop streaming\n");
		} state stop_stream
			
		when (linkB_up != linkB_up0){			
			linkB_up0 = linkB_up;
		} state wait_link_change		
	}
	
	state stop_stream {
		when(aggA || aggB) {		
			_stop_stream();
		} state wait_link_up
		
		when() {
		} state init		
	}
	
	state wait_link_up {
		when (((aggA && linkA_up) || (!aggA && !linkA_up)) &&
		      ((aggB && linkB_up) || (!aggB && !linkB_up))    ){
		      printf("link status OK, resume monitor\n");
		      linkA_up0 = linkA_up;
		      linkB_up0 = linkB_up;
		} state wait_link_change
	}
	
}