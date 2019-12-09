#include "global_variables.h"
#include "threads.h"


void *monitoring() { //Überwacht, ob das Spielen vom aktuellen Lied beendet ist.
	while (1) {
		if (read(outfp, output, 128)) {
			if (strstr(output, "@P 3")) {
				song_index++;
				generate_command();
				if (write(infp, cmd, 128) == -1) {
					status_LED(3);
					exit(EXIT_FAILURE);
				}
			}
			if (strstr(output, "@I ")) {
				printf("Error reading file, please check your media\n");
				status_LED(2);
				exit(EXIT_FAILURE);
			}
		}
		else {
			printf("Error reading Stdout, critical error!\n");
			status_LED(3);
			exit(EXIT_FAILURE);
		}


	}
}

void *play_music() { // Beim Betätigen der Play-Taste und wenn das Programm frisch ausgeführt wird, wird das erste Lied gespielt
	while (1) {
		if (play == 1 && firstrun == 0) {
			firstrun = 1;
			generate_command();
			if (write(infp, cmd, 128) == -1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}

		}
	}
	return 0;
}

void *stop_music() { // Beim Bestätigen der Play-Taste und wenn bereit eine Musik spielt, wird das Wiedergeben pausiert.
	while (1) {
		if (play == 0 && firstrun != 0) {

			if (write(infp, "PAUSE\n", 128) == -1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}

		}
	}
	return 0;
}


void *control() { //Dieser Thread liest alle Tasten und Potis über GPIO-Eingängen
	while (1) {
		if ((flank_high(PLAY) == HIGH) && !(flank_high(VOR)) && !(flank_high(RUECK))) { // Bei Betätigung der Play-Taste wird der Zustand zum Spielen bzw. Pausieren gewechselt.
			if (play == 0) {
				play++;
				delay_no_itr(500);
			}
			else {
				play--;
			}

		}
		if (flank_high(VOR) == HIGH && firstrun != 0 && !(flank_high(PLAY)) && !(flank_high(RUECK))) { //Bei Betätigung der VOR-Taste wird der Index der Playliste um 1 inkrementiert, wenn das Ende nicht erreicht wurde.
			if (song_index < size_playlist() - 1) {
				song_index++;
				delay_no_itr(500);
			}
			generate_command();
			if (write(infp, cmd, 128) == -1) {
				status_LED(3);
				exit(EXIT_FAILURE);
			}


		}
		if (flank_high(RUECK) == HIGH && firstrun != 0 && !(flank_high(PLAY)) && !(flank_high(VOR))) { //Ebenso für die RÜCK-Taste
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
		//set_volume(25);

	}
	return 0;
}