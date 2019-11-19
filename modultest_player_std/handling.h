#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "status_display.h"

void cleanup() {
	puts("\nTerminating...\n");
	system("sudo pkill mpg321");
	status_LED(4);
}

void siginthandler(int sig_num) {
	//signal(SIGINT, siginthandler);
	//printf("Exiting with Ctrl+C...\n");

	cleanup();
	exit(EXIT_SUCCESS);
	
}