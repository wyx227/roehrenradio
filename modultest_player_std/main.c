#define _POSIX_C_SOURCE 200809L
//Deklaration der Verwendung von POSIX_C Standard

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include "flanken.h"
#include "popen.h"
#include "volumen.h"
#include "handling.h"
#include "delay.h"
#include "status_display.h"
#include "threads.h"
#include "global_variables.h"


//Funktionsprototypen 
int flank_high(int pin);
void cleanup();
void siginthandler(int sig_num); // Signalhandler beim Beenden, mpg321 zu beenden
void status_LED(int status_code);

int size_playlist() { // Mit dieser Funktion wird die eigentliche Länge der Playliste bestimmt. Bei Erstellen der Array-Liste werden alle Elemente mit 0 gefüllt.
	int length = 0;
	for (int i = 0; i < 32768; i++) {
		if (playlist[i] != 0) { // Alle 0 werden nicht gezählt
			length++;
		}
	}
	return length;
}

int read_dir() { // Verzeichnis wird gelesen

	DIR *d;
	struct dirent *dir;
	d = opendir(path); // Verzeichnis wird gelesen
	int ind = 0;
	
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (dir->d_type == 8) { //8 für Dateien, nicht für Ordner
				if (strstr(dir->d_name, "mp3")) {
					playlist[ind] = dir->d_name; // Die Playliste werden mit den Dateinamen (ausschliesslich mp3) gefüllt.
					ind++;
				}

			}

		}
		closedir(d); 
		return 0;
	}
	else {
		return 1; // Sollte das Verzeichnis nicht so zugegriffen werden, danach wird ein Fehlercode ausgegeben.
	}

	if (playlist[0] == 0) { // Sollte das Verzeichnis leer sein, wird ein Fehlercode ausgegeben.
		return 1;
	}
	
}

void generate_command() { // Der Play-Befehl wird generiert mit dem aktuellen Index.
	strcpy(cmd, "LOAD ");
	strcat(cmd, path);
	strcat(cmd, playlist[song_index]);
	strcat(cmd, "\n");
}




int main() {
	if (wiringPiSetupSys() == -1) { // Beim Fehler der GPIO-Freigabe wird das Programm direkt beendet.
		printf("Einrichten WiringPI fehlgeschlagen!\n");
		status_LED(3);
		return 1;
	}

	signal(SIGINT, siginthandler); // Deklaration des Signal-Händlers

	pinMode(PLAY, INPUT); //GPIO-Freigabe
	pinMode(VOR, INPUT);
	pinMode(RUECK, INPUT);

	pullUpDnControl(PLAY, PUD_UP); //Freischalten der Pullwiderstände
	pullUpDnControl(VOR, PUD_UP);
	pullUpDnControl(RUECK, PUD_UP);



	if (read_dir() == 0) {
		printf("Starting playback funtion\n");

		if (popen2("mpg321 -R 123", &infp, &outfp) == 0) {
			printf("Starting mpg321 player failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		status_LED(1);

		if (pthread_create(&t_play, NULL, play_music, NULL) != 0) {
			printf("Creating playback thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		if (pthread_create(&t_control, NULL, control, NULL) != 0) {
			printf("Creating input control thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		if (pthread_create(&t_stop, NULL, stop_music, NULL) != 0) {
			printf("Creating pausing control thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		if (pthread_create(&t_monitor, NULL, monitoring, NULL) != 0) {
			printf("Creating monitoring thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		if (pthread_join(t_stop, NULL) != 0) {
			printf("Starting pausing thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}

		if (pthread_join(t_play, NULL) != 0) {
			printf("Starting playing thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		if (pthread_join(t_control, NULL) != 0) {
			printf("Starting control thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		if (pthread_join(t_monitor, NULL) != 0) {
			printf("Starting monitoring thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}

		if (close(infp) != 0) {
			printf("Closing input handler failed, exiting anyway...\n");
			status_LED(3);
			return 1;
		}

		if (close(outfp) != 0) {
			printf("Closing output handler failed, exiting anyway...\n");
			status_LED(3);
			return 1;
		}
		//
		status_LED(4);
		return 0;

	
	}
	else {
		printf("File System Error: Unable to read the given directory or the directory is empty\n");
		status_LED(2);
		return 1;
	}



}