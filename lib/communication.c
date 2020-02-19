#include "communication.h"
#include "serial.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>


pthread_t devAliveThread, msgDecoderThread;

int devComInit(){

	//Spusti se signalizace "keepalive" komunikace
	/*int err = pthread_create(&devAliveThread, NULL, &devAliveWorker, NULL);
    if(err != 0){
    	printf(ERROR "Nepodarilo se spustit vlakno komunikace se zarizenim! Chyba: %s\n", strerror(err));
    	return 0;
    }*/

    int err = pthread_create(&msgDecoderThread, NULL, &msgDecoder, NULL);
    if(err != 0){
    	printf(ERROR "Nepodarilo se spustit vlakno pro dekodovani zprav! Chyba: %s\n", strerror(err));
    	return 0;
    }

    return 1;

}

void *devAliveWorker(){

	char buff[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x27, 0x00, 0xAB};
	
	while(1){
		sem_wait(&sercomLock);
		write(sercom, buff, sizeof(buff));
		sem_post(&sercomLock);

		sleep(1);
	}
}

void *msgDecoder(){	
	while(1){
		
		if(serialCMDAvailable() > 0){
			
			char buff[200];
			int len = serialCMDAvailable();
			int i = serialCMDRead(buff);

			char * typS = (char*)malloc(30);
			char * zdrojS = (char*)malloc(30);
			char * cilS = (char*)malloc(30);

			int broadcast = (buff[6] & 0x04) >> 3;
			int type = buff[6] & 0xE0;
			int src = ((buff[6] & 0x18) >> 3);
			int dest = (buff[6] & 0x03);

			if(type == 0x20){
				if(broadcast){
					typS = "Internal Broadcast";
				}else typS = "Internal Unicast";
			}else{
				if(broadcast){
					typS = "External Broadcast";
				}else typS = "External Unicast";
			}

			if(src == 0x00){
				zdrojS = "PC";
			}else if(src == 0x01){
				zdrojS = "Ovladac";
			}else if(src == 0x02){
				zdrojS = "Hl. jednotka";
			}else{
				zdrojS = "Jine";
			}

			if(dest == 0x00){
				cilS = "PC";
			}else if(dest == 0x01){
				cilS = "Ovladac";
			}else if(dest == 0x02){
				cilS = "Hl. jednotka";
			}else{
				cilS = "Jine";
			}
			
			if(dest == ADDRESS || broadcast){
				printf("\n"CMD "Typ: %s Zdroj: %s Cil: %s Delka: %d Data: ", typS, zdrojS, cilS, len-6);

				for(int i = 6; i < len; i++){
					printf("%x", buff[i]);
				}

				printf("\n");

				decode(buff, len);
			}
			
		}

		usleep(1000);
	}
}

void decode(char * msg, int len){
	//Internal
	char msgType = msg[6];

	if((msgType & 0xE0) == 0x20){	
		if(msg[7] == INTERNAL_COM){
			if(msg[8] == INTERNAL_COM_PLAY){
				printf("\n"CMD "Prehraj %s.mid\n", (msg+9));
				if(midiPlay(msg+9)){
					msgAOK(0, msgType, len, 0, NULL);
				}else msgERR(0, msgType, len);
			}

			if(msg[8] == INTERNAL_COM_STOP){
				printf("\n"CMD "Stop\n");
				msgERR(0, msgType, len);
				if(midiStop()){
					msgAOK(0, msgType, len, 0, NULL);
				}else msgERR(0, msgType, len);
			}

			if(msg[8] == INTERNAL_COM_REC){
				printf("\n"CMD "Nahraj %s.mid\n", (msg+9));
				if(midiRec(msg+9)){
					msgAOK(0, msgType, len, 0, NULL);
				}else msgERR(0, msgType, len);
			}
		}
	}else{

	}
}

void msgAOK(int aokType, int recType, int recSize, int dataSize, char * msg){
	char * buffer = (char*)malloc(dataSize);
	//Utvori se AOK znak s typem
	buffer[0] = 0x80 | (aokType & 0x7f);
	buffer[1] = recType;
	buffer[2] = (recSize-6 & 0xff00) >> 8;
	buffer[3] = recSize-6 & 0xff;
	memcpy(&buffer[4], msg, dataSize);
	sendMsg(ADDRESS, ((recType & 0x18) >> 3), 0, 0x07, buffer, dataSize+4);
	free(buffer);
}

void msgERR(int errType, int recType, int recSize){
	char * buffer = (char*)malloc(4);
	//Utvori se ERR znak s typem
	buffer[0] = 0x7f & (errType & 0x7f);
	buffer[1] = recType;
	buffer[2] = (recSize-6 & 0xff00) >> 8;
	buffer[3] = recSize-6 & 0xff;

	sendMsg(ADDRESS, ((recType & 0x18) >> 3), 0, 0x07, buffer, 5);
	free(buffer);
}

void sendMsg(int src, int dest, int broadcast, int type, char * msg, int len){
	char * buffer = (char*)malloc(len+7);
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;
	buffer[4] = (len >> 4) & 0xff;
	buffer[5] = len & 0xff;
	buffer[6] = ((type & 0x07) << 5) | ((src & 0x3) << 3) | ((broadcast & 0x01) << 2) | (dest & 0x03);
	memcpy(&buffer[7], msg, len);

	sem_wait(&sercomLock);
	write(sercom, buffer, len+7);
	sem_post(&sercomLock);
	free(buffer);
}