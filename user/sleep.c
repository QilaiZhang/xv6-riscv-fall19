#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
	int sleep_sec;
	if (argc < 2){
		fprintf(2, "Usage: sleep [number] (100ms)\n");
		exit();
	}

	sleep_sec = atoi(argv[1]);
	if (sleep_sec > 0){
		sleep(sleep_sec);
	} else {
		fprintf(2, "Invalid number %s\n", argv[1]);
	}
	exit();
}