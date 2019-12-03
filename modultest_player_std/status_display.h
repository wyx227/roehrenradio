#pragma once
#include <stdio.h>
#include <wiringPi.h>

#define POWERON 13 //Green:
#define FILE_ERROR 19 //
#define FATAL_ERROR 26 //

void status_LED(int status_code) {
	switch (status_code) {
	case 1:
		digitalWrite(POWERON,HIGH);
		break;
	case 2:
		digitalWrite(FILE_ERROR,HIGH);
		break;
	case 3:
		digitalWrite(FATAL_ERROR, HIGH);
		break;
	case 4:
		digitalWrite(POWERON, LOW);
	}

}
