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
void midiStop();
int midiRec(char songname[]);
void *midiPlayParser(void * args);
void *midiRecordParser(void * args);
unsigned long long int toVLQ(unsigned long long int dt, int *bytes);

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