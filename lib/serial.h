#ifndef SERIAL_H
#define SERIAL_H

int sercom;
int serialInit(char port[], char baud[]);
int serialConfig(int sercom, char port[], char baud[]);

#endif