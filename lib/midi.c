#include "midi.h"
#include "utils.h"
#include "serial.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>
#include <math.h>
#include "communication.h"

//Zapne debug pro dekodovani/kodovani MIDI souboru
//#define MIDI_DEBUG

int midiInit(){
	char * buff = system("aplaymidi -l | grep MIDIControl");
	int p1 = 0, p2 = 0;
	sscanf("%d:%d", &p1, &p2);
	printf("Port = %d:%d", p1, p2);
	return 1;

}

int midiPlay(char songname[]){

	if(!aliveMain){
		printf(ERROR "S hlavni jednotkou neni navazano spojeni!\n");
		return 0;
	}

	//Kontrola zda uz se neprehrava/nenahrava
	if(trackStatus == 1 || trackStatus == 3) return 0;

	trackStatus = 0;

	char dir[255];

	//Soubor se otevre
	strcpy(dir, parameters[2]);
	strcat(dir, "/");
	strcat(dir, songname);
	strcat(dir, ".mid");

 	char msg[strlen(songname)+7];
    msg[0] = INTERNAL_COM;
    msg[1] = INTERNAL_COM_PLAY;
	sprintf(&msg[2], "%s.mid", songname);
	printf("%s", songname);
	sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, strlen(songname)+6);

	usleep(100000);

	//system("");

   	trackStatus = 1;
   	
	return 1;


}

int midiStop(){

	if(!aliveMain){
		printf(ERROR "S hlavni jednotkou neni navazano spojeni!\n");
		return 0;
	}

	//Pokud se udela stop pri prehravani
	if(trackStatus == 1){
		char msg[] = {INTERNAL_COM, INTERNAL_COM_STOP};
		sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, 2);

		
	}else if(trackStatus == 3){
		char msg[] = {INTERNAL_COM, INTERNAL_COM_STOP};
		sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, 2);

		
	}

	trackStatus = 0;
	return 1;
}


int midiRec(char songname[]){

	if(!aliveMain){
		printf(ERROR "S hlavni jednotkou neni navazano spojeni!\n");
		return 0;
	}


	//Kontrola zda uz se neprehrava/nenahrava
	if(trackStatus == 1 || trackStatus == 3) return 0;

	char dir[255];

	//Nastaveni cest
	strcpy(dir, parameters[2]);
	strcat(dir, "/");
	strcat(dir, songname);
	strcat(dir, ".mid");
	
	//Pokud soubor existuje
	if(access(dir, F_OK) != -1){
		printf(ERROR "Zadany soubor %s jiz existuje!\n", dir);
		return 0;
	}

 	char msg[strlen(songname)+7];
    msg[0] = INTERNAL_COM;
    msg[1] = INTERNAL_COM_REC;
	sprintf(&msg[2], "%s.mid", songname);
	printf("%s", songname);
	sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, strlen(songname)+6);

	usleep(100000);

	

	return 1;


}
