#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../lib/utils.h"
#include "../lib/serial.h"
#include "../lib/midi.h"
#include "../lib/commands.h"

char version[] = "1.0.0";
char cmd[255], name[255];

char **character_name_completion(const char *, int, int);
char *character_name_generator(const char *, int);

int main(int argc, char const *argv[]){

	printf("\n\e[1mZapina se MIDI controll verze %s\e[0m\n(c) Vojtech Vosahlo 2019\n\n\n", version);

	if(!getConfig()) return 0;

	//if(!serialInit(parameters[0], parameters[1])) return 0;

	while(1){

		rl_attempted_completion_function = character_name_completion;

		char * line = readline("> ");
        if(!line) break;
        if(*line) add_history(line);
        
        sscanf(line, "%s %s", cmd, name);
        if(!strcmp(cmd, "play")){
				midiPlay(name);
		}

        free(line);
	}

	return 0;
}

char ** character_name_completion(const char *text, int start, int end){
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, character_name_generator);
}

char * character_name_generator(const char *text, int state){
    static int list_index, len;
    char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = maincmds[list_index++].cmd)) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}