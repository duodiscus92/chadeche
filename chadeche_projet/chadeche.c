/*
	chadeche by J. Ehrlich
	Use wiringPi Interface Library released under the GNU LGPLv3 license
		http://wiringpi.com
 */

#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>



#define A0	0	//GPIO 17
#define A1	2	//GPIO 27
#define	CS0	4	//GPIO 23 CS0
#define	CS1	5	//GPIO 24 CS1
#define	REL	7	//GPIO 4 commande relais
#define REL_TEMPORARY	3	//GPIO 22 commande relais carte adresse 1 (temporaire pour cause erreur hardware)
#define ENDLED  6	//GPIO 6 End of test (red led)

#define CHARGE HIGH
#define DISCHARGE LOW

typedef enum lang{FR, EN} LANGUAGE;

/* default values */
#define CMIN 100 /* mAh */
#define CMAX 1300 /* mAh */
#define VMAX_OPEN 1.65 /* Vcell max when I = 0 */
#define VMAX 1.75 /* Vcell max during charge */
#define VMIN 1.0 /* Vcell min */
#define OFFSET 0
#define RESULTS_FILENAME "chadeche-results.csv"
#define CONFIG_FILENAME "chadeche-config.csv"
//#define RECORDTHRESHOLD 1	/* used only when record mode is RECORD_BY_VOLTAGE */
#define CONCISE 0
#define VERBOSE 1
#define RECORDPERIOD 120 	/* used only when record mode is RECORD_BY_TIME */
#define NCYCLE 1
#define DAUGHTER_BOARD_ADRESS 0

/* parameters et valeurs part défaut */
LANGUAGE language = EN;
int	verbose_concise = CONCISE,
	recordPeriod = RECORDPERIOD,
	cmin = CMIN,
	cmax = CMAX,
	ncycle = NCYCLE,
	dba = DAUGHTER_BOARD_ADRESS,
	langage = FR;
double 	offset = OFFSET,
	vmax = VMAX,
	vmin = VMIN;
char 	results_filename[80]=RESULTS_FILENAME,
	config_filename[80]=CONFIG_FILENAME;

/* semaphore for exclusive access to Rpi */
static sem_t *semaphore = NULL;


typedef struct config {
	int adr;
	char cop;
	int  milliamp;
	int duration;
	char toolow[5];
	char toohigh[5];
	char controlflag[5];
	char comment[80];
} CONFIG;

CONFIG tconfig[50];
/* read the config file and store values into tconfig */
void readconf(char *filename)
{
	FILE *fp;
	int i = 0;
	char buffer[80];

	if ((fp = fopen(filename, "r")) == NULL){
	    printf("Program aborted : can't open config file %s\n", filename);
	    exit(1);
	}

	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    sscanf(buffer, "%d;%c;%d;%d;%[A-Z0-9\\-];%[A-Z0-9\\-];%[A-Z0-9\\-]\n",
		&(tconfig[i].adr),
		&(tconfig[i].cop),
		&(tconfig[i].milliamp),
		&(tconfig[i].duration),
		tconfig[i].toolow,
		tconfig[i].toohigh,
		tconfig[i].controlflag);
	    strcpy(tconfig[i].comment, strchr(buffer, '#')+1);
	    i++;
	}
	tconfig[i-1].cop = 0; // pour marquer le fin de config
}

int min(int a, int b)
{
	return (a < b ? a : b);
}

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


/* chadeche hardware initialisation */
/* this function may be called only by the first process launched */
/* therefore, msemaphore are not mandattory here */
int firstcall = 1;
void initchadeche(void)
{
    sem_wait(semaphore);
    wiringPiSetup () ;
    if(firstcall == 1){
    	pinMode (A0, OUTPUT) ;
    	pinMode (A1, OUTPUT) ;
    	pinMode (CS0, OUTPUT) ;
    	pinMode (CS1, OUTPUT) ;
	pinMode (dba == 0 ? REL : REL_TEMPORARY, OUTPUT) ;
    	pinMode (ENDLED, OUTPUT) ;

    	digitalWrite (CS0, LOW) ; // deselect DAC
    	digitalWrite (CS1, LOW) ; // deselect CAN
    	digitalWrite (dba == 0 ? REL : REL_TEMPORARY, DISCHARGE) ;  // discharge mode
     }
     wiringPiSPISetup(0, 100000); // init SPI interface
     sem_post(semaphore);
}

