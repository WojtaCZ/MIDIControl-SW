#ifndef SERIAL_H
#define SERIAL_H

#include <semaphore.h>

int sercom;
sem_t sercomLock, midiBuffLock, cmdBuffLock;

pthread_t serialReceiverThread;

int cmdBuffIndex, midiBuffIndex, msgNullCounter, recvMsgLen;
unsigned char cmdBuffer[100];
unsigned char midiBuffer[500];

int serialInit(char port[], char baud[]);
int serialConfig(int sercom, char port[], char baud[]);
void *serialReceiver();
int serialMIDIRead(void * buf, size_t count);
int serialCMDRead(void * buf);
int serialCMDAvailable();

#endif