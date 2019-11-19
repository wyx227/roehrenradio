#include <wiringPi.h>

void delay_no_itr(unsigned long duration)  //Aufruf der Delay-Funktion ohne Interrupt, um Effizienz zu erh�hen.
{
	unsigned long current = millis();
	if (millis() - current == duration) {
		current = millis();
	}
}