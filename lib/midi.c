#include "midi.h"
#include "utils.h"
#include "serial.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

//Zapne debug pro dekodovani/kodovani MIDI souboru
//#define MIDI_DEBUG

//Pointer na midi soubor
FILE *midifp;
pthread_t playerThread;
//Struktura s udaji o souboru
struct midifile playfile;

int midiPlay(char songname[]){

	//Zakladni tempo je 120beats/minute
	playfile.tempo = 500000;

	trackStatus = 0;

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

	playfile.deltaType = (playfile.division & 0x8000) >> 15;

	if(playfile.deltaType){
		//Format v ticih na sekundu (framech za sekundu a ticich na frame)
		playfile.timeMultiplier = (playfile.division & 0x7f00)*(playfile.division & 0x00ff);
	}else{
		//Format je v ticich na ctvrtinovou notu
		playfile.timeMultiplier = playfile.division & 0x7fff;
	}

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
	unsigned char cmd;

	while(1){

		unsigned long deltaTicks = 0;

		//Vypocita se delta casu pro prikaz
		do{
			c = fgetc(fp);
			deltaTicks |= (c & 0x7f) & 0xff;
			if(c & 0x80) deltaTicks <<= 7;
		}while(c & 0x80);


		if(playfile.deltaType){
			usleep(((double)playfile.timeMultiplier*(double)deltaTicks)*1000000.0);
		}else{
			/*struct timespec tim;
			tim.tv_sec = 0;
			tim.tv_nsec = (((double)playfile.tempo / (double)playfile.timeMultiplier)*(double)deltaTicks*1000000000L);*/
			//printf("Delay %lf\n", ((double)deltaTicks*((double)playfile.timeMultiplier/((double)playfile.tempo/60.0)))*100000.0);
			//nanosleep(&tim, (struct timespec*)NULL);
			usleep((((double)playfile.tempo / (double)playfile.timeMultiplier)*(double)deltaTicks));
		}
		
		//deltaSum += deltaTicks;

		c = fgetc(fp);

		if((c & 0xf0) == 0x80 || (c & 0xf0) == 0x90){
			cmd = c;
			unsigned char note = fgetc(fp);
			unsigned char velocity = fgetc(fp);
			noteChannel = (c & 0x0f)+1;
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", deltaSum, noteChannel, note, velocity);
			#endif
			prevNote = 1;
			unsigned char serbuff[] = {cmd, note, velocity};
			write(sercom, serbuff, 3);
		}else if((c & 0xf0) == 0xA0){
			cmd = c;
			unsigned char note = fgetc(fp);
			unsigned char pressure = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Pressure: %d\n", deltaSum, (c & 0x0f)+1, note, pressure);
			#endif
			unsigned char serbuff[] = {cmd, note, pressure};
			write(sercom, serbuff, 3);
			prevNote = 0;
		}else if((c & 0xf0) == 0xB0){
			cmd = c;
			unsigned char controller = fgetc(fp);
			unsigned char value = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Controller: %d  Value: %d\n", deltaSum, (c & 0x0f)+1, controller, value);
			#endif
			unsigned char serbuff[] = {cmd, controller, value};
			write(sercom, serbuff, 3);
			prevNote = 0;
		}else if((c & 0xf0) == 0xC0){
			cmd = c;
			unsigned char program = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Program: %d\n", deltaSum, (c & 0x0f)+1, program);
			#endif
			unsigned char serbuff[] = {cmd, program};
			write(sercom, serbuff, 2);
			prevNote = 0;
		}else if((c & 0xf0) == 0xD0){
			cmd = c;
			unsigned char pressure = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: All Pressure: %d\n", deltaSum, (c & 0x0f)+1, pressure);
			#endif
			unsigned char serbuff[] = {cmd, pressure};
			write(sercom, serbuff, 2);
			prevNote = 0;
		}else if((c & 0xf0) == 0xE0){
			cmd = c;
			unsigned char lsb = fgetc(fp);
			unsigned char msb = fgetc(fp);
			noteChannel = (c & 0x0f)+1;
			unsigned char serbuff[] = {cmd, lsb, msb};
			write(sercom, serbuff, 3);
			prevNote = 0;
		}else if((c & 0xff) == 0xF0){
			cmd = c;
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
			unsigned char serbuff[] = {cmd, sysexLenght};
			write(sercom, serbuff, 3);
			write(sercom, sysexData, sysexLenght);


		}else if((c & 0xff) == 0xFF){
			cmd = c;
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

			if(metaType == 81){
				playfile.tempo = ((metaData[0] << 16) | (metaData[1] << 8) | metaData[2]);	
			}

			unsigned char serbuff[] = {cmd, metaType, metaLenght};
			write(sercom, serbuff, 3);
			write(sercom, metaData, metaLenght);

			prevNote = 0;
		}else if(prevNote){
			unsigned char note = c;
			unsigned char velocity = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", deltaSum, noteChannel, note, velocity);
			#endif
			prevNote = 1;
			unsigned char serbuff[] = {cmd, note, velocity};
			write(sercom, serbuff, 3);
		}



	}

	fclose(fp);
	return NULL;
}