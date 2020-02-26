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
#define MIDI_DEBUG

//Pointer na midi soubor
FILE *midifp;
pthread_t playerThread;
pthread_t recorderThread;
//Struktura s udaji o souboru
struct midifile playfile, recfile;

int midiPlay(char songname[]){

	if(!aliveMain){
		printf(ERROR "S hlavni jednotkou neni navazano spojeni!\n");
		return 0;
	}

	//Kontrola zda uz se neprehrava/nenahrava
	if(trackStatus == 1 || trackStatus == 3) return 0;

	//Zakladni tempo je 120beats/minute
	playfile.tempo = 500000;

	trackStatus = 0;

	char dir[255];

	//Soubor se otevre
	strcpy(dir, parameters[2]);
	strcat(dir, "/");
	strcat(dir, songname);
	strcat(dir, ".mid");
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
		printf("File format: %d  Track Count: %d  Time division: %d Time multiplier: %ld Track size: %lu\n",playfile.format, playfile.tracks, playfile.division, playfile.timeMultiplier, playfile.trackSize);
	#endif

	char msg[50];
	msg[0] = 0x00;
	msg[1] = 0x01;
	memcpy(&msg[2], songname, strlen(songname));
	sendMsg(ADDRESS_PC, ADDRESS_MAIN, 1, INTERNAL, msg, strlen(songname)+3);

	int err = pthread_create(&playerThread, NULL, &midiPlayParser, (void *)midifp);
    if (err != 0){
    	printf(ERROR "Nepodarilo se spustit vlakno prehravace! Chyba: %s\n", strerror(err));
    	return 0;
    } 

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
		//Killne se prehravani
		pthread_cancel(playerThread);
		fclose(midifp);

		//Vypnou se vsechny noty
		for(int i = 0; i <= 0xf; i++){
				for(int j = 0; j <= 127; j++){
					char serbuff[] = {(0x80 | i), j, 64};
					sem_wait(&sercomLock);
					write(sercom, serbuff, sizeof(serbuff));
					sem_post(&sercomLock);
				}
		}
	}

	trackStatus = 0;

	return 1;


}

void *midiPlayParser(void * args){

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
		
		deltaSum += deltaTicks;

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
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
		}else if((c & 0xf0) == 0xA0){
			cmd = c;
			unsigned char note = fgetc(fp);
			unsigned char pressure = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Pressure: %d\n", deltaSum, (c & 0x0f)+1, note, pressure);
			#endif
			unsigned char serbuff[] = {cmd, note, pressure};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
			prevNote = 0;
		}else if((c & 0xf0) == 0xB0){
			cmd = c;
			unsigned char controller = fgetc(fp);
			unsigned char value = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Controller: %d  Value: %d\n", deltaSum, (c & 0x0f)+1, controller, value);
			#endif
			unsigned char serbuff[] = {cmd, controller, value};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
			prevNote = 0;
		}else if((c & 0xf0) == 0xC0){
			cmd = c;
			unsigned char program = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Program: %d\n", deltaSum, (c & 0x0f)+1, program);
			#endif
			unsigned char serbuff[] = {cmd, program};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
			prevNote = 0;
		}else if((c & 0xf0) == 0xD0){
			cmd = c;
			unsigned char pressure = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: All Pressure: %d\n", deltaSum, (c & 0x0f)+1, pressure);
			#endif
			unsigned char serbuff[] = {cmd, pressure};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
			prevNote = 0;
		}else if((c & 0xf0) == 0xE0){
			cmd = c;
			unsigned char lsb = fgetc(fp);
			unsigned char msb = fgetc(fp);
			noteChannel = (c & 0x0f)+1;
			unsigned char serbuff[] = {cmd, lsb, msb};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
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
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			write(sercom, sysexData, sysexLenght);
			sem_post(&sercomLock);
			


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
				printf("DT: %lu  Meta type: %d  Tempo: %ld\n", deltaSum, metaType ,playfile.tempo );
			}

			unsigned char serbuff[] = {cmd, metaType, metaLenght};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			write(sercom, metaData, metaLenght);
			sem_post(&sercomLock);
			

			prevNote = 0;
		}else if(prevNote){
			unsigned char note = c;
			unsigned char velocity = fgetc(fp);
			#ifdef MIDI_DEBUG
				printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", deltaSum, noteChannel, note, velocity);
			#endif
			prevNote = 1;
			unsigned char serbuff[] = {cmd, note, velocity};
			sem_wait(&sercomLock);
			write(sercom, serbuff, sizeof(serbuff));
			sem_post(&sercomLock);
		}



	}

	fclose(fp);
	trackStatus = 0;
	return NULL;
}


