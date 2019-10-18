#include "midi.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Pointer na midi soubor
FILE *midifp;

int midiPlay(char songname[]){

	//Struktura s udaji o souboru
	struct midifile playfile;

	char dir[255];

	//Soubor se otevre
	strcpy(dir, parameters[2]);
	strcat(dir, "/");
	strcat(dir, songname);
	midifp = fopen(dir, "rb");

	if(midifp == NULL){
		printf(ERROR "Zadany soubor %s neexistuje!\n", dir);
		return 0;
	}

	//Vytvori se buffer do ktereho se pote zapise cast souboru
	unsigned char headerBuffer[200];

	//Precte se zacatek souboru a najde se v nem hlavicka
	fread(headerBuffer, 190, 1, midifp);

	for(int k = 0; k < 190; k++){
		if(headerBuffer[k] == 0) headerBuffer[k] = 1;
	}

	unsigned char *trackPointer = strstr(headerBuffer, "MTrk");
	int trackIndex = (trackPointer - headerBuffer);

	unsigned char *headerPointer = strstr(headerBuffer, "MThd");
	int headerIndex = (headerPointer - headerBuffer);

	if(headerPointer == NULL){
		printf(ERROR "Soubor neobsahuje hlavicku MIDI souboru 'MThd'!\n");
		return 0;
	}

	//Skoci se na hlavicku
	fseek(midifp, headerIndex, SEEK_SET);

	//Nactou se zakladni data (format...)
	char data[255];
	fgets(data, 15, midifp);
	playfile.format = ((data[8]<<8) | (data[9]) & 0xff);
	playfile.tracks = ((data[10]<<8) | (data[11] & 0xff));
	playfile.division = ((data[12]<<8) | (data[13] & 0xff));

	if(playfile.format != 0){
		printf(ERROR "Soubor je typu %d (vicestopy), dostupne je pouze prehravani souboru typu 0 (jednostopy)!\n", playfile.format);
		return 0;
	}

	fseek(midifp, trackIndex+4, SEEK_SET);
	fgets(data, 5, midifp);

	playfile.trackSize = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]));	

	printf("%d  %d  %d %ld\n",playfile.format, playfile.tracks, playfile.division, playfile.trackSize);
	
	fclose(midifp);


}

int midiParser(uint8_t data){

}