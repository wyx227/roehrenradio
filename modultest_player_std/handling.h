#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void cleanup() {
	puts("Terminating...\n");
	system("sudo pkill mpg321");
}

void siginthandler(int sig_num) {
	//signal(SIGINT, siginthandler);
	//printf("Exiting with Ctrl+C...\n");

	cleanup();
	exit(EXIT_SUCCESS);
	
}