int midiRec(char songname[]){

	if(!aliveMain){
		printf(ERROR "S hlavni jednotkou neni navazano spojeni!\n");
		return 0;
	}


	//Kontrola zda uz se neprehrava/nenahrava
	if(trackStatus == 1 || trackStatus == 3) return 0;

	//Zakladni tempo je 120beats/minute
	recfile.tempo = 50000; //us/1/4
	recfile.division = 10000; //tick/1/4

	recfile.timeMultiplier = 10000;

	trackStatus = 0;

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

	//Soubor se vytvori
	midifp = fopen(dir, "wb");

	if(midifp == NULL){
		printf(ERROR "Zadany soubor %s se nepodarilo vytvorit!\n", dir);
		trackStatus = -1;
		return 0;
	}

	printf("%x %x", (recfile.division & 0xff00)>>8, (recfile.division & 0x00ff));

	//Vytvori se hlavicka MIDI souboru
	unsigned char headerBuffer[] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, (recfile.division & 0xff00) >> 8, (recfile.division & 0x00ff), 'M', 'T', 'r', 'k', 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x51, 0x03, (recfile.tempo & 0xff0000)>>16, (recfile.tempo & 0x00ff00)>>8,recfile.tempo & 0x0000ff};


	for(int i = 0; i < sizeof(headerBuffer); i++){
		//Header se vlozi do souboru
		fprintf(midifp, "%c", headerBuffer[i]);
	}


	int err = pthread_create(&recorderThread,/* &attr*/NULL, &midiRecordParser, (void *)midifp);
    if (err != 0){
    	printf(ERROR "Nepodarilo se spustit vlakno nahravace! Chyba: %s\n", strerror(err));
    	return 0;
    }

	//midiPlayParser(midifp);



	return 1;


}


