#ifndef CMD_H
#define CMD_H

#include <stdio.h>

typedef struct{
	char* cmd;
	char* desc;
} COMMAND;

COMMAND maincmds[] = {
{"play", "Prehraje zvoleny MIDI soubor"},
{"record", "Prehraje zvoleny MIDI soubor"},
{"pause", "Pozastavi prehravani aktualniho MIDI souboru"},
{"stop", "Prehraje zvoleny MIDI soubor"},
{"resume", "Prehraje zvoleny MIDI soubor"},
{"exit", "Vypne program"},
{"help", "Zobrazi napovedu pro prikaz"},
{NULL, NULL}
};

COMMAND files[100];


#endif