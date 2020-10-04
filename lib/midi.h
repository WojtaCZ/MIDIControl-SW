#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// -1 - chyba souboru
// 0 - nic nehraje
// 1 - prehrava se
// 2 - pozastaveno
// 3 - nahravani
int trackStatus;

int midiPlay(char songname[]);
int midiStop();
int midiRec(char songname[]);

#endif