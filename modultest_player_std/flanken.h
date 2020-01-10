#include <wiringPi.h>

//Flankenerkennung

int flank_high(int pin) {
	int old_level = digitalRead(pin);
	int new_level;

	delay(50);
	new_level = digitalRead(pin);
	if (!old_level && new_level) {
		return HIGH;
	}
	old_level = new_level;
	return;
}