void *midiRecordParser(void * args){

	FILE *fp = args;

	struct timespec time1, time2;
	char byte[10];
	int second = 1;
	long delta;
	unsigned long long int deltaTicks;


	trackStatus = 3;
	//Promenna pro kontrolu predchazejiciho prikazu
	int prevNote = 0;
	int noteChannel = 0;

	//Flag pro variable lenght quantity
	//-1 - neinicializovano
	//0 - neni VLQ
	//1 - VLQ probiha
	//2 - VLQ hotovo
	int vlqStat = -1;
	
	unsigned char metaType, sysexType;
	unsigned long metaLen;


	unsigned char uc;

	unsigned char c[500];
	unsigned char cmd = 0;
	unsigned int reqBytes = 1, readBytes = 0;
	long long int lenght = 0;

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time1);

	while(trackStatus == 3){

		memset(c, 0, sizeof(c));
		readBytes = serialMIDIRead(c, reqBytes);
		//printf("Time: %f\n", time_spent);
		//readBytes = read(sercom, c, reqBytes);

		

		if(readBytes == reqBytes){
			//printf("XXX RB: %d REB: %d\n", readBytes, reqBytes);

			if(cmd == 0){
				cmd = c[0];
				if(cmd != 0xfe){
					clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time2);

					double time = (double)timeDiff(time1, time2);
					delta = round((double)timeDiff(time1, time2)/((double)recfile.tempo/(double)recfile.timeMultiplier));
					clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time1);
				}
			}

			if((cmd & 0xf0) == 0x80 || (cmd & 0xf0) == 0x90){

				reqBytes = 2;

				if(readBytes == reqBytes){
					noteChannel = (cmd & 0x0f)+1;
					unsigned char note = c[0];
					unsigned char velocity = c[1];
					
					#ifdef MIDI_DEBUG
						printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", delta, noteChannel, note, velocity);
					#endif
					
					int len = 0;
					deltaTicks = toVLQ(delta, &len);

					fwrite(&deltaTicks, len, 1, fp);
					recfile.trackSize += (len + 3);

					//Vytvori se hlavicka MIDI souboru
					unsigned char fileBuff[] = {cmd, note, velocity};

					for(int i = 0; i < sizeof(fileBuff); i++){
						//Header se vlozi do souboru
						fprintf(fp, "%c", fileBuff[i]);
					}

					prevNote = 1;
					cmd = 0;
					reqBytes = 1;

				}

			}else if((cmd & 0xf0) == 0xA0){

				reqBytes = 2;

				if(readBytes == reqBytes){
					noteChannel = (cmd & 0x0f)+1;
					unsigned char note = c[0];
					unsigned char pressure = c[1];
					#ifdef MIDI_DEBUG
						printf("DT: %lu Channel: %d Note: %d  Pressure: %d\n", delta, noteChannel, note, pressure);
					#endif

					int len = 0;
					deltaTicks = toVLQ(delta, &len);

					fwrite(&deltaTicks, len, 1, fp);
					recfile.trackSize += (len + 3);

					unsigned char fileBuff[] = {cmd, note, pressure};

					for(int i = 0; i < sizeof(fileBuff); i++){
						fprintf(fp, "%c", fileBuff[i]);
					}


					cmd = 0;
					reqBytes = 1;
					prevNote = 0;
				}
				
			}else if((cmd & 0xf0) == 0xB0){

				reqBytes = 2;

				if(readBytes == reqBytes){
					noteChannel = (cmd & 0x0f)+1;
					unsigned char controller = c[0];
					unsigned char value = c[1];
					#ifdef MIDI_DEBUG
						printf("DT: %lu Channel: %d Controller: %d  Value: %d\n", delta, noteChannel, controller, value);
					#endif

					int len = 0;
					deltaTicks = toVLQ(delta, &len);

					fwrite(&deltaTicks, len, 1, fp);
					recfile.trackSize += (len + 3);

					unsigned char fileBuff[] = {cmd, controller, value};

					for(int i = 0; i < sizeof(fileBuff); i++){
						fprintf(fp, "%c", fileBuff[i]);
					}

					cmd = 0;
					reqBytes = 1;
					prevNote = 0;
				}
				
			}else if((cmd & 0xf0) == 0xC0){

				
				reqBytes = 1;

				if(readBytes == reqBytes && c[0] != cmd){
					noteChannel = (cmd & 0x0f)+1;
					unsigned char program = c[0];
					#ifdef MIDI_DEBUG
						printf("DT: %lu Channel: %d Program: %d\n", delta, noteChannel, program);
					#endif

					int len = 0;
					deltaTicks = toVLQ(delta, &len);

					fwrite(&deltaTicks, len, 1, fp);
					recfile.trackSize += (len + 2);

					unsigned char fileBuff[] = {cmd, program};

					for(int i = 0; i < sizeof(fileBuff); i++){
						fprintf(fp, "%c", fileBuff[i]);
					}

					cmd = 0;
					reqBytes = 1;
					prevNote = 0;
				}
				
			}else if((cmd & 0xf0) == 0xD0){

				reqBytes = 1;

				if(readBytes == reqBytes){
					noteChannel = (cmd & 0x0f)+1;
					unsigned char pressure = c[0];
					#ifdef MIDI_DEBUG
						printf("DT: %lu Channel: %d Note: All Pressure: %d\n", delta, noteChannel, pressure);
					#endif

					int len = 0;
					deltaTicks = toVLQ(delta, &len);

					fwrite(&deltaTicks, len, 1, fp);
					recfile.trackSize += (len + 2);

					unsigned char fileBuff[] = {cmd, pressure};

					for(int i = 0; i < sizeof(fileBuff); i++){
						fprintf(fp, "%c", fileBuff[i]);
					}

					cmd = 0;
					reqBytes = 1;
					prevNote = 0;
				}
			}else if((cmd & 0xf0) == 0xE0){

				reqBytes = 2;

				if(readBytes == reqBytes){
					noteChannel = (cmd & 0x0f)+1;
					unsigned char lsb = c[0];
					unsigned char msb = c[1];
					#ifdef MIDI_DEBUG
						printf("DT: %lu Channel: %d Note: All Pitch: %d\n", delta, noteChannel, (msb << 8) | (lsb<<1));
					#endif

					int len = 0;
					deltaTicks = toVLQ(delta, &len);

					fwrite(&deltaTicks, len, 1, fp);
					recfile.trackSize += (len + 3);

					unsigned char fileBuff[] = {cmd, lsb, msb};

					for(int i = 0; i < sizeof(fileBuff); i++){
						fprintf(fp, "%c", fileBuff[i]);
					}


					cmd = 0;
					reqBytes = 1;
					prevNote = 0;
				}
			}else if((cmd & 0xff) == 0xF0){

				unsigned long sysexLenght = 0;

				/*do{
					//c = fgetc(fp);
					sysexLenght |= (c & 0x7f) & 0xff;
					if(c & 0x80) sysexLenght <<= 7;
				}while(c & 0x80);*/

				//unsigned char sysexData[255];
				//memset(sysexData, 0, sizeof(sysexData));

				//fread(sysexData, sysexLenght, 1, fp);

				#ifdef MIDI_DEBUG
					/*printf("DT: %lu Lenght: %lu Data: f0", deltaSum ,sysexLenght);

					for(int i = 0; i < sysexLenght; i++){
						printf(" %02x", sysexData[i]);
					}

					printf("\n");*/
				
				#endif

				cmd = 0;
				reqBytes = 1;
				prevNote = 0;
				//unsigned char serbuff[] = {cmd, sysexLenght};
				//write(sercom, serbuff, 3);
				//write(sercom, sysexData, sysexLenght);


			}else if((cmd & 0xff) == 0xFF){

				if(vlqStat == -1){
					reqBytes = 1;
					vlqStat = 0;
					metaType = 0;
					metaLen = 0;
				}else if(readBytes == reqBytes && vlqStat == 0){
					
					metaType = c[0];
					reqBytes = 1;
					vlqStat = 1;
		
				}else if(vlqStat == 1){
					if(c[0] & 0x80){
						metaLen |= (c[0] & 0x7f) & 0xff;
						metaLen <<= 7;
					}else{
						metaLen |= (c[0] & 0x7f) & 0xff;
						vlqStat = 2;
						reqBytes = metaLen;
					}

				}else if(vlqStat == 2 && readBytes == reqBytes){

					#ifdef MIDI_DEBUG
						if(metaType > 0 && metaType < 7){
							printf("DT: %lu  Meta type: %d  Lenght: %lu Data: %s\n", delta, metaType ,metaLen, c);
						}else{
							printf("DT: %lu  Meta type: %d  Lenght: %lu\n", delta, metaType ,metaLen);
						}
					#endif

					cmd = 0;
					reqBytes = 1;
					prevNote = 0;
					vlqStat = -1;
				}
			}else if((cmd & 0xff) == 0xfe){
				reqBytes = 1;
				cmd = 0;
				prevNote = 0;
				#ifdef MIDI_DEBUG
					printf("Active sensing\n");
				#endif

			}else if(prevNote){
				//unsigned char note = c;
				//unsigned char velocity = fgetc(fp);
				#ifdef MIDI_DEBUG
					//printf("DT: %lu Channel: %d Note: %d  Velocity: %d\n", deltaSum, noteChannel, note, velocity);
				#endif
				//prevNote = 1;
				//unsigned char serbuff[] = {cmd, note, velocity};
				//write(sercom, serbuff, 3);
			}

		
		}

	}


	fseek(fp,18 ,SEEK_SET);

	//Pricte se prvni meta tag
	recfile.trackSize += 7;
	//Do hlavicky se doda delka
	unsigned char headerBuffer[] = {(recfile.trackSize & 0xff000000)>>24, (recfile.trackSize & 0xff0000)>>16, (recfile.trackSize & 0xff00)>>8, (recfile.trackSize & 0xff)};

	for(int i = 0; i < sizeof(headerBuffer); i++){
		//Header se vlozi do souboru
		fprintf(fp, "%c", headerBuffer[i]);
	}

	fclose(fp);
	trackStatus = 0;
	return NULL;
}

unsigned long long int toVLQ(unsigned long long int dt, int *bytes){
	unsigned long long int out = 0;

	out = dt & 0x7f;
	while((dt >>= 7) > 0){
 		out <<= 8;
 		out |= 0x80;
 		out += (dt & 0x7f);
 		(*bytes)++;
 	}

 	(*bytes)++;
 	return out;

}

