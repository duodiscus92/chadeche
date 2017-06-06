 /*
	chadeche by J. Ehrlich
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
//void CtrlZHandler(int sig)
//{
//    controlflag=1;
//}
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

/* data for shared memory */
struct procdesc {
    int nprocess;
    int tprocess[NB_BOARD];
};
struct procdesc *p;

/*******************************/
/* C'est ici que tout commence */
/*******************************/
int main(int argc, char **argv)
{
    int i, j, repeat, step, dt, peakdetected=0, milliampScaled/*, stepmAh*/, mAh;
    unsigned int initialData, currentData, milliamp;
    double voltage, fmAh;
    time_t t, totaltime, cycletime, elapsedtime, steptime, looptime, waketime;
    struct tm *pt;
    FILE *fp;
    char cop, decision;
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
    displayarg();
    /* lecture et mémorisation fichier de config */
    readconf(config_filename);
    /* affichage de le confirguration */
    displayconf();

    /*i = */elapsedtime = 0;

    /* installing the Ctrl-C handler */
    signal(SIGINT, CtrlCHandler);
    /* installing the Ctrl-Z handler */
    act.sa_sigaction = &CtrlZHandler;
    act.sa_flags = SA_SIGINFO;
    //signal(SIGTSTP, CtrlZHandler);
    if(sigaction(SIGTSTP, &act, NULL) <0){
	perror("sigaction");
	exit(1);
    }
#define  SEMMODULE
#ifdef SEMMODULE /* il y existe un module sem.c qui fonctionne */
    if (initmem() == -1) exit(1);
    if (initsem() == -1) exit(1);
    if (subscribe(dba) == -1){
	termsem();
	termmem();
	exit(1);
    }
#else /* pas de module sem.c ou bien le module ne fonctionne pas */
#ifdef SHAREDMEMORY
    /* shared memory creation */
#ifdef DEBUG
    printf("Trying to create shared memorey using name = %s\n", MEM_NAME);
#endif
    if ((mid= shm_open(MEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )) == -1) {
	//perror("shm_open : warning : shared memory already exists");
	if ((mid= shm_open(MEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )) == -1) {
	    perror("shm_open : error : memory already exists and unable to opent it");
	    exit(1);
	}
#ifdef DEBUG
	else
	   printf("An already exisitng shared memory will be used with mid = %ld\n", mid);
#endif
    }
#ifdef DEBUG
    else
	printf("A new shared memory is created with mid = %ld\n", mid);
#endif
    /* shared memory mapping */
    ftruncate(mid, sizeof(struct procdesc));
    if ((p = mmap(p, sizeof(struct procdesc), PROT_READ | PROT_WRITE, MAP_SHARED, mid, 0)) == (struct procdesc *)-1) {
	perror("mmap: unable to map shared memory");
    	if (shm_unlink(MEM_NAME) == -1)
	    perror("shm_unlink: unable to free shared memory");
	exit(1);
    }
#ifdef DEBUG
    else
	printf("mmap: memory mapped\n");
#endif
#endif

    /* semaphore creation and initialization */
    semaphore = sem_open("/semaphore", O_CREAT | O_RDWR | O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
	//perror("/semaphore");
	semaphore = sem_open("/semaphore", O_RDWR);
	if (semaphore == SEM_FAILED) {
	    perror("/semaphore : Unable to open existing semaphore\n");
#ifdef SHAREDMEMORY
    	    if (munmap(p, sizeof(struct procdesc)) == -1)
	    	perror("munmap : unable to unmap shared memry");
#endif
	    exit(1);
	 }
     }
     else{
	firstcall = 0;
#ifdef DEBUG
	printf("semaphore  created\n");
#endif
     }
#ifdef SHAREDMEMORY
     /* shared memory test and initialisation */
     sem_wait(semaphore);
     if(p->tprocess[dba] != 0){
	sem_post(semaphore);
	printf(language == EN ? "Program aborted !!! A process nr. %d is already using this board nr. %d\n" : \
				"Progamme abandonné !!! Un processus n° %d utilise déjà cetee carte n° %d\n", p->tprocess[dba], dba);
    	if (munmap(p, sizeof(struct procdesc)) == -1)
	    perror("munmap : unable to unmap shared memry");
#ifdef DEBUG
	else
	    printf("Unmapping shared memory before exiting\n");
#endif
	exit(1);
     }
     else {
     	p->nprocess++;
     	p->tprocess[dba] = p->nprocess; 
     	sem_post(semaphore);
#ifdef DEBUG
	printf("Process nr. %d launched : Using the board %d\n", p->nprocess, dba);
#endif
     }
#endif
#endif /*SEMMODULE */
     /* Rpi initialization */
    initchadeche();

    /* end led (red) off */
    begled();

    /* settintgs and pre-tests */
    printf(language == EN ? "Setting current to zero and waiting 5 seconds ...\n" : "Courant à zéro et attente de 5 secondes ...\n");
    mcp4922write(0,0);
    delay(5000);

    /* wait for battery presence before starting test (excepted if -t optiont used )*/
    if(test == FALSE){
	if((currentData=mcp3201read()) == 0 ){
	   printf(language == EN ? "Waiting for battery presence\n" : "Attente présence accu\n");
	   do {
	      printf("."); fflush(stdout);
	       delay(1000);
	   } while ((currentData = mcp3201read()) == 0);
	   printf(language == EN ? "\nBattery presence detected. Test will start in 5 seconds ...\n" : "Présence de l'accu détectée. L'essai commence dans 5 secondes ... \n");
	   delay(5000);
	}
    }
    else{
	currentData=mcp3201read();
	printf(language == EN ? "\nTest will start in 5 seconds ...\n" : "L'essai commence dans 5 secondes ... \n");
	delay(5000);
    }


    /* verify that Vcell don't exceed Vmax_open */
    //if((voltage = (double)currentData*5.1/4096 + offset) >= VMAX_OPEN)
    if((voltage = slope*(double)currentData/1000 + offset) >= VMAX_OPEN){
	decisioncause = VMAXOPEN_DETECT;
	goto abort;
    }

   /* creating  the file to record results */
    //if((fp = fopen(results_filename,  "w+x"))==NULL){
    if((fp = fopen(results_filename,  "a+"))==NULL){
	printf("Program aborted : can't open results file : %s\n",results_filename);
#ifdef SEMMODULE
	if(unsubscribe(dba)== 0){
	    termsem();
	    termmem();
	}
#else
#ifdef SHAREDMEMORY
	sem_wait(semaphore);
 	p->nprocess--;
 	p->tprocess[dba] = 0; 
	i = p->nprocess;
	sem_post(semaphore);
	if (munmap(p, sizeof(struct procdesc)) == -1)
	    perror("munmap : unable to unmap shared memory");
#ifdef DEBUG
	else
	    printf("Unmapping shared memory befor exiting\n");
#endif
	/* if this process is the last one */
 	if(i == 0){
#ifdef DEBUG
	    printf("Last process aborted : removing semaphore and shared memory\n");
#endif
	    sem_unlink("/semaphore");
	    if (shm_unlink(MEM_NAME) == -1)
	    	perror("shm_unlink: unable to free shared memory");
	}
#endif
#endif /* SEMMODULE */
	exit(1);
    }

    /* main measurement loop */
    time(&totaltime);
    /* cycle loop */
    fmAh=cinit; elapsedtime = 0;
    ongoingled();
    for (repeat =0; (repeat < ncycle) ; /*repeat++*/){
	printf(language == EN ? "Starting cycle nr.:%2d\n" :  "Lancement cycle nr.:%2d\n", repeat+1);
    	/* initializing time in second */
    	time(&cycletime);
	step = 0; /*stepmAh=0*/;
	/* step loop */
	//while( (tconfig[step].cop !=0) ){
	for(step = 0;  (tconfig[step].cop !=0); step++){
	    /*mAh += stepmAh;*/
	    time(&steptime);
	    t = steptime;
	    //printf("Starting step nr.: %2d: %c\tmA=%d\tDuration=%d\tmAh=%d\tE.T.=%d\t%s",
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
	        //voltage = (double)currentData*0.9758*5.1/4096 + offset;
                //voltage = (double)currentData*5.1/4096 + offset;
		decision = 'I'; // Ignore
		decisioncause = NO_CAUSE;
		if(stopflag == 1 || peakdetected)
		    goto abort;
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
		/* appliquer la décision */
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

    if (peakdetected)
    	printf (language == EN ? "\nBattery test ends because battery peak detected with voltage = %5.3f\n" :  \
				 "\nFin d'essai car un pic de tension a été détecté = %5.3f volts\n", voltage);
    else if (stopflag==1)
    	printf (language == EN ? "\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n" : \
				 "\nFin d'essai à cause d'un  Ctrl-C. Tension accu = %5.3f\n" , voltage);
    /* testing why the test ends */
    else{
    switch(decisioncause) {
    case VMAXOPEN_DETECT:
    	printf (language == EN ? "\nBattery test ends because battery voltage = %5.3f >= %4.2f Volts (battery removed)\n" : \
				 "\nFin d'essai car tension de l'accu = %5.3f  >= %4.2f Volts (accu absent)\n", voltage, VMAX_OPEN);
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
    }

//endchadeche:
    printf(language == EN ? "Setting the discharge current to zero\n" : "Remise à zéro du courant de charge ou décharge\n");
    mcp4922write(0,0);
    /* switching the end led (red) on */
    printf(language == EN ? "Type Ctrl-C to quit definitively\n" : "Tapez Ctrl-C pour quitter définitvement\n");
    stopflag = 0;
    while(stopflag == 0) {
	//digitalWrite (ENDLED, LOW) ;
	charge();
	delay(2000);
        //digitalWrite (ENDLED, HIGH) ;
	discharge();
	delay(2000);
    }
    //digitalWrite (ENDLED, LOW) ;
    endled();
    discharge();
    /* clear shared memory */
#ifdef SEMMODULE
    if(unsubscribe(dba) == 0){
	termsem();
    	termmem();
    }
#else
#ifdef SHAREDMEMORY
    sem_wait(semaphore);
    p->nprocess--;
    p->tprocess[dba] = 0; 
    i = p->nprocess;
    sem_post(semaphore);
    if (munmap(p, sizeof(struct procdesc)) == -1)
	perror("munmap : unable to unmap shared memry");
#ifdef DEBUG
    else
	printf("Unmapping shared memory befor exiting\n");
#endif
    /* if this process is the last one */
    if(i == 0){
#ifdef DEBUG
	printf("Last process ending : removing semaphore and shared memory\n");
#endif
	if (shm_unlink(MEM_NAME) == -1)
	    perror("shm_unlink: unable to free shared memory");
    	sem_unlink("/semaphore");
    }
#endif
#endif /*SEMMODULE*/
    printf(language == FR ? "\nAu revoir !!!\n" : "\nBye !!!\n");
    return 0;
}
