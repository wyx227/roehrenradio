#pragma once

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include "flanken.h"
#include "popen.h"
#include "volumen.h"
#include "handling.h"
#include "delay.h"
#include "status_display.h"
#include <unistd.h>

void *monitoring();
void *play_music();
void *stop_music();
void *control();