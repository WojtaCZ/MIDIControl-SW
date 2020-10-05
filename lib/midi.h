#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"

// -1 - chyba souboru
// 0 - nic nehraje
// 1 - prehrava se
// 2 - pozastaveno
// 3 - nahravani
int trackStatus;
//File pro spousteni linuxovych prikazu
FILE *midifp;
//MIDI Port (XX:XX)
char midiport[7];
//Cislo aktivniho procesu
int activePID;

int midiPlay(char songname[]);
int midiStop();
int midiRec(char songname[]);

#endif