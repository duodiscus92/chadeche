/*

Copyright 2017 Jacques Ehrlich

This file is part of Chadeche.

    Chadeche is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Chadeche is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Chadeche.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
        Use wiringPi Interface Library released under the GNU LGPLv3 license
                http://wiringpi.com
*/

#include "chadeche.h"

CONFIG tconfig[MAX_STEP];

/* read the config file and store values into tconfig */
void readconf(char *filename)
{
	FILE *fp;
	int i = 0;
	char buffer[80];

	if ((fp = fopen(filename, "r")) == NULL){
	    printf(language == EN ? "Program aborted : can't open config file %s\n" : \
				    "Programme abandonné : impossible ouvir fichier de configuration %s\n", filename);
	    exit(1);
	}

	/* first line always ignored */
	fgets(buffer, 80, fp);
	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    //sscanf(buffer, "%d;%c;%d;%d;%[A-Z0-9\\-];%[A-Z0-9\\-];%[A-Z0-9\\-];%[A-Z0-9\\-]\n",
	    sscanf(buffer, "%d\t%c\t%d\t%d\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\n",
		&(tconfig[i].adr),
		&(tconfig[i].cop),
		&(tconfig[i].milliamp),
		&(tconfig[i].duration),
		tconfig[i].toolow,
		tconfig[i].toohigh,
		tconfig[i].toolowcapacity,
		tconfig[i].toohighcapacity,
		tconfig[i].always,
		tconfig[i].controlflag);
	    strcpy(tconfig[i].comment, strchr(buffer, '#')+1);
	    if (i++ > MAX_STEP){
		printf(language == EN ? "Program aborted : too much step in a configuation file. Must not exceed %d\n" : \
					"Programme abandonné : trop de séquence dans un fichier de configuration. Ne peut  pas dépasser %d\n", MAX_STEP);
		exit(1);
	    }
	}
	tconfig[i-1].cop = 0; // pour marquer le fin de config
}

#define IGNORE /* pour ne pas compiler une portion de code qui ne marche pas */
/* affichage de la configuration */
void displayconf(void)
{
    int i, jumpadress;
    char *str, *endptr;

    printf(language == EN ? "Sequences will be as follow ...\n" : "Les séquences se dérouleront comme suit ...\n");
    printf("STEP\t|\tADR\tCOP\tmA\tS\tL\tH\tLmAh\tHmAh\tTRUE\tCtrlZ\tComment\n");
    for(i = 0/*, mAh = cinit*/; tconfig[i].cop != 0; i++){
	printf("%d\t|\t%d\t%c\t%d\t%d",
	    i+1,
	    tconfig[i].adr,					// numero ligne
	    tconfig[i].cop, 					// code opération
	    tconfig[i].milliamp,				//courant de charge ou décharge
	    tconfig[i].duration);				// durée de l'étape
#ifndef IGNORE
	    if( 
		( strchr((str = tconfig[i].toolow), ':') == NULL) 		||
		( strchr((str = tconfig[i].toohigh), ':') == NULL)		||
		( strchr((str = tconfig[i].toolowcapacity), ':') == NULL)	||
		( strchr((str = tconfig[i].toohighcapacity), ':') == NULL)	||
		( strchr((str = tconfig[i].always), ':') == NULL)		||
		( strchr((str = tconfig[i].controlflag), ':') == NULL)
	    ){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
	    	printf("\t%s", str);
#else
	    if( strchr((str = tconfig[i].toolow), ':') == NULL){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
	    	printf("\t%s", str);
	    if( strchr((str = tconfig[i].toohigh), ':') == NULL){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
		printf("\t%s", str);
	    if( strchr((str = tconfig[i].toolowcapacity), ':') == NULL){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
		printf("\t%s", str);
	    if( strchr((str = tconfig[i].toohighcapacity), ':') == NULL){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
		printf("\t%s", str);
	    if( strchr((str = tconfig[i].always), ':') == NULL){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
		printf("\t%s", str);
	    if( strchr((str = tconfig[i].controlflag), ':') == NULL){ 
	    	jumpadress = strtol(str,  &endptr, 10);
	    	endptr == str ? printf("\t%c", str[0]) : printf("\t%d", jumpadress);
	    }
	    else
		printf("\t%s", str);
#endif
	    printf("\t%s",  tconfig[i].comment);
    }
}
