#include <stdio.h>
#include <math.h>
#include <wiringPi.h>
#include <mcp3004.h>

#define MUX1 1
#define MUX2 2
#define MUX3 3

#define BASE 200
#define SPI_CHAN 0

struct schaltungsparameter {
	double R;
	double poti_max;
	double poti_min;
	double u_0;
	double u_ref;
	double bit;
	double drehwinkel;
};

struct lookup_table {
	int adu_linear[1024];
	int adu_log[1024];
};

struct lookup_table tabelle_global;

int poti_linear(double winkel, double m, struct schaltungsparameter schaltung);
int poti_log(double winkel, double k, struct schaltungsparameter schaltung);
double get_parameter(int type, struct schaltungsparameter schaltung);
struct lookup_table filling();
int linearisieren(int adu_wert);

int main() {
	if (wiringPiSetupSys() == -1) {
		printf("wiringPi Setup fehlgeschlagen\n");
		return 1;
	}

	if (!mcp3004Setup(BASE, SPI_CHAN)) {
		printf("ADU-Initialisierung fehlgeschlagen\n");
		return -1;
	}

	digitalWrite(MUX1, LOW);
	digitalWrite(MUX2, LOW);
	digitalWrite(MUX3, HIGH);

	tabelle_global = filling();

	while (1) {
		int poti = analogRead(BASE + 2);
		printf("Nicht linearisiert: %i Linearisiert: %i\n", poti, linearisieren(poti));
		delay(500);
	}

	return 0;
}

int poti_linear(double winkel, double m, struct schaltungsparameter schaltung) {

	int adu_linear = floor(schaltung.R*schaltung.u_0*(pow(2.0, schaltung.bit) / schaltung.u_ref) / (schaltung.R + m * winkel + schaltung.poti_min));
	return adu_linear;

}

int poti_log(double winkel, double k, struct schaltungsparameter schaltung) {
	int adu_log = floor(schaltung.R*schaltung.u_0*(pow(2.0, schaltung.bit) / schaltung.u_ref) / (schaltung.R + exp(k* winkel) * schaltung.poti_min));
	return adu_log;
}

double get_parameter(int type, struct schaltungsparameter schaltung) {
	double para;
	switch (type) {
	case 0: //linear
		para = (schaltung.poti_max - schaltung.poti_min) / schaltung.drehwinkel;
		break;
	case 1: //log
		para = log(schaltung.poti_max / schaltung.poti_min) / schaltung.drehwinkel;
		break;
	default:
		break;
	}
	return para;
}

struct lookup_table filling() {
	struct schaltungsparameter radio;
	struct lookup_table tabelle;

	radio.bit = 10.0;
	radio.drehwinkel = 270.0 * 3.14 / 180.0;
	radio.poti_max = 20000000.0;
	radio.poti_min = 200.0;
	radio.R = 1000000.0;
	radio.u_0 = 3.3;
	radio.u_ref = 3.3;

	for (int i = 0; i < pow(2.0, radio.bit); i++) {
		double angle = radio.drehwinkel*i / pow(2.0, radio.bit);
		tabelle.adu_linear[i] = poti_linear(angle, get_parameter(0, radio), radio);
		tabelle.adu_log[i] = poti_log(angle, get_parameter(1, radio), radio);
	}

	return tabelle;
}

int linearisieren(int adu_wert) {
	for (int i = 0; i < 1024; i++) {
		if (abs(adu_wert - tabelle_global.adu_log[i]) < 0.1*adu_wert) {
			return tabelle_global.adu_linear[i];
		}
	}
}