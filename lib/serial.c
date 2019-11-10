#include "serial.h"
#include "utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

int serialInit(char port[], char baud[]){
	sercom = open(port, O_RDWR | O_NOCTTY);
	if(sercom < 1){
		printf(ERROR "Seriovy port %s nelze otevrit!\n", port);
		return 0;
	}else{
		printf(OK "Seriovy port %s uspesne otevren.\n", port);
	}

	if(!serialConfig(sercom, port, baud)) return 0;

	//close(sercom);
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
	}else{
		printf(OK "Prenosova rychlost seriove linky uspesne nastavena na %dbps.\n", atoi(baud));
	}

	return 1;
}