#include <stdio.h>
#include <wiringPi.h>

#define PLAY 17


int main() {
	if (wiringPiSetupSys() == -1) {
		printf("Einrichten WiringPI fehlgeschlagen!");
		return 1;
	}
	pinMode(PLAY, INPUT);
	pullUpDnControl(PLAY, PUD_UP);
	int druck = 0;l
	while (1) {
		int old_level = 0;
		int new_level;
		

		new_level = digitalRead(PLAY);
		if (!old_level && new_level) {
			//printf("Play!\n");
			druck++;
			printf("Es wurde %d mal gedrueckt.\n", druck);
			delay(50);
		}
		
		old_level = new_level;
		
	}

	return 0;
}

