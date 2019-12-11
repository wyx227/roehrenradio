
#define alloca(x)  __builtin_alloca(x)

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

//Funktion zur Volumensteuerung

void set_volume(int volume){ //Default-Einstellung fuer Onboard-Soundkarte
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "PCM";

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volume*800 - 10000); //Hier kann man die Volumen anpassen. Der Poti verhaelt sich sehr seltsam.

	snd_mixer_close(handle);
}
