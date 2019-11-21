#pragma once
#include <wiringPi.h>

int flank_high(int pin) {
	int old_level = 0;
	int new_level;


	new_level = digitalRead(pin);
	if (!old_level && new_level) {
		return HIGH;
	}
	old_level = new_level;
	return;
}