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

//Pinbelegung der Tasten
#define PLAY 17
#define VOR 27
#define RUECK 22
//globale Variablen
pthread_t t_play,t_stop, t_control; //pthread-Bezeichner aller auszuf�hrenden Threads
int song_index = 0; //Index der Playliste
int play = 0; // Variable, ob die Playtaste gedr�ckt wird, also ob die Musik gespielt wurde
int firstrun = 0; // Variable, ob es sich um den frischen Start handelt
char *path = "//home//pi//music//"; //vorgegebener Pfad zur Speicherung der Musikdateien
int infp, outfp; // Bezeichner f�r die Stdin und Stdout des durch Pipe gestarteten Prozesses
char cmd[255]; // Die an stdin zu sendenden Befehle
char *playlist[32768] = { 0 }; // Char-Array zur Speicherung der Playliste, maximal 32768 Elemente erlaubt, maximaler Index bis 32767

//Funktionsprototypen 
int flank_high(int pin);
void cleanup();
void siginthandler(int sig_num); // Signalhandler beim Beenden, mpg321 zu beenden


int size_playlist() { // Mit dieser Funktion wird die eigentliche L�nge der Playliste bestimmt. Bei Erstellen der Array-Liste werden alle Elemente mit 0 gef�llt.
	int length = 0;
	for (int i = 0; i < 32768; i++) {
		if (playlist[i] != 0) { // Alle 0 werden nicht gez�hlt
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
			if (dir->d_type == 8) { //8 f�r Dateien, nicht f�r Ordner
				playlist[ind] = dir->d_name; // Die Playliste werden mit den Dateinamen gef�llt.
				ind++;
			}

		}
		closedir(d); 
		return(0);
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

void *play_music() { // Beim Bet�tigen der Play-Taste und wenn das Programm frisch ausgef�hrt wird, wird das erste Lied gespielt
	while (1) {
		if (play == 1 && firstrun == 0) {
			firstrun = 1;
			generate_command();
			write(infp, cmd, 128);
			
		}
	}
	return 0;
} 

void *stop_music(){ // Beim Best�tigen der Play-Taste und wenn bereit eine Musik spielt, wird das Wiedergeben pausiert.
	while (1) {
		if (play == 0 && firstrun != 0) {
			
			write(infp, "PAUSE\n", 128);

		}
	}
	return 0;
}


void *control() { //Dieser Thread liest alle Tasten und Potis �ber GPIO-Eing�ngen
	while (1) {
		if (flank_high(PLAY) == HIGH) { // Bei Bet�tigung der Play-Taste wird der Zustand zum Spielen bzw. Pausieren gewechselt.
			if (play == 0) {
				play++;
				delay(500);
			}
			else {
				play--;
			}
			
		}
		if (flank_high(VOR) == HIGH && firstrun != 0) { //Bei Bet�tigung der VOR-Taste wird der Index der Playliste um 1 inkrementiert, wenn das Ende nicht erreicht wurde.
			if (song_index < size_playlist()-1) {
				song_index++;
				delay(500);
			}
			generate_command();
			write(infp, cmd, 128);


		}
		if (flank_high(RUECK) == HIGH && firstrun != 0) { //Ebenso f�r die R�CK-Taste
			if (song_index > 0) {
				song_index--; 
				delay(500);
			}
			generate_command();
			write(infp, cmd, 128);


		}
		//set_volume(25);
		
	}
	return 0;
}



int main() {
	if (wiringPiSetupSys() == -1) { // Beim Fehler der GPIO-Freigabe wird das Programm direkt beendet.
		printf("Einrichten WiringPI fehlgeschlagen!\n");
		return 1;
	}

	signal(SIGINT, siginthandler); // Deklaration des Signal-H�ndlers

	pinMode(PLAY, INPUT); //GPIO-Freigabe
	pinMode(VOR, INPUT);
	pinMode(RUECK, INPUT);

	pullUpDnControl(PLAY, PUD_UP); //Freischalten der Pullwiderst�nde
	pullUpDnControl(VOR, PUD_UP);
	pullUpDnControl(RUECK, PUD_UP);

	if (read_dir() == 0) {
		printf("Starting playback funtion\n");

		if (popen2("mpg321 -R 123", &infp, &outfp) == 0) {
			printf("Starting mpg321 player failed, exiting...\n");
			return 1;
		}


		if (pthread_create(&t_play, NULL, play_music, NULL) != 0) {
			printf("Creating playback thread failed, exiting...\n");
			return 1;
		}
		if (pthread_create(&t_control, NULL, control, NULL) != 0) {
			printf("Creating input control thread failed, exiting...\n");
			return 1;
		}
		if (pthread_create(&t_stop, NULL, stop_music, NULL) != 0) {
			printf("Creating pausing control thread failed, exiting...\n");
			return 1;
		}
		if (pthread_join(t_stop, NULL) != 0) {
			printf("Starting pausing thread failed, exiting...\n");
			return 1;
		}

		if (pthread_join(t_play, NULL) != 0) {
			printf("Starting playing thread failed, exiting...\n");
			return 1;
		}
		if (pthread_join(t_control, NULL) != 0) {
			printf("Starting control thread failed, exiting...\n");
			return 1;
		}

		if (close(infp) != 0) {
			printf("Closing input handler failed, exiting anyway...\n");
			return 1;
		}
		//
		
		return 0;
	
	}
	else {
		printf("File System Error: Unable to read the given directory or the directory is empty\n");
		return 1;
	}



}