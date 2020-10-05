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

int midiInit(){

	activePID = 0;

	//Spusti se prikaz
	midifp = popen("aplaymidi -l | grep MIDIControl", "r");
	if(midifp == NULL){
		printf(ERROR "Nepodarilo se nacist MIDI porty. Vyzkousejte spustitelnost 'aplaymidi -l | grep MIDIControl'\n");
		return 0;
	}

	char buff[100];
	fgets(buff, sizeof(buff), midifp);

	if(strlen(buff) <= 0){
		printf(ERROR "Nepodarilo se nacist MIDI porty. Je zarizeni pripojene?\n");
		return 0;
	}

	int p1 = 0, p2 = 0;

	sscanf(buff, "%d:%d", &p1, &p2);
	sprintf(midiport, "%d:%d", p1, p2);

	pclose(midifp);

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

	//Nacte se cesta k souboru
	strcpy(dir, parameters[2]);
	strcat(dir, "/");
	strcat(dir, songname);
	strcat(dir, ".mid");

	char buff[255];

	sprintf(buff, "aplaymidi -p %s %s &", midiport, dir);

	//Spusti se prikaz
	system(buff);

	/*midifp = popen(buff, "r");
	if(midifp == NULL){
		printf(ERROR "Nepodarilo se spustit prehravani. Vyzkousejte spustitelnost '%s'\n", buff);
		return 0;
	}
	
	fgets(buff, sizeof(buff), midifp);

	sscanf(buff, "[%*d] %d", &activePID);

	if(strlen(buff) > 15 ||activePID == 0){
		printf(ERROR "Chyba prehravani. Hlaska: %s\n", buff);
		return 0;
	}

	pclose(midifp);*/

	usleep(100000);

	char msg[strlen(songname)+7];
    msg[0] = INTERNAL_COM;
    msg[1] = INTERNAL_COM_PLAY;
	sprintf(&msg[2], "%s.mid", songname);
	printf("%s", songname);
	sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, strlen(songname)+6);


   	trackStatus = 1;
   	
	return 1;


}

int midiStop(){

	if(!aliveMain){
		printf(ERROR "S hlavni jednotkou neni navazano spojeni!\n");
		return 0;
	}

	system("killall aplaymidi arecordmidi");

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
	
	char buff[255];

	sprintf(buff, "arecordmidi -p %s %s &", midiport, dir);

	//Spusti se prikaz
	system(buff);
	/*midifp = popen(buff, "r");
	if(midifp == NULL){
		printf(ERROR "Nepodarilo se spustit nahravani. Vyzkousejte spustitelnost '%s'\n", buff);
		return 0;
	}
	
	fgets(buff, sizeof(buff), midifp);

	sscanf(buff, "[%*d] %d", &activePID);

	if(strlen(buff) > 15 ||activePID == 0){
		printf(ERROR "Chyba nahravani. Hlaska: %s\n", buff);
		return 0;
	}

	pclose(midifp);*/

	usleep(100000);

 	char msg[strlen(songname)+7];
    msg[0] = INTERNAL_COM;
    msg[1] = INTERNAL_COM_REC;
	sprintf(&msg[2], "%s.mid", songname);
	printf("%s", songname);
	sendMsg(ADDRESS_PC, ADDRESS_OTHER, 1, INTERNAL, msg, strlen(songname)+6);

	trackStatus = 3;

	return 1;

}
