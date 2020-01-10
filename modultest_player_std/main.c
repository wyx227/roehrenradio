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
#include <mcp3004.h>


#include "flanken.h"
#include "popen.h"
#include "volumen.h"
#include "handling.h"
#include "status_display.h"


//Pinbelegung der Tasten
#define PLAY 17
#define VOR 27
#define RUECK 22


#define MUX1 14
#define MUX2 15
#define MUX3 18

#define BASE 200
#define SPI_CHAN 0

//globale Variablen
pthread_t t_control, t_monitor; //pthread-Bezeichner aller auszufuehrenden Threads
int song_index = 0; //Index der Playliste
int fresh_start = 1; // Variable, ob es sich um den frischen Start handelt
char *path = "//home//pi//music//"; //vorgegebener Pfad zur Speicherung der Musikdateien
int infp, outfp; // Bezeichner fuer die Stdin und Stdout des durch Pipe gestarteten Prozesses
char cmd[128]; // Die an stdin zu sendenden Befehle
char output[128]; // Stdout der MPG321
char *playlist[32768] = { 0 }; // Char-Array zur Speicherung der Playliste, maximal 32768 Elemente erlaubt, maximaler Index bis 32767

//Funktionsprototypen 
int flank_high(int pin);
void cleanup();
void siginthandler(int sig_num); // Signalhandler beim Beenden, mpg321 zu beenden
void status_LED(int status_code);

int size_playlist() { // Mit dieser Funktion wird die eigentliche Laenge der Playliste bestimmt. Bei Erstellen der Array-Liste werden alle Elemente mit 0 gefuellt.
	int length = 0;
	for (int i = 0; i < 32768; i++) {
		if (playlist[i] != 0) { // Alle 0 werden nicht gezaehlt
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
			if (dir->d_type == 8) { //8 fuer Dateien, nicht fuer Ordner
				if (strstr(dir->d_name, "mp3")) {
					playlist[ind] = dir->d_name; // Die Playliste werden mit den Dateinamen (ausschliesslich mp3) gefuellt.
					ind++;
				}

			}

		}
		closedir(d); 
	}
	else {
		return 1; // Sollte das Verzeichnis nicht so zugegriffen werden, danach wird ein Fehlercode ausgegeben.
	}

	if (playlist[0] == 0) { // Sollte das Verzeichnis leer sein, wird ein Fehlercode ausgegeben.
		return 1;
	}
	else {
		return 0;
	}
	
}

void generate_command() { // Der Play-Befehl wird generiert mit dem aktuellen Index.
	memset(cmd, 0, sizeof(cmd));
	strcpy(cmd, "LOAD ");
	strcat(cmd, path);
	strcat(cmd, playlist[song_index]);
	strcat(cmd, "\n");
}

void *monitoring() { //Ueberwacht, ob das Spielen vom aktuellen Lied beendet ist. -> 1ms Zyklus
	while (1) {
		if (read(outfp, output, 128)) {
			if (strstr(output, "@P 3")) //Nach dem Ende der Wiedergabe wird dieser String ausgegeben.
			{
				if (song_index != size_playlist(playlist)-1) {
					song_index++;
					
				}
				else {
					song_index = 0; //Reset nach dem Abspielen
				}
				generate_command();
				if (write(infp, cmd, strlen(cmd)) == -1) {
					status_LED(3);
					exit(EXIT_FAILURE);
				}
				
			}
		}
		else {
			printf("Error reading Stdout, critical error!\n");
			status_LED(3);
			exit(EXIT_FAILURE);
		}
		
	}
}




void *control() { //Dieser Thread liest alle Tasten und Potis ueber GPIO-Eingaengen. Gleichzeitiges Druecken von Tasten wird ausgeschlossen.
	//500ms-Zyklus
	int analog_input;
	while (1) {
		if ((flank_high(PLAY) == HIGH) && !(flank_high(VOR)) && !(flank_high(RUECK))) { // Bei Betaetigung der Play-Taste wird der Zustand zum Spielen bzw. Pausieren gewechselt.

			if (fresh_start == 0) {
				if (write(infp, "PAUSE\n", 7) == -1) { //Wenn die Wiedergabe pausiert ist, wird durch Eingabe von "PAUSE" die Wiedergabe fortgesetzt
					status_LED(3);
					exit(EXIT_FAILURE);
				}
			}
			else {
				fresh_start = 0;
				generate_command();
				if (write(infp, cmd, strlen(cmd)) == -1) {
					status_LED(3);
					exit(EXIT_FAILURE);
				}
			}
			
		}
		if (flank_high(VOR) == HIGH && fresh_start != 0 && !(flank_high(PLAY)) && !(flank_high(RUECK))) { //Bei Betaetigung der VOR-Taste wird der Index der Playliste um 1 inkrementiert, wenn das Ende nicht erreicht wurde.
			if (song_index < size_playlist()-1) {
				song_index++;
				
			}
			generate_command();
			if (write(infp, cmd, strlen(cmd)) == -1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}


		}
		if (flank_high(RUECK) == HIGH && fresh_start != 0 && !(flank_high(PLAY)) && !(flank_high(VOR))) { //Ebenso fuer die RUeCK-Taste
			if (song_index > 0) {
				song_index--; 
				
			}
			generate_command();
			if (write(infp, cmd, strlen(cmd)) == 1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}


		}
		//dreistufige Volumenregelung
		analog_input = analogRead(BASE + 2);
		if (analog_input < 300) {
			set_volume(-3251);
		}
		else if (analog_input < 450 && analog_input >= 300) {
			set_volume(-5213);
		}
		else {
			set_volume(-7628);
		}
		
		delay(500);
	}
	return 0;
}



int main() {
	if (wiringPiSetupSys() == -1) { // Beim Fehler der GPIO-Freigabe wird das Programm direkt beendet.
		printf("WiringPi configuration failed!\n");
		status_LED(3);
		return 1;
	}

	if (mcp3004Setup(BASE, SPI_CHAN) == -1) {
		printf("ADC configuration failed!\n");
		status_LED(3);
		return 1;
	}

	signal(SIGINT, siginthandler); // Deklaration des Signal-Haendlers

	pinMode(PLAY, INPUT); //GPIO-Freigabe
	pinMode(VOR, INPUT);
	pinMode(RUECK, INPUT);

	pullUpDnControl(PLAY, PUD_UP); //Freischalten der Pullwiderstaende
	pullUpDnControl(VOR, PUD_UP);
	pullUpDnControl(RUECK, PUD_UP);

	
	digitalWrite(MUX1, LOW);
	digitalWrite(MUX2, LOW);
	digitalWrite(MUX3, HIGH);



	if (read_dir() == 0) {
		printf("Starting playback funtion\n");
		status_LED(1);

		if (popen2("mpg321 -R 123", &infp, &outfp) == 0) {
			printf("Starting mpg321 player failed, exiting...\n");
			status_LED(3);
			return 1;
		}
		status_LED(1);


		if (pthread_create(&t_control, NULL, control, NULL) != 0) {
			printf("Creating input control thread failed, exiting...\n");
			status_LED(3);
			return 1;
		}

		if (pthread_create(&t_monitor, NULL, monitoring, NULL) != 0) {
			printf("Creating monitoring thread failed, exiting...\n");
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

		status_LED(4);
		return 0;

	
	}
	else {
		printf("File System Error: Unable to read the given directory or the directory is empty\n");
		status_LED(2);
		return 1;
	}



}