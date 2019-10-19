#include "midi.h"
#include "utils.h"
#include <string.h>
#include <pthread.h>

//Zapne debug pro dekodovani/kodovani MIDI souboru
//#define MIDI_DEBUG

//Pointer na midi soubor
FILE *midifp;
pthread_t playerThread;

int midiPlay(char songname[]){

	trackStatus = 0;

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
		trackStatus = -1;
		return 0;
	}

	//Vytvori se buffer do ktereho se pote zapise cast souboru
	unsigned char headerBuffer[1000];

	//Precte se zacatek souboru a najde se v nem hlavicka
	fread(headerBuffer, 998, 1, midifp);

	//strstr konci na \0 take se musi 0 nahradit 1 a ponechat jen poslední
	for(int k = 0; k < 999; k++){
		if(headerBuffer[k] == 0) headerBuffer[k] = 1;
	}

	//Nactou se pozice hlavicky a prvniho tracku
	unsigned char *trackPointer = strstr(headerBuffer, "MTrk");
	int trackIndex = (trackPointer - headerBuffer);

	unsigned char *headerPointer = strstr(headerBuffer, "MThd");
	int headerIndex = (headerPointer - headerBuffer);

	if(headerPointer == NULL){
		printf(ERROR "Soubor neobsahuje hlavicku MIDI souboru 'MThd'!\n");
		trackStatus = -1;
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
		trackStatus = -1;
		return 0;
	}

	if(trackPointer == NULL){
		printf(ERROR "Soubor neobsahuje stopu MIDI souboru 'MTrk'!\n");
		trackStatus = -1;
		return 0;
	}

	fseek(midifp, trackIndex+4, SEEK_SET);
	fgets(data, 5, midifp);

	playfile.trackSize = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]));	

	#ifdef MIDI_DEBUG
		printf("File format: %d  Track Count: %d  Time division: %d Track size: %ld\n",playfile.format, playfile.tracks, playfile.division, playfile.trackSize);
	#endif


	int err = pthread_create(&playerThread, NULL, &midiRTParser, (void *)midifp);
    if (err != 0) printf(ERROR "Nepodarilo se spustit vlakno prehravace! Chyba: %s\n", strerror(err));

	//midiRTParser(midifp);

	return 0;


}

void *midiRTParser(void * args){

	FILE *fp = args;

	trackStatus = 1;
	unsigned long deltaSum = 0;
	//Promenna pro kontrolu predchazejiciho prikazu
	int prevNote = 0;
	int noteChannel = 0;
	char c;
	unsigned char uc;

	while(1){

		unsigned long deltaTime = 0;
		

		//Vypocita se delta casu pro prikaz
		do{
			c = fgetc(fp);
			deltaTime |= (c & 0x7f) & 0xff;
			if(c & 0x80) deltaTime <<= 7;
		}while(c & 0x80);

		deltaSum += deltaTime;

		c = fgetc(fp);

		if((c & 0xf0) == 0x80 || (c & 0xf0) == 0x90){
			unsigned char note = fgetc(fp);
			unsigned char velocity = fgetc(fp);
			noteChannel = (c & 0x0f)+1;
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", deltaSum, noteChannel, note, velocity);
			#endif
			prevNote = 1;
		}else if((c & 0xf0) == 0xA0){
			fseek(fp, 2, SEEK_CUR);
			prevNote = 0;
		}else if((c & 0xf0) == 0xB0){
			unsigned char controller = fgetc(fp);
			unsigned char value = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Controller: %d  Value: %d\n", deltaSum, (c & 0x0f)+1, controller, value);
			#endif
			prevNote = 0;
		}else if((c & 0xf0) == 0xC0){
			fseek(fp, 1, SEEK_CUR);
			prevNote = 0;
		}else if((c & 0xf0) == 0xD0){
			fseek(fp, 1, SEEK_CUR);
			prevNote = 0;
		}else if((c & 0xf0) == 0xE0){
			fseek(fp, 2, SEEK_CUR);
			prevNote = 0;
		}else if((c & 0xff) == 0xF0){

			unsigned long sysexLenght = 0;

			do{
				c = fgetc(fp);
				sysexLenght |= (c & 0x7f) & 0xff;
				if(c & 0x80) sysexLenght <<= 7;
			}while(c & 0x80);

			unsigned char sysexData[255];
			memset(sysexData, 0, sizeof(sysexData));

			fread(sysexData, sysexLenght, 1, fp);

			#ifdef MIDI_DEBUG
				printf("DT: %lu Lenght: %lu Data: f0", deltaSum ,sysexLenght);

				for(int i = 0; i < sysexLenght; i++){
					printf(" %02x", sysexData[i]);
				}

				printf("\n");
			
			#endif

			prevNote = 0;

		}else if((c & 0xff) == 0xFF){

			int metaType = fgetc(fp);

			unsigned long metaLenght = 0;
			do{
				c = fgetc(fp);
				metaLenght |= (c & 0x7f) & 0xff;
				if(c & 0x80) metaLenght <<= 7;
			}while(c & 0x80);

			unsigned char metaData[255];
			memset(metaData, 0, sizeof(metaData));

			fread(metaData, metaLenght, 1, fp);
			#ifdef MIDI_DEBUG
				if(metaType > 0 && metaType < 7){
					printf("DT: %lu  Meta type: %d  Lenght: %lu Data: %s\n", deltaSum, metaType ,metaLenght, metaData);
				}else{
					printf("DT: %lu  Meta type: %d  Lenght: %lu\n", deltaSum, metaType ,metaLenght);
				}
			#endif

			// 47 je konec tracku, vyskoci ze smycky
			if(metaType == 47){
				trackStatus = 0;
				break;
			}

			prevNote = 0;
		}else if(prevNote){
			unsigned char note = c;
			unsigned char velocity = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", deltaSum, noteChannel, note, velocity);
			#endif
			prevNote = 1;
		}

	}

	fclose(fp);
	return NULL;
}