#include "serial.h"
#include "utils.h"
#include "midi.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <pthread.h>

int serialInit(char port[], char baud[]){
	sercom = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if(sercom < 1){
		printf(ERROR "Seriovy port %s nelze otevrit!\n", port);
		return 0;
	}else{
		printf(OK "Seriovy port %s uspesne otevren.\n", port);
	}

	if(!serialConfig(sercom, port, baud)) return 0;

	if(sem_init(&sercomLock, 0, 1) != 0) { 
        printf(ERROR "Nepodarilo se inicializovat mutex serioveho portu.");
        return 0; 
    } 

    if(sem_init(&midiBuffLock, 0, 1) != 0) { 
        printf(ERROR "Nepodarilo se inicializovat mutex MIDI bufferu.");
        return 0; 
    } 


	if(sem_init(&cmdBuffLock, 0, 1) != 0) { 
        printf(ERROR "Nepodarilo se inicializovat mutex DATA bufferu.");
        return 0; 
    } 

	int rc;
	pthread_attr_t attr;
	struct sched_param param;

	rc = pthread_attr_init(&attr);
	rc = pthread_attr_getschedparam(&attr, &param);
	param.sched_priority += 100;
	rc = pthread_attr_setschedparam(&attr, &param);

	int err = pthread_create(&serialReceiverThread, &attr, &serialReceiver, NULL);
    if(err != 0){
    	printf(ERROR "Nepodarilo se spustit vlakno prijimace serioveho portu! Chyba: %s\n", strerror(err));
    	return 0;
    }

	return 1;
}

int serialConfig(int sercom, char port[], char baud[]){
	struct termios serconfig;

	//Kontrola spravne predaneho pointeru
	if(!isatty(sercom)){
		printf(ERROR "Seriovy port %s neni platnym TTY.\n", port);
		return 0;
	}

	//Zkusi se nacist aktualni konfigurace serioveho portu
	if(tcgetattr(sercom, &serconfig) < 0){
		printf(ERROR "Nepodarilo se nacist aktualni konfiguraci portu %s.\n", port);
		return 0;
	}

	serconfig.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	serconfig.c_oflag = 0;
	serconfig.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	serconfig.c_cflag &= ~(CSIZE | PARENB);
	serconfig.c_cflag |= CS8;
	serconfig.c_cc[VMIN]  = 1;
	serconfig.c_cc[VTIME] = 0;

	int baudConverted;
	//Prevede se slovni vyjadreni na definici prenosove rychlosti
	switch(atoi(baud)){
		//Pokud je vysledek atoi = 0, mame chybu
		case 0: baudConverted = -1;
		break;
		case 50: baudConverted = B50;
		break;
		case 75: baudConverted = B75;
		break;
		case 110: baudConverted = B110;
		break;
		case 134: baudConverted = B134;
		break;
		case 150: baudConverted = B150;
		break;
		case 200: baudConverted = B200;
		break;
		case 300: baudConverted = B300;
		break;
		case 600: baudConverted = B600;
		break;
		case 1200: baudConverted = B1200;
		break;
		case 1800: baudConverted = B1800;
		break;
		case 2400: baudConverted = B2400;
		break;
		case 4800: baudConverted = B4800;
		break;
		case 9600: baudConverted = B9600;
		break;
		case 19200: baudConverted = B19200;
		break;
		case 38400: baudConverted = B38400;
		break;
		case 57600: baudConverted = B57600;
		break;
		case 115200: baudConverted = B115200;
		break;
		case 230400: baudConverted = B230400;
		break;
		case 460800: baudConverted = B460800;
		break;
		case 500000: baudConverted = B500000;
		break;
		case 576000: baudConverted = B576000;
		break;
		case 921600: baudConverted = B921600;
		break;
		case 1000000: baudConverted = B1000000;
		break;
		case 1152000: baudConverted = B1152000;
		break;
		case 1500000: baudConverted = B1500000;
		break;
		case 2000000: baudConverted = B2000000;
		break;
		case 2500000: baudConverted = B2500000;
		break;
		case 3000000: baudConverted = B3000000;
		break;
		case 3500000: baudConverted = B3500000;
		break;
		case 4000000: baudConverted = B4000000;
		break;
		//Prenosova rychlost neni v seznamu
		default: baudConverted = -2;
		break;
	}

	if(baudConverted == -1){
		baudConverted = B2000000;
		printf(ISSUE "Nebylo mozne ziskat prenosovou rychlost z konfiguracniho souboru! String \"\e[2m%s\e[22m\" nelze prevest na cislo. Rychlost nastavena na 2000000bps!\n", baud);
	}else if(baudConverted == -2){
		baudConverted = B2000000;
		printf(ISSUE "Zvolena prenosova rychlost %dbps neni sandardni rychlosti. Rychlost nastavena na 2000000bps!\n", atoi(baud));
	}else if(cfsetispeed(&serconfig, baudConverted) >= 0 || cfsetospeed(&serconfig, baudConverted) >= 0){
		printf(OK "Prenosova rychlost seriove linky uspesne nastavena na %dbps.\n", atoi(baud));
	}else{
		printf(ERROR "Nepodarilo se nastavit rychlost serioveho portu!\n");
	}

	if(tcsetattr(sercom, TCSAFLUSH, &serconfig) < 0){
		printf(ERROR "Nepodarilo se nastavit parametry serioveho portu!\n");
		return 0;
	}

	return 1;
}

