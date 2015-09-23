
// Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
// Updated 2105 Joshua Besneatte (http://pirates.jairaja.org)

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <libconfig.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

// delay in ms before processing each XTest event
// CurrentTime means no delay
#define DELAY CurrentTime

// protocol for events from the shuttlepro HUD device
//
// ev.type values:
#define EVENT_TYPE_DONE 0
#define EVENT_TYPE_KEY 1
#define EVENT_TYPE_JOGSHUTTLE 2
#define EVENT_TYPE_ACTIVE_KEY 4

// ev.code when ev.type == KEY
#define EVENT_CODE_KEY1 255
// KEY2 257, etc...

// ev.value when ev.type == KEY
// 1 -> PRESS; 0 -> RELEASE

// ev.code when ev.type == JOGSHUTTLE
#define EVENT_CODE_JOG 7
#define EVENT_CODE_SHUTTLE 8

// ev.value when ev.code == JOG
// 8 bit value changing by one for each jog step

// ev.value when ev.code == SHUTTLE
// -7 .. 7 encoding shuttle position

#define PRESS 1
#define RELEASE 2
#define PRESS_RELEASE 3
#define HOLD 4

#define NUM_KEYS 17
#define NUM_SHUTTLES 15
#define NUM_JOGS 2

#define KJS_KEY_DOWN 1
#define KJS_KEY_UP 2
#define KJS_SHUTTLE 3
#define KJS_JOG 4
