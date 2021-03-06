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


/* renvoie un pointeur sur le premier caractère rencontré différent de c */
static char *strnskip(char *s, char c)
{
    char *p = s;
    while(*p)
	if (*p != c)
	   break;
	else
	    p++;
    return p;
}

/* param manager */
int prmManager(void)
{
   FILE *fp;
   int myargc=1, opt;
   char buffer[80], *myargv[50], fname[20], *p;

   strcpy(fname, PRM_FILENAME);
   p = strchr(fname, 'X');
   *p = dba+0x30;

   if ((fp = fopen(fname, "r")) == NULL){
	printf(language == EN ? "Unable to open parameter file %s\n" : "Impossible ouvrir fichier de paramètres %s\n", fname);
	return -1;
    }
    // par forcément utile mais je sais pas si getopt en a besoin
    myargv[0] = malloc(strlen(fname)+1);
    strcpy(myargv[0], fname);

    // on construit la liste des arguments pour getopt
    fgets(buffer, 80, fp);
    while (!feof(fp)) {
	//printf(buffer);
	myargv[myargc] = malloc(strlen(buffer)+1);
	strncpy(myargv[myargc++], buffer, strlen(buffer)-1);
      	fgets(buffer, 80, fp);
    }
    //printf("Nb parm = %d\n", myargc);
    optind = 1; //indispensable
    while ((opt = getopt(myargc, myargv, "c:C:i:n:f:g:o:p:vm:M:s:l:1:2:")) != -1) {	
    switch(opt){
	/* is it battery capacity min ?*/
	case 'c':
	    cmin = atoi(optarg);
	    break;
	/* is it battery capacity max ?*/
	case 'C':
	    cmax = atoi(optarg);
	    break;
	/* is it battery initial capacity ?*/
	case 'i':
	    cinit = atoi(optarg);
	    break;
	/* is it number of cycle ?*/
	case 'n':
	    ncycle = atoi(optarg);
	    break;
	/* is it filename ?*/
	case 'f':
	    strcpy(results_filename, strnskip(optarg, ' '));
	    break;
	/* is it config filename ?*/
	case 'g':
	    strcpy(config_filename, strnskip(optarg, ' '));
	    break;
	/* is it offset voltage ? */
	case 'o':
	    offset = atof(optarg);
	    break;
	/* is it slope voltage ? */
	case 's':
	    slope = atof(optarg);
	    break;
	/* is it offset current? */
	case '2':
	    ioffset = atof(optarg);
	    break;
	/* is it slope current ? */
	case '1':
	    islope = atof(optarg);
	    break;
	/* is it recordPeriod */
	case 'p':
	    recordPeriod = atoi(optarg);
	    break;
	/* is it language seclection */
	case 'l':
	    language = (optarg[0] == 'E' ? EN : FR);
	    break;
	/* is it verbose_concise */
	case 'v':
	    verbose_concise = VERBOSE;
	    break;
	/* is it offset vmin */
	case 'm':
	    vmin = atof(optarg);
	    break;
	/* is it offset vmaw */
	case 'M':
	    vmax= atof(optarg);
	    break;
	default :
	    printf(language == EN ? "prmManager : Unknown option or bad syntax\n" : "prmManager : option inconnue ou erreur de syntaxe\n");
	    return -1;
    }
    }
    return 0;
}


