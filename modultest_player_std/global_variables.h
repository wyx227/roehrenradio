#pragma once
#include <pthread.h>
//Pinbelegung der Tasten
#define PLAY 17
#define VOR 27
#define RUECK 22
//globale Variablen
pthread_t t_play, t_stop, t_control, t_monitor; //pthread-Bezeichner aller auszuführenden Threads
int song_index = 0; //Index der Playliste
int play = 0; // Variable, ob die Playtaste gedrückt wird, also ob die Musik gespielt wurde
int firstrun = 0; // Variable, ob es sich um den frischen Start handelt
char *path = "//home//pi//music//"; //vorgegebener Pfad zur Speicherung der Musikdateien
int infp, outfp; // Bezeichner für die Stdin und Stdout des durch Pipe gestarteten Prozesses
char cmd[255]; // Die an stdin zu sendenden Befehle
char output[255]; // Stdout der MPG321
char *playlist[32768] = { 0 }; // Char-Array zur Speicherung der Playliste, maximal 32768 Elemente erlaubt, maximaler Index bis 32767