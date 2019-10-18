#ifndef CMD_H
#define CMD_H

typedef struct{
	char* cmd;
	char* desc;
} COMMAND;

COMMAND maincmds[] = {
{"play", "Prehraje zvoleny MIDI soubor"}
};

#endif