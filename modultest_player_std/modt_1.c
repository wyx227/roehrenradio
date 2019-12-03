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
#include "delay.h"

//Pinbelegung der Tasten
#define PLAY 17
#define VOR 27
#define RUECK 22
#define MODSELECT 14

//globale Variablen
pthread_t t_play,t_stop, t_control, t_monitor; //pthread-Bezeichner aller auszufuehrenden Threads
int song_index = 0; //Index der Playliste
int play = 0; // Variable, ob die Playtaste gedrueckt wird, also ob die Musik gespielt wurde
int firstrun = 0; // Variable, ob es sich um den frischen Start handelt
char *path = "//home//pi//music//"; //vorgegebener Pfad zur Speicherung der Musikdateien
char *usbpath = "//media//usb0//"; //Default-Mounting-Point des USB-Speichergeraetes
int infp, outfp; // Bezeichner fuer die Stdin und Stdout des durch Pipe gestarteten Prozesses
char cmd[128]; // Die an stdin zu sendenden Befehle
char *playlist[32768] = { 0 }; // Char-Array zur Speicherung der Playliste, maximal 32768 Elemente erlaubt, maximaler Index bis 32767
char output[128]; //Stdout des MPG321-Players
int usb_was_plugged = 0; //Speichere, ob ein USB-Stick eingesteckt wurde. 
//int use_usb = 0;

//Funktionsprototypen 
int flank_high(int pin);
void cleanup();
void siginthandler(int sig_num); // Signalhandler beim Beenden, mpg321 zu beenden
int size_playlist();
int read_dir(char *input_path);
int generate_command();
void *play_music();
void *stop_music();
void *control();
//int determine_usb();
void *monitoring_player();



int size_playlist() { // Mit dieser Funktion wird die eigentliche Laenge der Playliste bestimmt. Bei Erstellen der Array-Liste werden alle Elemente mit 0 gef�llt.
	int length = 0;
	for (int i = 0; i < 32768; i++) {
		if (playlist[i] != 0) { // Alle 0 werden nicht gezaehlt
			length++;
		}
	}
	return length;
}

