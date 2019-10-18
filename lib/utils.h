#ifndef UTILS_H
#define UTILS_H

//Barva pro stavova hlaseni
#define ERROR "\e[41m ERROR \e[0m  "
#define ISSUE "\e[43m ISSUE \e[0m  "
#define OK "\e[42m OK \e[0m     "

//Pole s konfiguracnimy parametry
char parameters[255][255];
//Indexy
// 0 = SERCOM - Seriovy port komunikace
// 1 = SERBAUD - Rychlost komunikace
//Ziskani konfigurace ze souboru
int getConfig();

#endif