/* arg manager */
void argManager(int argc, char **argv)
{
    int opt;
    /* Testing and getting parameters */
    /* Paramters are
    -a : daughter board adresse
    -i : initial capacity
    -c : battery capacity min (mAh)
    -C : battery capacity max (mAh)
    -f : filename to record results
    -n : # cycles
    -g : filename configuration
    -r : threshold for recording
    -h : help */
    //while(i>=1){
    optind = 1;
    while ((opt = getopt(argc, argv, "a:c:C:i:n:f:g:p:vm:M:l:ht")) != -1) {	
    switch(opt){
	/* is it daughter board adress ?*/
        case 'a':
	    dba = atoi(optarg);
	    break;
	/* is it battery capacity min ?*/
	case 'c':
	    cmin = atoi(optarg);
	    break;
	/* is it battery capacity max ?*/
	case 'C':
	    cmax = atoi(optarg);
	    break;
	/* is it battery initial capacity ?*/
	case 'i':
	    cinit = atoi(optarg);
	    break;
	/* is it number of cycle ?*/
	case 'n':
	    ncycle = atoi(optarg);
	    break;
	/* is it filename ?*/
	case 'f':
	    strcpy(results_filename, optarg);
	    break;
	/* is it config filename ?*/
	case 'g':
	    strcpy(config_filename, optarg);
	    break;
	/* is it offset ? */
	case 'o':
	    offset = atof(optarg);
	    break;
	/* is it recordPeriod */
	case 'p':
	    recordPeriod = atoi(optarg);
	    break;
	/* is it verbose_concise */
	case 'v':
	    verbose_concise = VERBOSE;
	    break;
	/* is it test and calibration mode */
	case 't':
	    test = TRUE;
	    break;
	/* is it language seclection */
	case 'l':
	    language = (optarg[0] == 'E' ? EN : FR);
	    break;
	/* is it offset vmin */
	case 'm':
	    vmin = atof(optarg);
	    break;
	/* is it offset vmaw */
	case 'M':
	    vmax= atof(optarg);
	    break;
	/* is it help */
	case 'h':
	    printf(language == EN ? "Syntax: chadeche <option>\n" :  "Syntaxe: chadeche <option>\n");
	    printf(language == EN ? "\t-a daugther board adress (0-3, def = %d)\n": "\t-a adresse de la carte mère (0-3, def = %d)\n", DAUGHTER_BOARD_ADRESS);
	    printf(language == EN ? "\t-i initial battery capacity (mAh, def = %d)\n" :  "\t-i charge initiale de la batterie (mAh, def = %d)\n", CINIT);
	    printf(language == EN ? "\t-c battery capacity min (mAh, def = %d)\n" : "\t-c charge min de la batterie (mAh, def = %d)\n", CMIN);
	    printf(language == EN ? "\t-C battery capacity max (mAh, def = %d)\n" : "\t-C charge mxn de la batterie (mAh, def = %d)\n", CMAX);
	    printf(language == EN ? "\t-n Repeat factor (def = %d)\n" : "\t-n facteur de répétition (def = %d)\n", NCYCLE);
	    printf(language == EN ? "\t-f results filename (def = %s)\n" : "\t-f nom du fichier de résultats (def = %s)\n",RESULTS_FILENAME);
	    printf(language == EN ? "\t-g config filename (def = %s)\n" : "\t-g nom du fichier de configuration (def = %s)\n" , CONFIG_FILENAME);
	    printf(language == EN ? "\t-p record period (seconds, def = %d)\n" :  "\t-p période d'enregistrement (secondes, def = %d)\n", RECORDPERIOD);
	    printf(language == EN ? "\t-v activate VERBOSE mode\n" :  "\t-v mode verbeux\n");
	    printf(language == EN ? "\t-t activate TEST mode\n" :  "\t-t mode TEST activé\n");
	    //printf("\t-o offset (volts, def = %5.4f)\n", OFFSET);
	    printf(language == EN ? "\t-m Vmin (default is %5.4f)\n" : "\t-m Vmin (def =  %5.4f)\n", VMIN);
	    printf(language == EN ? "\t-M vmin (default is %5.4f)\n" : "\t-M Vmax (def =  %5.4f)\n", VMAX);
	    printf(language == EN ? "\t-h this help\n" : "\t-h cette aide\n");
	    exit(1);
	default :
	    printf(language == EN ? "argManager : Unknown option or bad syntax\n" : "prmManager : option inconnue ou erreur de syntaxe\n");
	    exit(1);
    }
    }
}

