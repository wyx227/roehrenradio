#pragma once
#define alloca(x)  __builtin_alloca(x)

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <math.h>

struct schaltungsparameter {
	int R;
	int poti_max;
	int poti_min;
	int u_0;
	int u_ref;
	int bit;
	double drehwinkel;
};

struct lookup_table {
	int adu_linear[1024];
	int adu_log[1024];
};
//Funktion zur Volumensteuerung

void set_volume(int volume){
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "Default";
	const char *selem_name = "Speaker";

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

	snd_mixer_close(handle);
}

int poti_linear(double winkel,double m) {

	int adu_linear = floor(m * winkel + 200.0);
	return adu_linear;

}

int poti_log(double winkel, double k) {
	int adu_log = floor(4.25 * pow(10, 6) * winkel + 200.0);
	return adu_log;
}

double get_parameter(int type,struct schaltungsparameter schaltung) {
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

	radio.bit = 10;
	radio.drehwinkel = 270 * 3.14/ 180;
	radio.poti_max = 20000000;
	radio.poti_min = 200;
	radio.R = 1000000;
	radio.u_0 = 5;
	radio.u_ref = 5;

	for (int i = 0; i < pow(2, radio.bit); i++) {
		double angle = radio.drehwinkel * i / pow(2, radio.bit);
		tabelle.adu_linear[i] = poti_linear(angle,get_parameter(0, radio));
		tabelle.adu_log[i] = poti_log(angle,get_parameter(1,radio));
	}

	return tabelle;
}


int linearisieren(int adu_wert) {
	struct lookup_table tabelle = filling();
	for (int i = 0; i < 1024; i++) {
		if (tabelle.adu_log[i] == adu_wert) {
			return tabelle.adu_linear[i];
		}
	}
}