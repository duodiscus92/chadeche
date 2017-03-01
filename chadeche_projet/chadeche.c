/*
	 chadeche by J. Ehrlich
 */

#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
//#include <setjmp.h>


#define A0	0	//GPIO 17
#define A1	2	//GPIO 2 
#define	CS0	4	//GPIO 23 CS0
#define	CS1	5	//GPIO 24 CS1
#define	REL	7	//GPIO 4 commande relais
#define ENDLED  6	//GPIO 6 End of test (red led)

#define CHARGE HIGH
#define DISCHARGE LOW

typedef enum lang{FR, EN} LANGUAGE;
LANGUAGE language = FR;

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
void readconf(char *filename)
{
	FILE *fp;
	int i = 0;
	char buffer[80];

	if ((fp = fopen(filename, "r")) == NULL){
	    printf("Impossible ouvrir fichier config %s\n", filename);
	    exit(1);
	}

	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    //sscanf(buffer, "%c%d%d%*c%c%*c%c%*c%c\n",
	    //sscanf(buffer, "%c%d%d%c%c%c\n",
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


/* chadeche initialisation */
void initchadeche(void)
{
    wiringPiSetup () ;
    pinMode (A0, OUTPUT) ;
    pinMode (A1, OUTPUT) ;
    pinMode (CS0, OUTPUT) ;
    pinMode (CS1, OUTPUT) ;
    pinMode (REL, OUTPUT) ;
    pinMode (ENDLED, OUTPUT) ;

    digitalWrite (A0, LOW) ;   // adresse A0=0
    digitalWrite (A1, LOW) ;   // adresse A1=0
    digitalWrite (CS0, HIGH) ; // deselect DAC
    digitalWrite (CS1, HIGH) ; // deselect CAN
    digitalWrite (REL, DISCHARGE) ;  // discharge mode

    wiringPiSPISetup(0, 100000); // init SPI interface
}

/* write avalue into the dac */
void mcp4921write(unsigned int value)
{
    unsigned char tmp;
    union uval{
        unsigned short int i;
	unsigned char buf[2];
    }val;

    val.i = value+4096+8192+16384;
    tmp= val.buf[0];
    val.buf[0] = val.buf[1];
    val.buf[1] = tmp;
    digitalWrite (CS0, LOW) ;
    wiringPiSPIDataRW(0, val.buf, 2);
    digitalWrite (CS0, HIGH) ;
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
    for(i=0, filtered=0 ;i<8; i++){
        digitalWrite (CS1, LOW) ;
	wiringPiSPIDataRW(0, val.buf, 2);
	digitalWrite (CS1, HIGH) ;
	tmp= val.buf[0];
	val.buf[0] = val.buf[1];
	val.buf[1] = tmp;
	val.i >>=1;
	val.i &= 0xfff;
	filtered += val.i;
	delay(5);
   }
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

/* default values */
#define CAPACITY 1700 /* mAh */
#define VMAX_OPEN 1.65 /* Vcell max when I = 0 */
#define VMAX 1.75 /* Vcell max during charge */
#define VMIN 1.0 /* Vcell min */
#define OFFSET 0.006
#define RESULTS_FILENAME "chadeche.csv"
#define CONFIG_FILENAME "chadeche.txt"
//#define RECORDTHRESHOLD 1	/* used only when record mode is RECORD_BY_VOLTAGE */
#define CONCISE 0
#define VERBOSE 1
#define RECORDPERIOD 120 	/* used only when record mode is RECORD_BY_TIME */
#define NCYCLE 1

/* parameters */
int verbose_concise = CONCISE, recordPeriod = RECORDPERIOD,  capacity = CAPACITY, ncycle = NCYCLE, langage = FR;
/* other global */
double offset = OFFSET; /* to offset CAN measurement errors */
char results_filename[80], config_filename[80];
char **msg; /* multilingual message */
/* param manager */
void argManager(int argc, char **argv)
{
    int i;
    /* Testing and getting parameters */
    /* Paramters are
    -c : battery capacity (mAh)
    -f : filename to record results
    -n : # cycles
    -g : filename configuration
    -r : threshold for recording
    -o : offset
    -h : help */
    //while(i>=1){
    for(i=argc-1; i>=1; i--){
	/* is it battery capacity ?*/
	if(!strncmp("-c", argv[i], 2)){
	    capacity = atoi(argv[i]+2);
	    continue;
	} else
	/* is it number of cycle ?*/
	if(!strncmp("-n", argv[i], 2)){
	    ncycle = atoi(argv[i]+2);
	    continue;
	} else
	/* is it filename ?*/
	if(!strncmp("-f", argv[i], 2)){
	    strcpy(results_filename, argv[i]+2);
	    continue;
	} else
	/* is it config filename ?*/
	if(!strncmp("-g", argv[i], 2)){
	    strcpy(config_filename, argv[i]+2);
	    continue;
	} else
	/* is it offset ? */
	if(!strncmp("-o", argv[i], 2)){
	    offset = atof(argv[i]+2);
	    continue;
	} else
	/* is it recordPeriod */
	if(!strncmp("-p", argv[i], 2)){
	    recordPeriod = atoi(argv[i]+2);
	    continue;
	} else
	/* is it verbose_concise */
	if(!strncmp("-v", argv[i], 2)){
	    verbose_concise = VERBOSE;
	    continue;
	} else
	/* is it help */
	if(!strncmp("-h", argv[i], 2)){
	    printf("Syntax: chadeche <option>\n");
	    printf("\t-c battery capacity (mAh)\n");
	    printf("\t-n Repeat factor\n");
	    printf("\t-f results filename\n");
	    printf("\t-g config filename\n");
	    printf("\t-p record period (seconds)\n");
	    printf("\t-v activate VERBOSE mode\n");
	    printf("\t-o offset (volts)\n");
	    printf("\t-h this help\n");
	    printf("Warning : no space between option flag and value (e.g. m0 is good but m 0 is bad)\n\n");
	    exit(1);
	} else{
	    printf("Unknown option or bad syntax\n");
	    exit(1);
          }
    }
}

/* pour point de reprise en cas d'abandon acquisition */
int main (int argc, char **argv)
{
    int i, j, repeat, step, dt, peakdetected=0;
    unsigned int initialData, currentData, milliamp, milliampScaled, mAh;
    double voltage;
    time_t t, totaltime, cycletime, elapsedtime, steptime;
    FILE *fp;
    char cop, decision;
    char *str, *endptr; // for call to function strtol
    int adress; // for Jump
    struct sigaction act;


    /* initialisation par défaut et gestion des arguments */
    strcpy(results_filename, RESULTS_FILENAME);
    strcpy(config_filename, CONFIG_FILENAME);
    argManager(argc, argv);

    printf("Chadeche test parameters:\n");
    printf("\tBattery capacity : %d\n", capacity); 
    printf("\tResults filename : %s\n", results_filename);
    printf("\tConfig filename : %s\n", config_filename);
    printf("\tRepeat factor: %d\n", ncycle); 
    printf("\tOffset : %5.4f\n", offset);
    printf("\tRecord period in seconds: %4d\n", recordPeriod);
    printf("\tMode :  ");
	 verbose_concise == CONCISE ? printf("CONCISE\n") : printf("VERBOSE\n");


    /* lecture et mémorisation fichier de config */
    readconf(config_filename);
    /* presentation de l'essai */
    i = elapsedtime = mAh = 0;
    printf("Battery test sequence will be as follow ...\n");
    printf("ADR\tCOP\tmA\tS\tL\tH\tC\tmAh\tE.T.\tComment\n");
    while(tconfig[i].cop != 0){
	printf("%d\t%c\t%d\t%d",
	    tconfig[i].adr,					// numero ligne
	    tconfig[i].cop, 					// code opération
	    tconfig[i].milliamp,				//courant de charge ou décharge
	    tconfig[i].duration);				// durée de l'étape
	    //tconfig[i].toolow,					// action si tension trop basse
	    //tconfig[i].toohigh,					// action si tension trop haute
	    //tconfig[i].controlflag,				// action si touches Contr-X pressées
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

   /* creating  the file to record results */
    if((fp = fopen(results_filename,  "w+x"))==NULL){
	printf("Can't open file : %s\n",results_filename);
	exit(1);
    }

    /* Rpi initialization */
    initchadeche();

    /* end led (red) off */
    digitalWrite (ENDLED, HIGH) ;

    /* installing the Ctrl-C handler */
    signal(SIGINT, CtrlCHandler);
    /* installing the Ctrl-Z handler */
    act.sa_sigaction = &CtrlZHandler;
    act.sa_flags = SA_SIGINFO;
    //signal(SIGTSTP, CtrlZHandler);
    if(sigaction(SIGTSTP, &act, NULL) <0){
	perror("sigaction");
	return 1;
    }
    /* settintgs and pre-tests */
    fprintf(fp, "COP;Cycle;Step;mA;ET;TT;Brut;Volts;Date\n");
    fflush(fp);
    printf("Setting current to zero and waiting 5 seconds ...\n");
    mcp4921write(0);
    delay(5000);
    /* wait for battery presence before starting test 
    if((currentData=mcp3201read()) == 0 ){
	printf("Waiting for battery presence\n");
	do {
	    printf("."); fflush(stdout);
	    delay(1000);
	} while ((currentData = mcp3201read()) == 0);
	printf("\nBattery presence detected. Test will start in 5 seconds ...\n");
	delay(5000);
    }
    */
    /* verify that Vcell don't exceed Vmax_open */
    if((voltage = (double)currentData*5.1/4096 + offset) >= VMAX_OPEN)
	goto abort;

    /* main measurement loop */
    time(&totaltime);
    for (repeat =0; (repeat < ncycle) ; /*repeat++*/){
	printf("Starting cycle nr.:%2d\n", repeat+1);
    	/* initializing time in second */
    	time(&cycletime);
	step = 0; elapsedtime = 0;
	//time(&t);
	while( (tconfig[step].cop !=0) ){
	    time(&steptime);
	    printf("Starting step %d: %c\tmA=%d\tDuration=%d\tmAh=%d\tE.T.=%d\t%s",
		step+1,
		cop = tconfig[step].cop,
	    	tconfig[step].milliamp,
	    	tconfig[step].duration,
	    	tconfig[step].milliamp*tconfig[step].duration/3600,
	    	elapsedtime += tconfig[step].duration,
	    	tconfig[step].comment);
	    /* set the relay for the current mode */
	    digitalWrite (REL, tconfig[step].cop == 'D' ? DISCHARGE : CHARGE) ;
	    /* set the current to the desired value */
	    //milliampScaled = tconfig[step].milliamp*8.0194+2.4817;
	    //milliampScaled = tconfig[step].milliamp*8.113+1.3484;
	    milliampScaled = tconfig[step].milliamp*8.0327+1.3484;
	    mcp4921write(milliampScaled);

	    j=0;
	    /* data acquisition and logging loop */
	    //while( (time(&t)-cycletime) <= elapsedtime ){
	    while( (time(&t)-steptime) <= tconfig[step].duration ){
		/* read battery voltage */
      		currentData = mcp3201read();
                voltage = (double)currentData*5.1/4096 + offset;
		decision = 'I'; // Ignore
		if(stopflag == 1 || peakdetected)
		    goto abort;
		else if(voltage <= VMIN){
	    	    adress = strtol(str = (tconfig[step].toolow),  &endptr, 10);
		    decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    //endptr == str ? decision = str[0]) : decision = 'J' /* jump */;
		    //decision = tconfig[step].toolow;
		}
		else if(voltage >= VMAX){
	     	    adress = strtol(str = (tconfig[step].toohigh),  &endptr, 10);
		    decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    //endptr == str ? printf("\t%c", str[0]) : printf(\t%d", val);
		    //decision = tconfig[step].toohigh;
		}
		else if(controlflag == 1){
		    controlflag = 0;
		    adress = strtol(str = (tconfig[step].controlflag),  &endptr, 10);
		    decision =  ((endptr == str) ? str[0] : 'J'); /* J = Jump */
	    //endptr == str ? printf("\t%c", str[0]) : printf(\t%d", val);
		    //decision = tconfig[step].controlflag;
		}
		switch(decision){
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
		if(!((t-cycletime) % dt) ){
            	    printf("%c/%02d/%02d mA=%4d E.T.=%6d T.T.=%6d Raw=%3d V=%5.3f %s", cop, repeat+1, step+1, tconfig[step].milliamp, t-cycletime, t-totaltime, currentData, voltage, ctime(&t));
            	    fprintf(fp, "%c;%02d;%02d;%4d;%6d;%6d;%3d;%5.3f;%s", cop, repeat+1, step+1, tconfig[step].milliamp, t-cycletime, t-totaltime, currentData, voltage, ctime(&t));
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
	    }
nextstep:   step++;
	}
nextcycle:  repeat++;
    }
abort:
    fclose(fp);

    /* testing why the test ends */
    if (voltage >= VMAX)
    	printf ("\nBattery test ends because battery voltage = %5.3f is >= %4.2f Volts\n", voltage, VMAX);
    else if (voltage >= VMAX_OPEN)
    	printf ("\nBattery test ends because battery voltage = %5.3f is >= %4.2f Volts\n", voltage, VMAX_OPEN);
    else if (voltage <= VMIN)
    	printf ("\nBattery test ends because battery voltage = %5.3f is <= %4.2f Volts\n", voltage, VMIN);
    else if (peakdetected)
    	printf ("\nBattery test ends because battery peak detected with voltage = %5.3f\n", voltage);
    else if (stopflag==1)
    	printf ("\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n", voltage);
    else
	printf ("\nBattery test because test is complete (normal condition)\n");

    printf("Setting the discharge current to zero\n");
    mcp4921write(0);
    /* switching the end led (red) on */
    printf("Type Ctrl-C to quit definitively\n");
    stopflag = 0;
    while(stopflag == 0) {
	digitalWrite (ENDLED, LOW) ;
	delay(1000);
        digitalWrite (ENDLED, HIGH) ;
	delay(1000);
    }
    digitalWrite (ENDLED, LOW) ;
    printf(language == FR ? "\nAu revoir !!!\n" : "\nBye !!!\n");
    //msg[FR] = "\nAu revoir !!!\n";
    //msg[EN] = "\nBye !!!\n";
    //printf(msg[FR]);

    return 0;
}
