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
#include <mcp3004.h>

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
pthread_t t_play,t_stop, t_control, t_monitor; //pthread-Bezeichner aller auszuf�hrenden Threads
int song_index = 0; //Index der Playliste
int play = 0; // Variable, ob die Playtaste gedr�ckt wird, also ob die Musik gespielt wurde
int fresh_start = 0; // Variable, ob es sich um den frischen Start handelt
char *path = "//home//pi//music//"; //vorgegebener Pfad zur Speicherung der Musikdateien
int infp, outfp; // Bezeichner f�r die Stdin und Stdout des durch Pipe gestarteten Prozesses
char cmd[128]; // Die an stdin zu sendenden Befehle
char output[128]; // Stdout der MPG321
char *playlist[32768] = { 0 }; // Char-Array zur Speicherung der Playliste, maximal 32768 Elemente erlaubt, maximaler Index bis 32767

//Funktionsprototypen 
int flank_high(int pin);
void cleanup();
void siginthandler(int sig_num); // Signalhandler beim Beenden, mpg321 zu beenden
void status_LED(int status_code);

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
				if (strstr(dir->d_name, "mp3")) {
					playlist[ind] = dir->d_name; // Die Playliste werden mit den Dateinamen (ausschliesslich mp3) gef�llt.
					ind++;
				}

			}

		}
		closedir(d); 
		//return 0;
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
	strcpy(cmd, "LOAD ");
	strcat(cmd, path);
	strcat(cmd, playlist[song_index]);
	strcat(cmd, "\n");
}

void *monitoring() { //�berwacht, ob das Spielen vom aktuellen Lied beendet ist.
	while (1) {
		if (read(outfp, output, 128)) {
			if (strstr(output, "@P 3")) //Nach dem Ende der Wiedergabe wird dieser String ausgegeben.
			{
				if (song_index != size_playlist(playlist)-1) {
					song_index++;
					generate_command();
				}
				else {
					song_index = 0; //Reset nach dem Abspielen
				}

				if (write(infp, cmd, 128) == -1) {
					status_LED(3);
					exit(EXIT_FAILURE);
				}
				//fflush(stdin);
			}
		}
		else {
			printf("Error reading Stdout, critical error!\n");
			status_LED(3);
			exit(EXIT_FAILURE);
		}
		
	}
}

void *play_music() { // Beim Bet�tigen der Play-Taste und wenn das Programm frisch ausgef�hrt wird, wird das erste Lied gespielt
	while (1) {
		if (play == 1 && fresh_start == 0) {
			fresh_start = 1;
			generate_command();
			if (write(infp, cmd, 128) == -1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}
			
		}
	}
	return 0;
} 

void *stop_music(){ // Beim Best�tigen der Play-Taste und wenn bereit eine Musik spielt, wird das Wiedergeben pausiert.
	while (1) {
		if (play == 0 && fresh_start != 0) {
			
			if (write(infp, "PAUSE\n", 128) == -1) { //Wenn die Wiedergabe pausiert ist, wird durch Eingabe von "PAUSE" die Wiedergabe fortgesetzt
				status_LED(3);
				exit(EXIT_FAILURE);
			}

		}
	}
	return 0;
}


void *control() { //Dieser Thread liest alle Tasten und Potis �ber GPIO-Eing�ngen. Gleichzeitiges Druecken von Tasten wird ausgeschlossen.
	int analog_input;
	while (1) {
		if ((flank_high(PLAY) == HIGH) && !(flank_high(VOR)) && !(flank_high(RUECK))) { // Bei Bet�tigung der Play-Taste wird der Zustand zum Spielen bzw. Pausieren gewechselt.
			if (play == 0) {
				play++; //Merkvariable, ob Wiedergabe gestartet wird.
				delay_no_itr(500);
			}
			else {
				play--;
				delay_no_itr(500);
			}
			
		}
		if (flank_high(VOR) == HIGH && fresh_start != 0 && !(flank_high(PLAY)) && !(flank_high(RUECK))) { //Bei Bet�tigung der VOR-Taste wird der Index der Playliste um 1 inkrementiert, wenn das Ende nicht erreicht wurde.
			if (song_index < size_playlist()-1) {
				song_index++;
				delay_no_itr(500);
			}
			generate_command();
			if (write(infp, cmd, 128) == -1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}


		}
		if (flank_high(RUECK) == HIGH && fresh_start != 0 && !(flank_high(PLAY)) && !(flank_high(VOR))) { //Ebenso f�r die R�CK-Taste
			if (song_index > 0) {
				song_index--; 
				delay_no_itr(500);
			}
			generate_command();
			if (write(infp, cmd, 128) == 1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}


		}
		//Zweistufige Volumenregelung
		/*analog_input = analogRead(BASE + 2);
		if (analog_input < 300) {
			set_volume(8);
		}
		else if (analog_input < 450 && analog_input >= 300) {
			set_volume(10);
		}
		else {
			set_volume(9);
		}
		*/
		delay_no_itr(100);
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

	signal(SIGINT, siginthandler); // Deklaration des Signal-H�ndlers

	pinMode(PLAY, INPUT); //GPIO-Freigabe
	pinMode(VOR, INPUT);
	pinMode(RUECK, INPUT);

	pullUpDnControl(PLAY, PUD_UP); //Freischalten der Pullwiderst�nde
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

		status_LED(4);
		return 0;

	
	}
	else {
		printf("File System Error: Unable to read the given directory or the directory is empty\n");
		status_LED(2);
		return 1;
	}



}