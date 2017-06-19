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



/* parametres et valeurs par défaut */
LANGUAGE language = FR;
int	verbose_concise = CONCISE,
	recordPeriod = RECORDPERIOD,
	cmin = CMIN,
	cmax = CMAX,
	cinit = CINIT,
	ncycle = NCYCLE,
	dba = DAUGHTER_BOARD_ADRESS,
	langage = FR,
	test = FALSE;
double 	offset = OFFSET, slope = SLOPE,		// pente et offset tension
        ioffset = OFFSET, islope = SLOPE, 	// pente et offset courant
	vmax = VMAX,
	vmin = VMIN;
char 	results_filename[80]=RESULTS_FILENAME,
	config_filename[80]=CONFIG_FILENAME;

/* semaphore for exclusive access to Rpi */
sem_t *semaphore = NULL;


volatile int stopflag=0;
/* Ctrl-C interrupt handler */
void CtrlCHandler(int sig)
{
    stopflag=1;
}

volatile int controlflag=0;
/* Ctrl-Z interrupt handler */
void CtrlZHandler(int sig, siginfo_t *siginfo, void * context)
{
    controlflag=1;
}


/* return a step number from its adress */
int adress2step(int adress)
{
    int step;

    for (step = 0; tconfig[step].cop != 0; step++)
	if (tconfig[step].adr == adress)
	    return step;
    return -1;
}

/* lit un car et vérifie qu'il est dans une liste de choix possibles */
int smartgetchar(char *choix)
{
   char  c;
   int i = 0;

   while(1){
	//fflush(stdin);
	read(0, &c, 1);
	for(i=0; i<strlen(choix); i++)
	   if(c == choix[i])
		return c;
	read(0, &c, 1);
	printf(language == EN ? "Unauthorized selection\n" : "Selection non autorisée\n"); 
	printf(language == EN ? "Type another selection : " : "Tapez une autre sélection : ");
	fflush(stdout);
   }
}


