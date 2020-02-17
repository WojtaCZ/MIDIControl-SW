#include "communication.h"
#include "serial.h"
#include "utils.h"
#include <stdio.h>


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

	char buff[] = {0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0xAB};
	
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
			printf("CMD: ");
			char buff[200];
			int len = serialCMDAvailable()+6;
			int i = serialCMDRead(buff);
			
			for(int i = 0; i < len; i++){
				printf("%x", buff[i]);
			}

			printf("\n");
			
		}
	}
}