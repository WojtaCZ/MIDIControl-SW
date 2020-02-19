#ifndef COM_H
#define COM_H

#define ADDRESS 0x00

#define INTERNAL_COM 0x00
#define INTERNAL_COM_KEEPALIVE 0xAB
#define INTERNAL_COM_STOP 0x00
#define INTERNAL_COM_PLAY 0x01
#define INTERNAL_COM_REC 0x02

#define INTERNAL_DISP 0x01
#define INTERNAL_USB 0x02
#define INTERNAL_CURR 0x03
#define INTERNAL_BLUETOOTH 0x04
#define INTERNAL_MIDI 0x05
#define INTERNAL_CHRG 0x05

int devComInit();	
void *devAliveWorker();
void *msgDecoder();

void msgAOK(int aokType, int recType, int recSize, int dataSize, char * msg);
void msgERR(int errType, int recType, int recSize);
void sendMsg(int src, int dest, int broadcast, int type, char * msg, int len);


#endif