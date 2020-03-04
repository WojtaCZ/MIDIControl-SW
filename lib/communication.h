#ifndef COM_H
#define COM_H

//Typy zprav
#define ADDRESS_MAIN 0x02
#define ADDRESS_PC 0x00
#define ADDRESS_CONTROLLER 0x01
#define ADDRESS_OTHER 0x03

#define INTERNAL 0x01

#define INTERNAL_COM 0x00
#define INTERNAL_COM_KEEPALIVE 0xAB
#define INTERNAL_COM_STOP 0x00
#define INTERNAL_COM_PLAY 0x01
#define INTERNAL_COM_REC 0x02

#define INTERNAL_DISP 0x01
#define INTERNAL_DISP_SET 0x00
#define INTERNAL_DISP_GET 0x01

#define INTERNAL_USB 0x02


#define INTERNAL_CURR 0x03
#define INTERNAL_CURR_ON 0x01
#define INTERNAL_CURR_OFF 0x00

#define INTERNAL_BLUETOOTH 0x04


#define INTERNAL_MIDI 0x05


#define INTERNAL_CHRG 0x05

#define EXTERNAL_DISP 0x03


int aliveRemote, aliveRemoteCounter, aliveMain, aliveMainCounter;

int devComInit();	
void *devAliveWorker();
void *msgDecoder();

void msgAOK(int aokType, int recType, int recSize, int dataSize, char * msg);
void msgERR(int errType, int recType, int recSize);
void sendMsg(int src, int dest, int broadcast, int type, char * msg, int len);


#endif