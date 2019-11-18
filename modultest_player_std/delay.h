#include <wiringPi.h>
//Verz�gern ohne Verwendung der Delay-Funktion, um Effizienz zu erh�hen
void delay_no_itr(unsigned long duration) {
	unsigned long current = millis();

	if (millis() - current >= duration) {
		current = millis();
	}

}