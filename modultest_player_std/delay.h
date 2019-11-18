#include <wiringPi.h>
//Verzögern ohne Verwendung der Delay-Funktion, um Effizienz zu erhöhen
void delay_no_itr(unsigned long duration) {
	unsigned long current = millis();

	if (millis() - current >= duration) {
		current = millis();
	}

}