/*******************************/
/* C'est ici que tout commence */
/*******************************/
int main(int argc, char **argv)
{
    int i, j, repeat, step, dt, /*peakdetected=0,*/ milliampScaled/*, stepmAh*/, mAh;
    unsigned int initialData, currentData, milliamp;
    double voltage, fmAh;
    time_t t, totaltime, cycletime, elapsedtime, steptime, looptime, waketime;
    struct tm *pt;
    FILE *fp;
    char cop, decision, c;
    char *str, *endptr; // for call to function strtol
    int jumpadress; // for Jump
    struct sigaction act; // for ctrlZ management

    /* end of step cause id */
    enum cause {
	NO_CAUSE,
	ALWAYS,
	VMAXOPEN_DETECT,
	TOOLOW_DETECT, TOOHIGH_DETECT, 
	TOOLOWCAPACITY_DETECT, TOOHIGHCAPACITY_DETECT,
	CONTROLZ_DETECT} decisioncause = NO_CAUSE;

    /* message to explain end fo step cause */
    char *msgcause[][2]= {
	"Début normal de séquence","Normal step starting",
	"Saut inconditionnel ou reveil d'une séquence endormie", "Unconditional jump or sleeping process awaked",
	"Dépassement de tension ou circuit accu non déconnecté", "Battery voltage overflow on open cicuit", 
	"Tension de l'accu < au seuil bas", "Battery voltage < low threshold",
	"Tension de l'accu > au seuil haut", "Battery voltage > high threshold",
	"Charge de l'accu < au seuil bas", "Battery capacity underflow",
	"Charge de l'accu > au seuil haut", "Battery capacity overflow",
	"Control-Z", "Contrl-Z",
    };

    int mid;

    /* gestion des arguments uniquement pour avoir le n° de la carte*/
    argManager(argc, argv);
    /* gestion du fichier de paramètres */
    prmManager();
    /* gestion des arguments cette fois-ci c'est pour tous les paramètres*/
    argManager(argc, argv);
    /* affichage des paramètres */
    //displayarg();
    displayarg2();
    /* test de certains arguments */
    printf(language == EN ? "Testing some parameters and abort if values are wrong\n" : \
			    "Test de certains paramètres et abandon si des valeurs sont mauvaises\n");
    if(argtest() == -1)
	exit(1);
    printf(language == EN ? "Test successful\n" : "Test réalisé avec succès\n");
    /* lecture et mémorisation fichier de config */
    readconf(config_filename);
    /* affichage de le confirguration */
    displayconf();
    /* ouverture du fichier pour enregistrer les résultats */
    /* s'il existe pas on le crée sinon on enregistre à la suite de ce qui existe déjà */
    if((fp = fopen(results_filename,  "a+" /* "w+x" */))==NULL){
	printf(language == EN ? "Program aborted : can't open results file : %s\n" : \
				"Programme abandonné : impossible d'ouvir le fichier de résultats: %s\n" ,results_filename);
	exit(1);
    }

    /* installation du Ctrl-C handler */
    signal(SIGINT, CtrlCHandler);
    /* installation du Ctrl-Z handler */
    act.sa_sigaction = &CtrlZHandler;
    act.sa_flags = SA_SIGINFO;
    //signal(SIGTSTP, CtrlZHandler);
    if(sigaction(SIGTSTP, &act, NULL) <0){
	perror("sigaction");
	exit(1);
    }
    /* creation et mapping  mémoire partagée */
    if (initmem() == -1) exit(1);
    /* creation sémaphores */
    if (initsem() == -1) exit(1);
    /* enregistrement du Chadeche pour la carte selectionnée */
    /* et abandon si un chadeche a déjà lancé sur cette carte */
    if (subscribe(dba) == -1){
	termsem();
	termmem();
	exit(1);
    }

    /* Intialisation du hardware (voir module hw.c */
    initchadeche();
    //delay(1000);

    /* clognotement led pour début essai */
    begled();

    /* settintgs and pre-tests */
    printf(language == EN ? "Setting current to zero and waiting 5 seconds ...\n" : "Courant à zéro et attente de 5 secondes ...\n");
    for(i=0;i<10;i++){
    	discharge();
    	delay(100);
    	charge();
    	delay(100);
    }
    mcp4922write(0,0);
    delay(3000);

    /* wait for battery presence before starting test (excepted if -t optiont used )*/
    /* ici il y a un problème car parfois la batterie n'est pas détectée ... je cherche ... */
    if(test == FALSE){
	currentData=mcp3201read();
	//printf("%d\n", currentData);
	voltage = slope*(double)currentData/1000 + offset;
	//if((currentData=mcp3201read()) > VMAX_OPEN ){
	if(voltage > VMAX_OPEN_ON_CHARGE ){
	   printf(language == EN ? "Waiting for battery presence\n" : "Attente présence accu\n");
	   do {
	      printf("."); fflush(stdout);
	      delay(1000);
	      currentData=mcp3201read();
	      voltage = slope*(double)currentData/1000 + offset;
	   } while (voltage > VMAX_OPEN_ON_CHARGE);
	   printf(language == EN ? "\nBattery presence detected. Test will start in 5 seconds ...\n" : "Présence de l'accu détectée. L'essai commence dans 5 secondes ... \n");
	   delay(5000);
	}
    }
    else{
	printf(language == EN ? "\nTest will start in 5 seconds ...\n" : "L'essai commence dans 5 secondes ... \n");
	delay(5000);
	currentData=mcp3201read();
    }


    /* verify that Vcell don't exceed Vmax_open */
    if((voltage = slope*(double)currentData/1000 + offset) >= VMAX_OPEN_ON_CHARGE){
	decisioncause = VMAXOPEN_DETECT;
	goto abort;
    }

    /* main measurement loop */
    time(&totaltime);
    /* cycle loop */
    fmAh=cinit; elapsedtime = 0;
    /* clignetement le sur essai en cours */
    ongoingled();
    for (repeat =0; (repeat < ncycle) ; /*repeat++*/){
	printf(language == EN ? "Starting cycle nr.:%2d\n" :  "Lancement cycle nr.:%2d\n", repeat+1);
    	/* initializing time in second */
    	time(&cycletime);
	step = 0; /*stepmAh=0*/;
	/* step loop */
	for(step = 0;  (tconfig[step].cop !=0); step++){
	    /*mAh += stepmAh;*/
	    time(&steptime);
	    t = steptime;
	    printf(language == EN ? "Starting step nr.%2d (%s): %s" : "Lancement séquence nr.%2d (%s): %s", step+1,  msgcause[decisioncause][language], tconfig[step].comment);
	    /* set the relay for the current mode */
	    (cop=tconfig[step].cop) == 'D' ? discharge() : charge() ;
	    /* set the current to the desired value */
	    milliampScaled = (tconfig[step].milliamp*islope+ioffset)*10;
	    mcp4922write(milliampScaled,0);

	    j=0;
	    /* data acquisition and logging loop */
	    while( (t-steptime) <= tconfig[step].duration ){
		/* read battery voltage */
      		currentData = mcp3201read();
	        voltage = slope*(double)currentData/1000 + offset;
		decision = 'I'; // Ignore
		decisioncause = NO_CAUSE;
		if(stopflag == 1 /*|| peakdetected*/){
		    /* annonce la suspension d'essai */
    		    printf(language == EN ? "\nSetting the discharge current to zero\n" : "\nRemise à zéro du courant de charge ou décharge\n");
    		    mcp4922write(0,0);
		    printf(language == EN ? "Test suspended ...\n" : "Essai suspendu ...\n");
    		    printf(language == EN ? "Type Q or q <ENTER> to quit definitively or R o r key to resume : " : "Tapez Q ou q  <ENTREE> pour quitter définitvement ou R ou r pour reprendre : ");
		    fflush(stdout);
		    //read(0, &c, 1);
		    if((c=smartgetchar("QqRr")) == 'Q' || c == 'q'){
		    //if(c == 'Q'){
		        //fflush(stdin);
		        goto abort;
		    }
		    else if (c== 'R' || c == 'r'){
	    		stopflag = 0;
			//fflush(stdin);
			printf(language == EN ? "Resuming ...\n" : "Reprise ...\n");
			continue;
		    }
		}
		/* absence batterie */
		if((tconfig[step].cop == 'C' && voltage >= VMAX_OPEN_ON_CHARGE) ||
		   (tconfig[step].cop == 'D' && voltage <= VMAX_OPEN_ON_DISCHARGE) ){
			decisioncause = VMAXOPEN_DETECT;
			goto abort;
		}
		if(voltage <= vmin){
		    //printf("Event : V < Vmin\n");
	    	    if( strchr((str = tconfig[step].toolow), ':') == NULL){ 
	    	    	jumpadress = strtol(str,  &endptr, 10);
		    	decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    	    }
	    	    else
		    	decision = 'W';
		    fmAh = 0;
		    decisioncause = decision == 'I' ? NO_CAUSE : TOOLOW_DETECT;
		}
		if(voltage >= vmax && decision == 'I'){
		    //printf("Event : V > Vmax\n");
	    	    if( strchr((str = tconfig[step].toohigh), ':') == NULL){ 
	    	    	jumpadress = strtol(str,  &endptr, 10);
		    	decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    	    }
	    	    else
		    	decision = 'W';
		    decisioncause = decision == 'I' ? NO_CAUSE : TOOHIGH_DETECT;
		}
		if( (fmAh /*+ stepmAh*/) < cmin /*&& cop == 'D'*/ && decision == 'I'){
		    //printf("Event : batt. capacity underflow\n");
	    	    if( strchr((str = tconfig[step].toolowcapacity), ':') == NULL){ 
	    		jumpadress = strtol(str,  &endptr, 10);
		    	decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    	    }
	    	    else
			decision = 'W';
		    decisioncause = decision == 'I' ? NO_CAUSE : TOOLOWCAPACITY_DETECT;
		}
		if( (fmAh /*+ stepmAh*/) > cmax /*&& cop == 'C'*/ && decision == 'I'){
		    //printf("Event : batt. capacity overflow\n");
	    	    if( strchr((str = tconfig[step].toohighcapacity), ':') == NULL){ 
	    		jumpadress = strtol(str,  &endptr, 10);
		    	decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    	    }
	    	    else
			decision = 'W';
		    decisioncause = decision == 'I' ? NO_CAUSE : TOOHIGHCAPACITY_DETECT;
		}
		if(controlflag == 1 && decision == 'I'){
		    //printf("Event : Ctrl-Z\n");
		    controlflag = 0;
	    	    if( strchr((str = tconfig[step].controlflag), ':') == NULL){ 
	    		jumpadress = strtol(str,  &endptr, 10);
		    	decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    	    }
	    	    else
			decision = 'W';
		    decisioncause = decision == 'I' ? NO_CAUSE : CONTROLZ_DETECT;
		}
		if(decision == 'I'){
		    //printf("Event : Always\n");
	    	    if( strchr((str = tconfig[step].always), ':') == NULL){ 
	    		jumpadress = strtol(str,  &endptr, 10);
		    	decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    	    }
	    	    else
			decision = 'W';
		    decisioncause = decision == 'I' ? NO_CAUSE : ALWAYS;
		}
		/* verbose mode */
		//if((verbose_concise == VERBOSE) && !((t-cycletime) % 10) ){
		dt = verbose_concise == VERBOSE ? 10 : recordPeriod; 
		if(!((t-cycletime) % dt) ){
            	    printf("%c/%02d/%02d DBA=%1d mA=%4d CET=%6ld SET=%6ld TET=%6ld STA=%6d V=%5.3f mAh=%5.1f %s", 
			/* code operation */cop,/* nr cycle */  repeat+1, /* nr step */step+1, /*daugher baord adress */dba, 
			/* consigne courant */tconfig[step].milliamp,
			/* CET */t-cycletime, /* SET */t-steptime , /* TET*/ t-totaltime,  /* STA */tconfig[step].duration-t+steptime,
			/* battery voltage */voltage, /* current charge level*/fmAh, ctime(&t));
            	    fprintf(fp, "%c;%02d;%02d;%1d;%4d;%6ld;%6ld;%6ld;%6d;%5.3f;%5.1f;%s", 
			/* code operation */cop,/* nr cycle */  repeat+1, /* nr step */step+1, /*daugher baord adress */dba, 
			/* consigne courant */tconfig[step].milliamp,
			/* CET */t-cycletime, /* SET */t-steptime , /* TET*/ t-totaltime,  /* STA */tconfig[step].duration-t+steptime,
			/* battery voltage */voltage, /* current charge level*/fmAh, ctime(&t));
	    	    fflush(fp);
        	}
	    	else if(verbose_concise == CONCISE){
		    printf("."); fflush(stdout);
                    if((j%50) == 0) printf("\n");
            	}
		j++;
		/* wait for 1 sec before next battery voltage measurement */
	    	delay(1000);
	    	//time(&t);
		/* estimate of mAh in the step */
		time(&looptime);
		elapsedtime = looptime-t;
		t = looptime;
	   	/*step*/fmAh += ( (tconfig[step].milliamp * /*(t-steptime)*/elapsedtime/3600.0) * (tconfig[step].cop == 'C' ? 1 : -1) );
		if (fmAh <= 0) fmAh = 0;
		/* appliquer la décision (y a des goto, c'est pas beau, mais c'est le plus simple)*/
mngdec:		switch(decision){
		    case 'A' : 	goto abort;		// abort
		    case 'S' : 	goto nextstep;	// next step
		    case 'C' : 	goto nextcycle;	// next cycle
		    case 'J' : 	step = adress2step(jumpadress)-1;	// Jump
			       	goto nextstep;
		    case 'I' : 	break;		// ignore (do nothin)
		    case 'W' : 	pt = localtime(&t);	//sleep until waketime
				sscanf(str, "%d:%d", &(pt->tm_hour), &(pt->tm_min));
				pt->tm_sec = 0;
				pt->tm_isdst = 0; /*  don't use daytime saving  */
				printf(language == EN ? "Sleeping until %d:%02d\n" : "Séquence en sommeil jusqu'à %d:%02d\n", pt->tm_hour, pt->tm_min);
				waketime = mktime(pt)-t;
				if (waketime  < 0 ) waketime +=  24*3600;
				printf(language == EN ? "Will be awaked in %d seconds\n" : "Sera reveillé dans %d secondes\n", waketime);
				mcp4922write(0,0); /* set current to 0 before sleeping */
				sleep(waketime);
				goto nextstep; /* when awaked start next step */
		    default :  	break;
		}
	    } /* end data acquisition loop */
nextstep:;   /*step++;*/
	} /* end step loop */
nextcycle:  repeat++;
    } /* end cycle loop */
abort:
    fclose(fp);

    /*if (peakdetected)
    	printf (language == EN ? "\nBattery test ends because battery peak detected with voltage = %5.3f\n" :  \
				 "\nFin d'essai car un pic de tension a été détecté = %5.3f volts\n", voltage);
    else*/ if (stopflag==1)
    	printf (language == EN ? "\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n" : \
				 "\nFin d'essai à cause d'un  Ctrl-C. Tension accu = %5.3f\n" , voltage);
    /* testing why the test ends */
    else{
    switch(decisioncause) {
    case VMAXOPEN_DETECT:
    	printf (language == EN ? "\nBattery test ends because battery voltage = %5.3f <= %4.2f Volts or >= %4.2f Volts (battery removed)\n" : \
				 "\nFin d'essai car tension de l'accu = %5.3f <= %4.2f Volts ou  >= %4.2f Volts (accu absent)\n", \
				voltage, VMAX_OPEN_ON_DISCHARGE, VMAX_OPEN_ON_CHARGE);
	break;
    case TOOLOW_DETECT:
    	printf (language == EN ? "\nBattery test ends because battery voltage = %5.3f <= %4.2f Volts (low threshold)\n" : \
				 "\nFin d'essai car tension de l'accu = %5.3f <= %4.2f Volts (seuil bas)\n", voltage, vmin);
	break;
    case TOOHIGH_DETECT:
    	printf (language == EN ? "\nBattery test ends because battery voltage = %5.3f >= %4.2f Volts (high threshold)\n" : \
				 "\nFin d'essai car tension de l'accu = %5.3f >= %4.2f Volts (seuil haut)\n", voltage, vmax);
	break;
    case TOOLOWCAPACITY_DETECT:
    	printf (language == EN ? "\nBattery test ends due to capacity underflow = %5.1f <= %5.1f mAh\n" : \
				 "\nFin d'essai car charge estimée = %5.1f <= %5.1f mAh (seuil bas)\n", fmAh, cmin);
	break;
    case TOOHIGHCAPACITY_DETECT:
    	printf (language == EN ? "\nBattery test ends due to capacity overflow = %5.1f >= %5.1f mAh\n" : \
				 "\nFin d'essai car charge estimée = %5.1f >= %5.1f mAh (seuil haut)\n",  fmAh, cmax);
	break;
    case CONTROLZ_DETECT:
    	printf (language == EN ? "\nBattery test ends due to Ctrl-Z occurence. Battery voltage = %5.3f\n" : \
				 "\nFin d'essai à cause d'un Ctrl-Z. Tension accu = %5.3f\n" , voltage);
	break;
    case ALWAYS:
    	printf (language == EN ? "\nBattery test ends due to unconditionnal abort. Battery voltage = %5.3f\n" : \
				 "\nFin d'essai sur occurence d'un arrêt inconditionner. Tension batterie = %5.3f\n", voltage);
	break;
    default:
        printf (language == EN ? "\nBattery test because test is complete (normal condition)\n" : \
				 "\nFin d'essai car la dernières séquence a été exécutée (fin normale d'essai)\n");
	break;
    }

    /* annonce la fin d'essai */
    printf(language == EN ? "Setting the discharge current to zero\n" : "Remise à zéro du courant de charge ou décharge\n");
    mcp4922write(0,0);
    printf(language == EN ? "Type Ctrl-C to quit definitively\n" : "Tapez Ctrl-C pour quitter définitvement\n");
    stopflag = 0;

    /* ça fait clignoter la led bicolore de rouge a vert et réciproquement */
    /* c'est pour indiquer que l'essai est terminé */
    while(stopflag == 0) {
	//digitalWrite (ENDLED, LOW) ;
	charge();
	delay(2000);
        //digitalWrite (ENDLED, HIGH) ;
	discharge();
	delay(2000);
    }
    }

    endled();
    charge();

/* nettoyage memoire partagée et semaphore */
    if(unsubscribe(dba) == 0){
	termsem();
    	termmem();
    }

    /* Tchao */
    printf(language == FR ? "\nAu revoir !!!\n" : "\nBye !!!\n");
    return 0;
}
