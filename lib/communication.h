#ifndef COM_H
#define COM_H

//Typy zprav
#define ADDRESS_MAIN 				0x02
#define ADDRESS_PC 					0x00
#define ADDRESS_CONTROLLER 			0x01

#define ADDRESS_OTHER 				0x03

#define AOKERR 						0x07
#define ERR 						0x00
#define AOK 						0x80

#define INTERNAL 					0x01
#define EXTERNAL_BT 				0x02
#define EXTERNAL_DISP 				0x03
#define EXTERNAL_MIDI				0x04
#define EXTERNAL_USB 				0x05

#define INTERNAL_COM 				0x00
#define INTERNAL_COM_KEEPALIVE 		0xAB
#define INTERNAL_COM_STOP 			0x00
#define INTERNAL_COM_PLAY 			0x01
#define INTERNAL_COM_REC 			0x02
#define INTERNAL_COM_CHECK_NAME 	0x03
#define INTERNAL_COM_GET_SONGS 		0x04
#define INTERNAL_COM_GET_TIME 		0x05
#define INTERNAL_COM_SET_TIME 		0x06

#define INTERNAL_DISP 				0x01
#define INTERNAL_DISP_GET_STATUS 	0x00
#define INTERNAL_DISP_SET_SONG	 	0x01
#define INTERNAL_DISP_SET_VERSE		0x02
#define INTERNAL_DISP_SET_LETTER 	0x03
#define INTERNAL_DISP_SET_LED	 	0x04

#define INTERNAL_USB 				0x02
#define INTERNAL_USB_GET_CONNECTED	0x00

#define INTERNAL_CURR 				0x03
#define INTERNAL_CURR_GET_STATUS	0x00
#define INTERNAL_CURR_SET_STATUS 	0x01

#define INTERNAL_BT		 			0x04
#define INTERNAL_BT_GET_STATUS		0x00

#define INTERNAL_MIDI 				0x05
#define INTERNAL_MIDI_GET_STATUS	0x00



int aliveRemote, aliveRemoteCounter, aliveMain, aliveMainCounter;

int devComInit();	
void *devAliveWorker();
void *msgDecoder();

void msgAOK(int aokType, int recType, int recSize, int dataSize, char * msg);
void msgERR(int errType, int recType, int recSize);
void sendMsg(int src, int dest, int broadcast, int type, char * msg, int len);


#endif