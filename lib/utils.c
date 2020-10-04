#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include "communication.h"

int getSongsStr(char * str){
	char *files[500];
	int count;
	getDirContents(parameters[2], &files, &count);

	strcpy(str, files[0]);
	strcat(str, "\n");

	for(int i = 1; i < count; i++){
		strcat(str, files[i]);
		strcat(str, "\n");
	}

	return 1;
}

int getConfig(){
	//Promenne funkce
	int paramCount = 0, lineCount = 0;
	char line[256], name[256], value[256];

	//Zjisteni pristupu k souboru
	if(access("config.conf", F_OK | R_OK) != -1){
		FILE *confFile;
		confFile = fopen("config.conf", "r"); 
		if(confFile != NULL){
			//Cteni souboru po linkach
			while(fgets(line, 256, confFile) != NULL){

				//Pocet radku pro jednodussi zjjisteni chyby
				lineCount++;

				//Pokud se jedna o komentar nebo enter, rovnou se preskoci
				if(line[0] == '#' || line[0] == '\n' || line[0] == '[') continue; 

				//Pokud je na radku mene nez 2 parametry, ukaze se chyba, u vice jak 2 se zbyle ignoruji
				if(sscanf(line, "%s %s", name, value) != 2){
					//Odstrani se newlnine
					line[strlen(line)-1] = 0;
					//Vypise se cislo chybneho radku i s radkem
					printf(ISSUE "Spatny zapis v konfiguracnim souboru na radku %d. Radek bude ignorovan.\n         -> \"\e[2m%s\e[22m\"\n", lineCount,line);
				}else{

					paramCount++;

					//Roztridi parametry do prislusnych bunek ( bohuzel Ccko neumi indexovat stringem :( )
					if(!strcmp("IDENT", name)){
						strcpy(parameters[0], value);
					}else if(!strcmp("SERBAUD", name)){
						strcpy(parameters[1], value);
					}else if(!strcmp("MIDIPATH", name)){
						strcpy(parameters[2], value);
					}else{
						//Odstrani se newlnine
						line[strlen(line)-1] = 0;
						//Vypise se cislo chybneho radku i s radkem
						printf(ISSUE "Neznamy parametr v konfiguracnim souboru na radku %d. Radek bude ignorovan.\n         -> \"\e[2m%s\e[22m\"\n", lineCount,line);
						//Odecte se chybny parametr
						paramCount--;
					}
				}
			}
		}else{
			printf(ERROR "Chyba pri otevirani souboru config.conf!\n");
			return 0;
		}
	}else{
		printf(ERROR "Chyba pri nacitani konfigurace, zkontrolujte existenci souboru config.conf a povoleni cteni!\n");
		return 0;	
	}
	printf(OK "Konfigurace nactena uspesne, zjisteno %d parametru.\n", paramCount);
	return 1;
}

int getDirContents(char * directory, char *(*filearray)[500], int * count){
	DIR *d;
	int i = 0;
	struct dirent *dir;
	d = opendir(directory);
	if(d){
    	while((dir = readdir(d)) != NULL){
    		if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
    			(*filearray)[i] = dir->d_name;
    			i++;
    		}
   		}
		closedir(d);
	}

	*count = i;

	return(0);
}

void sendTime(){
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);
  	char msg[8] = {INTERNAL_COM, INTERNAL_COM_SET_TIME, tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon+1, tm.tm_year-100};
  	sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, 8);
}