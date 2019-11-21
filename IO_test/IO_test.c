#include <stdio.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include <pthread.h>
#include "i2cdisplay.h"
#include "flanken.h"
#include "anschluss.h"
#include "meldung.h"
#include "delay.h"
#include "poti_aufbereitung.h"



//Gelb = GREEN + RED

pthread_t t_schleife;
int fatal_pressed = 0;
int datenfehler_pressed = 0;
int restart_pressed = 0;

void use_lcd(char *L1) {
	//ClrLcd();
	lcdLoc(LINE1);
	typeln("");
	typeln(L1);

}

void error_display(int error_code) {
	switch (error_code) {
	case 1:
		
		digitalWrite(LED_GREEN, HIGH);
		break;
	case 2:
		if (datenfehler_pressed == 0) {
			digitalWrite(LED_RED, HIGH);
			digitalWrite(LED_GREEN, HIGH);
		}
		else {
			digitalWrite(LED_RED, LOW);
			digitalWrite(LED_GREEN, LOW);
		}
		break;
	case 3:
		if (fatal_pressed == 0) {
			digitalWrite(LED_RED, HIGH);
		}
		else {
			digitalWrite(LED_RED, LOW);
		}
		
		break;
	case 4:
		if (restart_pressed == 0) {
			digitalWrite(LED_BLUE, HIGH);
		}
		else {
			digitalWrite(LED_BLUE, LOW);
		}
		break;
	}
}



void *read_input() {
	int lauf = 0;
	char *volumen;
	while (1) {
		printf("-----------Debugprogramm startet-----------------\n");
		printf("---Das ist der %d.te Durchlauf\n",lauf);
		lauf++;

		if (flank_high(PLAY) == HIGH) {
			printf(play_pressed);
			use_lcd(play_pressed);
			
		}

		if (flank_high(VOR) == HIGH) {
			printf(vor_pressed);
			use_lcd(vor_pressed);
		}

		if (flank_high(RUECK) == HIGH) {
			printf(rueck_pressed);
			use_lcd(rueck_pressed);
		}

		if (flank_high(FATAL_ERROR_SIMULATOR) == HIGH) {
			printf(fatal_fehler);
			use_lcd(fatal_fehler);
			!fatal_pressed;
		}

		if (flank_high(FILE_ERROR_SIMULATOR) == HIGH) {
			printf(datei_fehler);
			use_lcd(datei_fehler);
			!datenfehler_pressed;
		}

		if (flank_high(RESTART_SIMULATOR) == HIGH) {
			printf(restart_required);
			use_lcd(restart_required);
			!restart_pressed;
		}

		printf("ADU-Wert, nicht linearisiert: %d, linearisiert: %d\n", analogRead(SPI_CHAN + 2), linearisieren(analogRead(SPI_CHAN + 2)));
		lcdLoc(LINE2);
		itoa(linearisieren(analogRead(SPI_CHAN + 2)), volumen, 10);
		typeln(strcat("Messwert: ", volumen));
		delay_no_itr(500);
	}
}




int main() {
	//int play = 0;

	if (wiringPiSetupSys() == -1) {
		return 1;
	}

	if (mcp3004Setup(BASE,SPI_CHAN) == -1) {
		return 1;
	}

	pinMode(PLAY, INPUT); //GPIO-Freigabe
	pinMode(VOR, INPUT);
	pinMode(RUECK, INPUT);

	pullUpDnControl(PLAY, PUD_UP); //Freischalten der Pullwiderstände
	pullUpDnControl(VOR, PUD_UP);
	pullUpDnControl(RUECK, PUD_UP);


	digitalWrite(MUX1, LOW);
	digitalWrite(MUX2, LOW);
	digitalWrite(MUX3, HIGH);

	fd = wiringPiI2CSetup(I2C_ADDR);

	lcd_init(); // setup LCD

	pthread_create(&t_schleife,NULL,read_input,NULL);
	pthread_join(t_schleife,NULL);



	return 0;

}