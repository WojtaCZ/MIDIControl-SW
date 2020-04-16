#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <arpa/inet.h>
#include "main.h"
#include "../lib/utils.h"
#include "../lib/serial.h"
#include "../lib/midi.h"
#include "../lib/commands.h"
#include "../lib/communication.h"

int fileCount;
//char *files[500];

int main(int argc, char const *argv[]){

	printf("\n\e[1mZapina se MIDI controll verze %s\e[0m\n(c) Vojtech Vosahlo 2019\n\n\n", version);
	line = (char *) malloc(500);
	consoleLine = (char *) malloc(100);

	trackStatus = 0;

	if(!getConfig()) return 0;
	
	//getDirContents("/home/vojtech/Dokumenty/MIDI_soubory", &files, &fileCount);

	if(!serialInit(parameters[0], parameters[1])) return 0;

	if(!devComInit()) return 0;

	//Odesle se aktualni cas
	sendTime();

	while(1){

		rl_attempted_completion_function = cmd_completition;
		

		switch(trackStatus){
			case 0: consoleLine = "\e[2mMIDIctrl (\e[1mPripraveno\e[0m\e[2m)\e[0m  > ";
			break;
			case 1: consoleLine = "\e[2mMIDIctrl (\e[5m\e[92m\e[1mPrehravani\e[0m\e[2m)\e[0m  > ";
			break;
			case 2: consoleLine = "\e[2mMIDIctrl (\e[91m\e[1mZastaveno\e[0m\e[2m)\e[0m  > ";
			break;
			default: consoleLine = "\e[2mMIDIctrl (\e[1mPripraveno\e[0m\e[2m)\e[0m  > ";
			break;
		}

		for(int i = 0; i < fileCount; i++) printf("%s\n", files[i]);

		line = readline(consoleLine);
        if(!line) break;
        if(*line) add_history(line);        

        sscanf(line, "%s %s", cmd, name);
        if(!strcmp(cmd, "play")){
			midiPlay(name);
		}

		if(!strcmp(cmd, "record")){
			midiRec(name);
		}

		if(!strcmp(cmd, "pause")){
				trackStatus = 2;
		}

		if(!strcmp(cmd, "stop")){
				midiStop();
		}

		if(!strcmp(cmd, "resume")){
				trackStatus = 1;
		}

		if(!strcmp(cmd, "ls")){
			/*char *fil[500];
			int count;
			getDirContents(parameters[2], fil, &count);

			for(int i = 0; i < count; i++){
				printf("%s\n", fil[i]);
			}*/

			char * songs = (char*)malloc(500);
			getSongsStr(songs);
			printf("%s", songs);
		}

		if(!strcmp(cmd, "exit")){
				break;
		}

        free(line);
	}

	return 0;
}

char ** cmd_completition(const char *text, int start, int end){
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, cmd_completition_gen);
}

char * cmd_completition_gen(const char *text, int state){
    static int list_index, len;
    char *name;
    name = (char *) malloc(60);

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }		


    if(strstr(rl_line_buffer, "play")){

    	DIR *d;
		int i = 2;
		struct dirent *dir;
		d = opendir("/home/vojtech/Dokumenty/MIDI_soubory");
		files[0].cmd = ".";
		files[1].cmd = "..";
	    //files[1].desc = NULL;
		if(d){
	    	while((dir = readdir(d)) != NULL){
	    		if(strcmp(dir->d_name, ".") != 0 || strcmp(dir->d_name, "..") != 0){
	    			files[i].cmd = dir->d_name;
	    			files[i].desc = NULL;
	    			i++;
	    		}
	    		
	   		}
			closedir(d);
		}

		files[i].cmd = NULL;
	    files[i].desc = NULL;

    	while ((name = files[list_index++].cmd)) {
        	if (strncmp(name, text, len) == 0) {
            	return strdup(name);
        	}
    	}
    }else{
    	while ((name = maincmds[list_index++].cmd)) {
        	if (strncmp(name, text, len) == 0) {
            	return strdup(name);
        	}
    	}
    }
   

    return NULL;
}