int serialMIDIRead(void * buf, size_t count){

	if(midiBuffIndex >= count){
		sem_wait(&midiBuffLock);
		memcpy(buf, midiBuffer, count);
		memmove(midiBuffer, midiBuffer+count, sizeof(midiBuffer));
		midiBuffIndex -= count;
		sem_post(&midiBuffLock);
		return count;
	}

	
	//return midiBuffIndex;
	return -1;

}

int serialCMDRead(void * buf){
	//printf("READ: %d\n", count);
	if(cmdBuffIndex > 0){
		sem_wait(&cmdBuffLock);
		memcpy(buf, cmdBuffer, cmdBuffIndex);
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
		cmdBuffIndex = 0;
		sem_post(&cmdBuffLock);
		return 1;
	}
	
	return 0;
}

int serialCMDFlush(){
	sem_wait(&cmdBuffLock);
	memset(cmdBuffer, 0, sizeof(cmdBuffer));
	cmdBuffIndex = 0;
	sem_post(&cmdBuffLock);
	
	return ;
}

int serialCMDAvailable(){
	sem_wait(&cmdBuffLock);
	int len = cmdBuffIndex;
	sem_post(&cmdBuffLock);
	return len;
}

void *serialReceiver(){

	unsigned char recBytes[1000];
	int serReadBytes = 0, bytesAvailable = 0;

	//msgNullCounter = 0;


	sem_wait(&cmdBuffLock);
	memset(cmdBuffer, 0, sizeof(cmdBuffer));
	cmdBuffIndex = 0;
	//recvMsgLen = 0;
	sem_post(&cmdBuffLock);
	sem_wait(&midiBuffLock);
	memset(midiBuffer, 0, sizeof(midiBuffer));
	midiBuffIndex = 0;
	sem_post(&midiBuffLock);

	memset(recBytes, 0, sizeof(recBytes));

    sleep(1);
    sem_wait(&sercomLock);
  	tcflush(sercom,TCIOFLUSH);
  	sem_post(&sercomLock);

	while(1){

		sem_wait(&sercomLock);
		sem_wait(&midiBuffLock);

		ioctl(sercom, FIONREAD, &bytesAvailable);

		serReadBytes = read(sercom, recBytes, bytesAvailable);

		if(serReadBytes > 0){

			sem_wait(&cmdBuffLock);

			unsigned long ptr = memchr(recBytes, '\0', sizeof(recBytes));

	    	if(ptr > 0 && serReadBytes > 6){
	    		ptr -= (unsigned long)recBytes;
	        	if((recBytes[ptr] == 0 && recBytes[ptr+1] == 0 && recBytes[ptr+2] == 0 && recBytes[ptr+3] == 0) && (((recBytes[ptr+4] << 8) | recBytes[ptr+5]) > 0)){
	            	int len = (recBytes[ptr+4]<<8) | recBytes[ptr+5];
	            	memcpy(cmdBuffer, recBytes+ptr, len+6);
	            	memmove(recBytes+ptr, recBytes+ptr+len+6, sizeof(recBytes));
	            	if(trackStatus == 3){
	            		memcpy(midiBuffer+midiBuffIndex, recBytes, serReadBytes-(len+6));
	            		midiBuffIndex += serReadBytes-(len+6);
	            	}
	            	cmdBuffIndex += len+6;
	        	}else if(trackStatus == 3){
	        		memcpy(midiBuffer+midiBuffIndex, recBytes, serReadBytes);
	    			midiBuffIndex += serReadBytes;
	    			//printf("DATA NULL\n");
	        	}
	        
	    	}else if(trackStatus == 3){
	        	memcpy(midiBuffer+midiBuffIndex, recBytes, serReadBytes);
	    		midiBuffIndex += serReadBytes;
	    		//printf("DATA\n");
	        }

	    	sem_post(&cmdBuffLock);

	    	/*for(int i = 0; i < serReadBytes; i++){
	    		printf("%02x", recBytes[i]);
	    	}

	    	printf("\n");*/
	    }

	    serReadBytes = 0;

	    sem_post(&sercomLock);
	    sem_post(&midiBuffLock);
		/*//Pokud prijal byte
		if(serReadBytes == serReqBytes){

			sem_wait(&sercomLock);
			ioctl(sercom, FIONREAD, &bytesAvailable);
			sem_post(&sercomLock);

			//Pokud se prijal null
			if((msgNullCounter < 3 && recBytes[0] == 0) && bytesAvailable > 6){
				//Pricte se counter nullů
				msgNullCounter++;
				//Byte se zapise
				sem_wait(&cmdBuffLock);
				cmdBuffer[cmdBuffIndex++] = recBytes[0];
				sem_post(&cmdBuffLock);

				serReqBytes = 1;

			}else if(msgNullCounter == 3 && recBytes[0] == 0){
				//Nacte se dalsi byte (velikost zpravy)
				//cmdBuffer[cmdBuffIndex++] = recBytes[0];
				//Pricte se counter nullů
				msgNullCounter++;
				//Byte se zapise
				sem_wait(&cmdBuffLock);
				cmdBuffer[cmdBuffIndex++] = recBytes[0];
				sem_post(&cmdBuffLock);

				serReqBytes = 2;

			}else if(msgNullCounter == 4 && serReadBytes == 2){
				//Nacte se dalsi byte (velikost zpravy)
				sem_wait(&cmdBuffLock);
				cmdBuffer[cmdBuffIndex++] = recBytes[0];
				cmdBuffer[cmdBuffIndex++] = recBytes[1];
				msgLen = ((recBytes[0] << 8) | recBytes[1]);
				sem_post(&cmdBuffLock);
				serReqBytes = ((recBytes[0] << 8) | recBytes[1]);

			}else if(msgNullCounter == 4 && serReadBytes == msgLen){
				//Nacte se dalsi byte (velikost zpravy)
				sem_wait(&cmdBuffLock);
				memcpy(&cmdBuffer[cmdBuffIndex], recBytes, serReadBytes);

				recvMsgLen = msgLen;

				msgLen = 0;
				cmdBuffIndex = 0;

				sem_post(&cmdBuffLock);

				serReqBytes = 1;
				msgNullCounter = 0;
			}else if(trackStatus == 3){
				//Pokud jsem predtim prijimal ale nakonec to nebyla zprava
				if(msgNullCounter != 0){
					//Prekopiruji se data do midi bufferu
					sem_wait(&midiBuffLock);
					for(int i = 0; i < msgNullCounter; i++){
						midiBuffer[midiBuffIndex+i] = cmdBuffer[i];
					}

					//Zvysi se index bufferu
					midiBuffIndex += msgNullCounter;
					sem_post(&midiBuffLock);
					msgNullCounter = 0;
					sem_wait(&cmdBuffLock);
					cmdBuffIndex = 0;
					sem_post(&cmdBuffLock);
					serReqBytes = 1;
				}else{
					//Jinak se normalne zapise byte do bufferu
					sem_wait(&midiBuffLock);
					midiBuffer[midiBuffIndex++] = recBytes[0];
					printf("B: %x\n", recBytes[0]);
					sem_post(&midiBuffLock);
					sem_wait(&cmdBuffLock);
					cmdBuffIndex = 0;
					sem_post(&cmdBuffLock);
					serReqBytes = 1;
				}


				
				
			}
		}*/

		//usleep(1);
	}
}