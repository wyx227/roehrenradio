#include <wiringPi.h>

void delay_no_itr(unsigned long duration) {
	unsigned long current = millis();
	if (millis() - current == duration) {
		current = millis();
	}
}