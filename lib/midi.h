#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// -1 - chyba souboru
// 0 - nic nehraje
// 1 - prehrava se
// 2 - pozastaveno
int trackStatus;

int midiPlay(char songname[]);
void *midiRTParser(void * args);

struct midifile{
	uint16_t format;
	uint16_t tracks;
	uint16_t division;
	uint32_t tempo;
	uint8_t deltaType;
	//Zavysle na deltaType (bud v ticich na ctvrtinovou notu nebo v ticich na sekundu)
	uint32_t timeMultiplier;
	unsigned long trackSize;
};

#endif