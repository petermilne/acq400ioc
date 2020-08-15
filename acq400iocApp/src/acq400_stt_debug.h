#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static int fd = 1;


void openDebug(char* debug){
	if (debug == 0 || debug[0] == '\0' || debug[0] == '1'){
		fd = 1;
	}else{
		fd = open(debug, (O_CREAT | O_WRONLY), 0664);
		if (fd < 0){
			printf("ERROR failed to open debug:%s\n", debug);
			exit(-1);
		}
	}
}

void closeDebug() {
	if (fd != 1) close(fd);
}