/* endled off */
void endledoff(void)
{
    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   // adresse A0=0
    digitalWrite (A1, dba&0x02) ;   // adresse A1=0
    delay(10); // attente stabilisation
    digitalWrite (ENDLED, HIGH) ;
    sem_post(semaphore);
}

/* endled on */
void endledon(void)
{
    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   // adresse A0=0
    digitalWrite (A1, dba&0x02) ;   // adresse A1=0
    delay(10); // attente stabilisation
    digitalWrite (ENDLED, LOW) ;
    sem_post(semaphore);
}

/* relay on charge position */
int charge(void)
{
    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   // adresse A0=0
    digitalWrite (A1, dba&0x02) ;   // adresse A1=0
    delay(10); // attente stabilisation
    digitalWrite (dba == 0 ? REL : REL_TEMPORARY, CHARGE) ;  // charge mode
    sem_post(semaphore);
    return 1;
}

/* relay on discharge position */
int discharge(void)
{
    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   // adresse A0=0
    digitalWrite (A1, dba&0x02) ;   // adresse A1=0
    delay(10); // attente stabilisation
    digitalWrite (dba == 0 ? REL : REL_TEMPORARY, DISCHARGE) ;  // discharge mode
    sem_post(semaphore);
    return 0;
}

/* write avalue into the dac */
void mcp4921write(unsigned int value)
{
    unsigned char tmp;
    union uval{
        unsigned short int i;
	unsigned char buf[2];
    }val;

    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   // adresse A0=0
    digitalWrite (A1, dba&0x02) ;   // adresse A1=0
    delay(10); // attente stabilisation
    val.i = value+4096+8192+16384;
    tmp= val.buf[0];
    val.buf[0] = val.buf[1];
    val.buf[1] = tmp;
    digitalWrite (CS0, HIGH) ;
    wiringPiSPIDataRW(0, val.buf, 2);
    digitalWrite (CS0, LOW) ;
    sem_post(semaphore);
}


/* read mcp3201 using SPI */
unsigned int mcp3201read()
{
    unsigned char tmp;
    union uval{
        unsigned short int i;
	unsigned char buf[2];
    }val;

    unsigned int filtered;
    int i;

    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   // adresse A0=0
    digitalWrite (A1, dba&0x02) ;   // adresse A1=0
    delay(10); // attente stabilisation
    for(i=0, filtered=0 ;i<8; i++){
        digitalWrite (CS1, HIGH) ;
	wiringPiSPIDataRW(0, val.buf, 2);
	digitalWrite (CS1, LOW) ;
	tmp= val.buf[0];
	val.buf[0] = val.buf[1];
	val.buf[1] = tmp;
	val.i >>=1;
	val.i &= 0xfff;
	filtered += val.i;
	delay(5);
   }
   sem_post(semaphore);
   return filtered>>3;
}

/* détection du pic de fin de charge */
int deltapeak(int rawdata)
{
    static int derivative = 0, previous = 0, delta;

    delta = rawdata - previous;
    if(delta > 0){
	derivative++;
	if (derivative > 8) derivative = 8;
    }
    else if(delta  < 0) {
	derivative--;
	if (derivative  < -8)
	return 1;
    }
    previous = rawdata;
    return 0;
}

/* param manager */
void argManager(int argc, char **argv)
{
    int opt;
    /* Testing and getting parameters */
    /* Paramters are
    -a : daughter board adresse 
    -c : battery capacity min (mAh)
    -C : battery capacity max (mAh)
    -f : filename to record results
    -n : # cycles
    -g : filename configuration
    -r : threshold for recording
    -o : offset
    -h : help */
    //while(i>=1){
    while ((opt = getopt(argc, argv, "a:c:C:n:f:g:o:p:vm:M:h")) != -1) {	
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
	    printf("Syntax: chadeche <option>\n");
	    printf("\t-a daugther board adress (0-3, def = %d)\n", DAUGHTER_BOARD_ADRESS);
	    printf("\t-c battery capacity min (mAh, def = %d)\n", CMIN);
	    printf("\t-C battery capacity max (mAh, def = %d)\n", CMAX);
	    printf("\t-n Repeat factor (def = %d)\n", NCYCLE);
	    printf("\t-f results filename (def = %s)\n",RESULTS_FILENAME);
	    printf("\t-g config filename (def = %s)\n", CONFIG_FILENAME);
	    printf("\t-p record period (seconds, def = %d)\n", RECORDPERIOD);
	    printf("\t-v activate VERBOSE mode\n");
	    printf("\t-o offset (volts, def = %5.4f)\n", OFFSET);
	    printf("\t-m vmin (default is %5.4f)\n", VMIN);
	    printf("\t-M vmin (default is %5.4f)\n", VMAX);
	    printf("\t-h this help\n");
	    exit(1);
	default :
	    printf("Unknown option or bad syntax\n");
	    exit(1);
    }
    }
}

