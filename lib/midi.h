#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>

int midiPlay(char songname[]);
int midiParser(uint8_t data);

struct midifile{
	uint16_t format;
	uint16_t tracks;
	uint16_t division;
	unsigned long trackSize;
};

#endif