int read_dir(char *input_path) { // Verzeichnis wird gelesen

	DIR *d;
	struct dirent *dir;
	d = opendir(input_path); // Verzeichnis wird gelesen
	int ind = 0;
	//int zustand;
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (dir->d_type == 8) { //8 fuer Dateien, nicht fuer Ordner
				if (strstr(dir->d_name, ".mp3")!=NULL) {
					playlist[ind] = dir->d_name; // Die Playliste werden mit den Dateinamen gefuellt.
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
		return 2;
	}
	else {
		return 0;
	}
	
	
}

int generate_command() { // Der Play-Befehl wird generiert mit dem aktuellen Index.
	char *path_input;
	char *merk[32768] = { 0 };
	char cmd_copy[128];
	int zustand=usb_was_plugged;
	if(usb_was_plugged){
		path_input = usbpath; 
	}
	else {
		path_input = path;
	}
	
	if(read_dir(path_input)!=0){
		printf("File Error!\n");
		return 1;
	}
		
	strcpy(cmd, "LOAD ");
	if (usb_was_plugged) {

		strcat(cmd, usbpath);
	}
	else {
		strcat(cmd, path);
	}
	strcpy(merk, playlist);
	strcat(cmd, playlist[song_index]);
	strcpy(cmd_copy, cmd);
	strcat(cmd, "\n");
	return 0;
}

void *play_music() { // Beim Betaetigen der Play-Taste und wenn das Programm frisch ausgefuehrt wird, wird das erste Lied gespielt
	
	while (1) {
		//int cmdgen_succeed;
		if (play == 1 && firstrun == 0) {
			firstrun = 1;
			if (generate_command() == 0) {
				write(infp, cmd, 128);
			}
			
			
			
			
		}
	}
	return 0;
} 

void *stop_music(){ // Beim Bestaetigen der Play-Taste und wenn bereit eine Musik spielt, wird das Wiedergeben pausiert.
	while (1) {
		if (play == 0 && firstrun != 0) {
			
			write(infp, "PAUSE\n", 128);

		}
	}
	return 0;
}


void *control() { //Dieser Thread liest alle Tasten und Potis ueber GPIO-Eingaengen
	int cmdgen_succeed;
	while (1) {
		if (flank_high(PLAY) == HIGH && !(flank_high(VOR)) && !(flank_high(RUECK))) {
			//int cmdgen_succeed;
			// Bei Betaetigung der Play-Taste wird der Zustand zum Spielen bzw. Pausieren gewechselt.
			if (play == 0) {
				play++;
				delay_no_itr(500);
			}
			else {
				play--;
			}
			if (generate_command() == 0) {
				write(infp, cmd, 128);
			}

		}
		if (flank_high(VOR) == HIGH && firstrun != 0 && !(flank_high(PLAY)) && !(flank_high(RUECK))) { //Bei Betautigung der VOR-Taste wird der Index der Playliste um 1 inkrementiert, wenn das Ende nicht erreicht wurde.
			
			if (song_index < size_playlist()-1) {
				song_index++;
				delay_no_itr(500);
			}
			if (generate_command() == 0) {
				write(infp, cmd, 128);
			}

		}
		if (flank_high(RUECK) == HIGH && firstrun != 0 && !(flank_high(VOR)) && !(flank_high(PLAY)) ) { //Ebenso fuer die RUECK-Taste
			int cmdgen_succeed;
			if (song_index > 0) {
				song_index--; 
				delay_no_itr(500);
			}
			if (generate_command() == 0) {
				write(infp, cmd, 128);
			}
		}

		if (digitalRead(MODSELECT) == HIGH) { //Wechsel auf USB-Stick
			usb_was_plugged = 1;
			//delay_no_itr(500);
		}
		else {
			usb_was_plugged = 0;
		}
		//set_volume(25); Hier die Funktion fuer die Volumenregelung
		
	}
	return 0;
}

void *monitoring_player() {
	while (1) {
		int usb_was_plugged_last = usb_was_plugged;
		if (read(outfp, output, 128) == 0) {
			if (strcmp(output, "@P 3") == 0) { //Wenn die Wiedergabe mit dem aktuellen Lied beendet ist, wird es zum naechsten Lied gesprungen.
				song_index++;
				if (generate_command() == 0) {
					write(infp, cmd, 128);
				}
			}
			if (strstr(output, "@I")!=NULL) { //Unterbrechung, da Datei nicht gefunden wird.
				song_index = 0;
				firstrun = 0;

			}
		}
		if ( usb_was_plugged == !usb_was_plugged_last){//Nach Wechsel PLAY druecken
			write(infp,"PAUSE",128);
			/*if(determine_usb()){
				printf("File error\n");
			};*/
			//generate_command();
			//write(infp,cmd,128);
		}

		/*if(opendir(usbpath)==NULL && usb_was_plugged == 1){
			write(infp, "PAUSE",128);
			usb_was_plugged = ~usb_was_plugged;
			//Fehlerausgabe
		}else if(opendir(path) == NULL && usb_was_plugged == 0){
			write(infp, "PAUSE",128);
			usb_was_plugged = ~usb_was_plugged;
			//Fehlerausgabe
		}*/
	}

}

/*int determine_usb() { //Erstimpelementation der Einlesefunktion des USB-Speichers
	if (read_dir(usbpath) == 0) { //Lesen des USB-Mouting-Points und Erstellen einer neuer Playliste
		//strcpy(path, usbpath);
		return 0;
	}
	else {
		return 1;
	}
}*/


int main() {
	if (wiringPiSetupSys() == -1) { // Beim Fehler der GPIO-Freigabe wird das Programm direkt beendet.
		printf("Einrichten WiringPI fehlgeschlagen!\n");
		return 1;
	}

	signal(SIGINT, siginthandler); // Deklaration des Signal-H�ndlers
	signal(SIGSEGV, siginthandler);
	pinMode(PLAY, INPUT); //GPIO-Freigabe
	pinMode(VOR, INPUT);
	pinMode(RUECK, INPUT);
	pinMode(MODSELECT, INPUT);

	pullUpDnControl(PLAY, PUD_UP); //Freischalten der Pullwiderst�nde
	pullUpDnControl(VOR, PUD_UP);
	pullUpDnControl(RUECK, PUD_UP);
	pullUpDnControl(MODSELECT, PUD_UP);

	/**while (read_dir(path) != 0 && read_dir(usbpath) != 0) {
		prinf("Please insert your USB Device or copying some file into the given folder\n");
		delay_no_intr(1000);
	}**/
	//session_start();
	
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


	return 0;

}