int main (int argc, char **argv)
{
    int i, j, repeat, step, dt, peakdetected=0;
    unsigned int initialData, currentData, milliamp, milliampScaled, mAh, stepmAh;
    double voltage;
    time_t t, totaltime, cycletime, elapsedtime, steptime, oldt;
    FILE *fp;
    char cop, decision;
    char *str, *endptr; // for call to function strtol
    int adress; // for Jump
    struct sigaction act;
    enum cause {TOOLOW_DETECT, TOOHIGH_DETECT, CONTROLZ_DETECT} decisioncause;


    /* initialisation par défaut et gestion des arguments */
    argManager(argc, argv);

    printf("Chadeche test parameters:\n");
    printf("\tDaugther board adress : %d\n", dba);
    printf("\tBattery capacity min: %d\n", cmin); 
    printf("\tBattery capacity max : %d\n", cmax); 
    printf("\tResults filename : %s\n", results_filename);
    printf("\tConfig filename : %s\n", config_filename);
    printf("\tRepeat factor: %d\n", ncycle); 
    printf("\tOffset : %5.4f\n", offset);
    printf("\tVmin threshold : %5.4f\n", vmin);
    printf("\tVmax threshold : %5.4f\n", vmax);
    printf("\tRecord period in seconds: %4d\n", recordPeriod);
    printf("\tMode :  ");
	 verbose_concise == CONCISE ? printf("CONCISE\n") : printf("VERBOSE\n");


    /* lecture et mémorisation fichier de config */
    readconf(config_filename);
    /* presentation de l'essai */
    i = elapsedtime = mAh = 0;
    printf("Battery test sequence will be as follow ...\n");
    printf("ADR\tCOP\tmA\tS\tL\tH\tC\tmAh\tSTD\tComment\n");
    while(tconfig[i].cop != 0){
	printf("%d\t%c\t%d\t%d",
	    tconfig[i].adr,					// numero ligne
	    tconfig[i].cop, 					// code opération
	    tconfig[i].milliamp,				//courant de charge ou décharge
	    tconfig[i].duration);				// durée de l'étape
	    adress = strtol(str = (tconfig[i].toolow),  &endptr, 10);
	    endptr == str ? printf("\t%c", str[0]) : printf("\t%d", adress);
	    adress = strtol(str = (tconfig[i].toohigh),  &endptr, 10);
	    endptr == str ? printf("\t%c", str[0]) : printf("\t%d", adress);
	    adress = strtol(str = (tconfig[i].controlflag),  &endptr, 10);
	    endptr == str ? printf("\t%c", str[0]) : printf("\t%d", adress);
	    printf("\t%d\t%d\t%s",
	    mAh +=						// mAh cumulés
		((tconfig[i].milliamp*tconfig[i].duration/3600)* (tconfig[i].cop == 'C' ? 1 : -1)),
	    elapsedtime += tconfig[i].duration,			// temps total depuis le début de l'essai
	    tconfig[i].comment);
	i++;
    }

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

    /* semaphore creation ans initialization */
    semaphore = sem_open("/semaphore", O_CREAT | O_RDWR | O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
	perror("/semaphore");
	semaphore = sem_open("/semaphore", O_RDWR);
	if (semaphore == SEM_FAILED) {
	    perror("/semaphore");
	    exit(EXIT_FAILURE);
	 }
	firstcall = 0;
     }
     /* Rpi initialization */
    initchadeche();

    /* end led (red) off */
    endledoff();

    /* settintgs and pre-tests */
    printf("Setting current to zero and waiting 5 seconds ...\n");
    mcp4921write(0);
    delay(5000);

    /* wait for battery presence before starting test */
    if((currentData=mcp3201read()) == 0 ){
	printf("Waiting for battery presence\n");
	do {
	    printf("."); fflush(stdout);
	    delay(1000);
	} while ((currentData = mcp3201read()) == 0);
	printf("\nBattery presence detected. Test will start in 5 seconds ...\n");
	delay(5000);
    }

    /* verify that Vcell don't exceed Vmax_open */
    //if((voltage = (double)currentData*5.1/4096 + offset) >= VMAX_OPEN)
    if((voltage = (double)currentData/1000 + offset) >= VMAX_OPEN)
	goto abort;

   /* creating  the file to record results */
    if((fp = fopen(results_filename,  "w+x"))==NULL){
	printf("Program aborted : can't open results file : %s\n",results_filename);
	sem_unlink("/semaphore");
	exit(1);
    }

    /* main measurement loop */
    time(&totaltime);
    /* cycle loop */
    oldt = 0, mAh=0;
    for (repeat =0; (repeat < ncycle) ; /*repeat++*/){
	printf("Starting cycle nr.:%2d\n", repeat+1);
    	/* initializing time in second */
    	time(&cycletime);
	step = 0; elapsedtime = 0, stepmAh=0;
	/* step loop */
	while( (tconfig[step].cop !=0) ){
	    mAh += stepmAh;
	    time(&steptime);
	    //printf("Starting step nr.: %2d: %c\tmA=%d\tDuration=%d\tmAh=%d\tE.T.=%d\t%s",
	    printf("Starting step nr.: %2d\n",
		step+1
		//cop = tconfig[step].cop,
	    	//tconfig[step].milliamp,
	    	//tconfig[step].duration,
	    	//tconfig[step].milliamp*tconfig[step].duration/3600,
	    	//elapsedtime += tconfig[step].duration,
	    	//tconfig[step].comment
		);
	    /* set the relay for the current mode */
	    //digitalWrite (REL, (cop=tconfig[step].cop) == 'D' ? DISCHARGE : CHARGE) ;
	    (cop=tconfig[step].cop) == 'D' ? discharge() : charge() ;
	    /* set the current to the desired value */
	    milliampScaled = tconfig[step].milliamp*10;
	    mcp4921write(milliampScaled);

	    j=0;
	    /* data acquisition and logging loop */
	    while( (time(&t)-steptime) <= tconfig[step].duration ){
		/* read battery voltage */
      		currentData = mcp3201read();
	        voltage = (double)currentData/1000 + offset;
	        //voltage = (double)currentData*0.9758*5.1/4096 + offset;
                //voltage = (double)currentData*5.1/4096 + offset;
		decision = 'I'; // Ignore
		if(stopflag == 1 || peakdetected)
		    goto abort;
		if(voltage <= vmin){
		    printf("Evenement : V < Vmin\n");
	    	    adress = strtol(str = (tconfig[step].toolow),  &endptr, 10);
		    decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
		    if(decision != 'I') goto mngdec;
		}
		/*else*/ if(voltage >= vmax){
		    printf("Evenement : V > Vmax\n");
	     	    adress = strtol(str = (tconfig[step].toohigh),  &endptr, 10);
		    decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	  	    if(decision != 'I') goto mngdec;
		}
		/*else*/ if(controlflag == 1){
		    printf("Evenement : Ctrl-Z\n");
		    controlflag = 0;
		    adress = strtol(str = (tconfig[step].controlflag),  &endptr, 10);
		    decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
		    decisioncause = CONTROLZ_DETECT;
	  	    /*if(decision != 'I') goto mngdec;*/
		}
mngdec:		switch(decision){
		case 'A' : goto abort;		// abort
		case 'S' : goto nextstep;	// next step
		case 'C' : goto nextcycle;	// next cycle
		case 'J' : step = adress-1;	// Jump 
		case 'I' : break;		// ignore (do nothin)
		default :  break;
		}
		/* verbose mode */
		//if((verbose_concise == VERBOSE) && !((t-cycletime) % 10) ){
		dt = verbose_concise == VERBOSE ? 10 : recordPeriod; 
		/* estimate of mAh in the step */
	   	stepmAh =((tconfig[step].milliamp * (t-steptime)/3600) * (tconfig[step].cop == 'C' ? 1 : -1));
		if(!((t-cycletime) % dt) ){
            	    //printf("%c/%02d/%02d mA=%4d E.T.=%6d T.T.=%6d Raw=%3d V=%5.3f %s", cop, repeat+1, step+1, tconfig[step].milliamp, t-cycletime, t-totaltime, currentData, voltage, ctime(&t));
            	    //fprintf(fp, "%c;%02d;%02d;%4d;%6d;%6d;%3d;%5.3f;%s", cop, repeat+1, step+1, tconfig[step].milliamp, t-cycletime, t-totaltime, currentData, voltage, ctime(&t));
            	    printf("%c/%02d/%02d dba=%1d mA=%4d CET=%6ld SET=%6ld TET=%6ld STA=%6d V=%5.3f cap=%d, %s", 
			/* code operation */cop,/* nr cycle */  repeat+1, /* nr step */step+1, /*daugher baord adress */dba, 
			/* consigne courant */tconfig[step].milliamp,
			/* CET */t-cycletime, /* SET */t-steptime , /* TET*/ t-totaltime,  /* STA */tconfig[step].duration-t+steptime,
			/* battery voltage */voltage, /* current charge level*/mAh+stepmAh, ctime(&t));
            	    //fprintf(fp, "%c;%02d;%02d;%4d;%6d;%6d;%3d;%5.3f;%s", cop, repeat+1, step+1, tconfig[step].milliamp, t-cycletime, t-totaltime, currentData, voltage, ctime(&t));
            	    fprintf(fp, "%c;%02d;%02d;%1d;%4d;%6ld;%6ld;%6ld;%6d;%5.3f;%s", 
			/* code operation */cop,/* nr cycle */  repeat+1, /* nr step */step+1, /*daugher baord adress */dba, 
			/* consigne courant */tconfig[step].milliamp,
			/* CET */t-cycletime, /* SET */t-steptime , /* TET*/ t-totaltime,  /* STA */tconfig[step].duration-t+steptime,
			/* battery voltage */voltage, ctime(&t));
	    	    fflush(fp);
        	}
	    	else if(verbose_concise == CONCISE){
		    printf("."); fflush(stdout);
                    if((j%50) == 0) printf("\n");
            	}
		/* wait for 1 sec before next battery voltage measurement */ 
	    	delay(1000);
	    	//time(&t);
		j++;
		oldt = t;
	    }
nextstep:   step++;
	}
nextcycle:  repeat++;
    }
abort:
    fclose(fp);

    /* testing why the test ends */
    if (voltage >= vmax)
    	printf ("\nBattery test ends because battery voltage = %5.3f is >= %4.2f Volts\n", voltage, vmax);
    else if (voltage >= VMAX_OPEN)
    	printf ("\nBattery test ends because battery voltage = %5.3f is >= %4.2f Volts\n", voltage, VMAX_OPEN);
    else if (voltage <= vmin)
    	printf ("\nBattery test ends because battery voltage = %5.3f is <= %4.2f Volts\n", voltage, vmin);
    else if (peakdetected)
    	printf ("\nBattery test ends because battery peak detected with voltage = %5.3f\n", voltage);
    else if (stopflag==1)
    	printf ("\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n", voltage);
    else if (decisioncause == CONTROLZ_DETECT)
    	printf ("\nBattery test ends due to Ctrl-Z occurence. Battery voltage is = %5.3f\n", voltage);
    else
	printf ("\nBattery test because test is complete (normal condition)\n");

    printf("Setting the discharge current to zero\n");
    mcp4921write(0);
    /* switching the end led (red) on */
    printf("Type Ctrl-C to quit definitively\n");
    stopflag = 0;
    while(stopflag == 0) {
	//digitalWrite (ENDLED, LOW) ;
	endledon();
	delay(1000);
        //digitalWrite (ENDLED, HIGH) ;
	endledoff();
	delay(1000);
    }
    //digitalWrite (ENDLED, LOW) ;
    endledon();
    discharge();
    sem_unlink("/semaphore");
    printf(language == FR ? "\nAu revoir !!!\n" : "\nBye !!!\n");
    return 0;
}
