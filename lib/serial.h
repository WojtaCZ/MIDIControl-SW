#ifndef SERIAL_H
#define SERIAL_H

#include <semaphore.h>

int sercom;
sem_t sercomLock, cmdBuffLock;

pthread_t serialReceiverThread;

int cmdBuffIndex, msgNullCounter, recvMsgLen;
unsigned char cmdBuffer[10000];

int serialInit(char port[], char baud[]);
int serialConfig(int sercom, char port[], char baud[]);
void *serialReceiver();
int serialCMDRead(void * buf);
int serialCMDAvailable();
int serialCMDFlush();

#endif