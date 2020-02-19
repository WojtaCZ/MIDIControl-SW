#ifndef UTILS_H
#define UTILS_H

#include <time.h>

//Barva pro stavova hlaseni
#define ERROR "\e[41m ERROR \e[0m  "
#define ISSUE "\e[43m ISSUE \e[0m  "
#define OK "\e[42m OK \e[0m     "
#define CMD "\e[44m CMD \e[0m    "

//Pole s konfiguracnimy parametry
char parameters[255][255];
//Indexy
// 0 = SERCOM - Seriovy port komunikace
// 1 = SERBAUD - Rychlost komunikace
//Ziskani konfigurace ze souboru
int getConfig();
int getDirContents(char * directory, char *(*filearray)[500], int * count);
long timeDiff(struct timespec start, struct timespec end);

#endif