/* affichage des paramètres */
void displayarg(void)
{
    printf(language == EN ? "Chadeche parameters:\n" : "Paramètres de Chadeche:\n");
    printf(language == EN ? "\tDaugther board adress : %d\n" : "\tAdresse de la carte Chadeche : %d\n", dba);
    printf(language == EN ? "\tInitial battery capacity : %d\n" : "\tCharge initiale de l'accu : %d\n", cinit); 
    printf(language == EN ? "\tBattery capacity min: %d\n" : "\tCharge min de l'accu : %d\n" , cmin); 
    printf(language == EN ? "\tBattery capacity max : %d\n" : "\tCharge max de l'accu : %d\n" , cmax); 
    printf(language == EN ? "\tResults filename : %s\n" : "\tNom du fichier de résultats : %s\n", results_filename);
    printf(language == EN ? "\tConfig filename : %s\n" : "\tNom du fichier de configuration : %s\n", config_filename);
    printf(language == EN ? "\tCycle repeat factor: %d\n" : "\tFacteur de répétiion de cycles : %d\n", ncycle); 
    printf(language == EN ? "\tOffset voltage : %5.4f\n" : "\tOffset en tension : %5.4f\n", offset);
    printf(language == EN ? "\tSlope voltage : %5.4f\n" : "\tPente en tension : %5.4f\n", slope);
    printf(language == EN ? "\tOffset current : %5.4f\n" : "\tOffset en courant : %5.4f\n", ioffset);
    printf(language == EN ? "\tSlope current : %5.4f\n" : "\tPente en courant : %5.4f\n", islope);
    printf(language == EN ? "\tVmin threshold : %5.4f\n" : "\tTension de seuil Vmin : %5.4f\n", vmin);
    printf(language == EN ? "\tVmax threshold : %5.4f\n" : "\tTension de seuil Vmax : %5.4f\n", vmax);
    printf(language == EN ? "\tRecord period in seconds: %4d\n" : "\tPériode d'enregistrement en secondes : %4d\n", recordPeriod);
    printf(language == EN ? "\tMode :  " :  "\tMode :  ");
	 verbose_concise == CONCISE ? printf(language == EN ? "CONCISE\n" : "DISCRET\n") : printf(language == EN ? "VERBOSE\n" : "BAVARD\n");
    printf(language == EN ? "\tMode :  " :  "\tMode :  ");
	 test == TRUE ? printf(language == EN ? "TEST ON\n" : "TEST activé\n") : printf(language == EN ? "TEST OFF\n" : "TEST désactivé\n");

}

#define NB_ARG 16
static struct argdata{
   char nom[20];
   void *pvalue;
   char format[10];
}targ[NB_ARG];


/* affichage des arguments sous forme de tableau */
void displayarg2(void)
{
    printf("==========================================================================================================\n");
    printf("|                                          PARAMETRES/PARAMETERS                                         |\n");
    printf("==========================================================================================================\n");
    printf("| ADB | Nb. cycles | Trec | Mode  | Test | Seuil Vmin | Seuil Vmax | Fichier de configuation\n");
    printf("----------------------------------------------------------------------------------------------------------\n");
    printf("|%5d|%12d|%6d|%7s|%6s|%12.5f|%12.5f|%s\n", dba, ncycle, recordPeriod, verbose_concise==CONCISE ? "DISCRET" : "BAVARD", test == TRUE ? "ON" : "OFF", vmin, vmax, config_filename); 
    printf("==========================================================================================================\n");
    printf("| Seuil Cmin | Seuil Cmax | Pente V | Offset V | Pente I | Offset I | Fichier de résultats\n");
    printf("----------------------------------------------------------------------------------------------------------\n");
    printf("|%12d|%12d|%9.5f|%10.5f|%9.5f|%10.5f|%s\n", cmin, cmax, slope, offset, islope, ioffset, results_filename);
    printf("==========================================================================================================\n");
}

/* test sur la valeur de certains arguments */
int argtest(void)
{
    if(vmin < VMAX_OPEN_ON_DISCHARGE){
	printf(language == EN ? "Vmin cannot be lower than %5.4f Volt(s)\n" : \
				"Vmin ne peut pas être inférieur à %5.4f Volts(s)\n", VMAX_OPEN_ON_DISCHARGE);
	return -1;
    }

    if(vmax > VMAX_OPEN_ON_CHARGE){
	printf(language == EN ? "Vmin cannot be greter than %5.4f Volt(s)\n" : \
				"Vmin ne peut pas être supérieur à %5.4f Volts(s)\n", VMAX_OPEN_ON_CHARGE);
	return -1;
    }
    return 0;
}
