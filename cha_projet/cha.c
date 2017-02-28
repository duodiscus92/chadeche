/*
	 cha by J. Ehrlich
 */

#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>


#define A0	0	//GPIO 17
#define A1	2	//GPIO 2 
#define	CS0	4	//GPIO 23 CS0
#define	CS1	5	//GPIO 24 CS1
#define	REL	7	//GPIO 4 commande relais
#define ENDLED  6	//GPIO 6 End of test (red led)


int min(int a, int b)
{
	return (a < b ? a : b);
}

volatile int stopflag=0, stepnumber=1;
/* Ctrl-C interrupt handler */
void CtrlCHandler(int sig)
{
    stopflag=1;
    //stepnumber++;
}

/* Ctrl-Z interrupt handler */
void CtrlZHandler(int sig)
{
    //stopflag=1;
    stepnumber++;
}

/* cha initialisation */
void initcha(void)
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
    digitalWrite (REL, HIGH) ;  // charge mode

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
#define OFFSET 0.030
#define FILENAME "batterytest.csv"
#define RECORDTHRESHOLD 1	/* used only when record mode is RECORD_BY_VOLTAGE */
#define CONCISE 0
#define VERBOSE 1
#define RECORDPERIOD 120 	/* used only when record mode is RECORD_BY_TIME */
#define RECORD_BY_TIME 0
#define RECORD_BY_VOLTAGE 1
#define CURRENT_COEF_STEP1 0.1
#define CURRENT_COEF_STEP2 0.5
#define CURRENT_COEF_STEP3 0.05
#define STEP1_DURATION 3600
#define STEP2_DURATION ((int)(3600.0*1.0/(CURRENT_COEF_STEP2*0.8)))
#define TOTAL_DURATION (10*3600)
/* parameters */
int recordThreshold = RECORDTHRESHOLD, verbose_concise = CONCISE, recordPeriod = RECORDPERIOD, recordMode = RECORD_BY_VOLTAGE, capacity = CAPACITY;
double offset = OFFSET;
char filename[80];
/* param manager */
void argManager(int argc, char **argv)
{
    int i;
    /* Testing and getting parameters */
    /* Paramters are
    -c : battery capacity (mAh)
    -f : filename to record results
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
	/* is it record mode ?*/
	if(!strncmp("-m", argv[i], 2)){
	    recordMode = atoi(argv[i]+2);
	    continue;
	} else
	/* is it filename ?*/
	if(!strncmp("-f", argv[i], 2)){
	    strcpy(filename, argv[i]+2);
	    continue;
	} else
	/* is it offset ? */
	if(!strncmp("-o", argv[i], 2)){
	    offset = atof(argv[i]+2);
	    continue;
	} else
	/* is it recordThreshold */
	if(!strncmp("-r", argv[i], 2)){
	    recordThreshold = atoi(argv[i]+2);
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
	    printf("Syntax: batterytest <option>\n");
	    printf("\t-c battery capacity (mAh)\n");
	    printf("\t-m record mode : m1 record by voltage, m0 record by time\n");
	    printf("\t-f filename\n");
	    printf("\t-r record threshold (bits within 1 to 256)\n");
	    printf("\t-p record period (seconds)\n");
	    printf("\t-v activate VERBOSE mode\n");
	    printf("\t-o offset (volts)\n");
	    printf("\t-h this help\n");
	    /*printf("Attention : pas d'espace entre l'indicateur d'option et la valeur\n\n");*/
	    printf("Warning : no space between option flag and value (e.g. m0 is good but m 0 is bad)\n\n");
	    exit(1);
	} else{
	    printf("Unknown option or bad syntax\n");
	    exit(1);
          }
    }
}


int main (int argc, char **argv)
{
    long i, j;
    unsigned int initialData, currentData, milliamp, milliampScaled;
    unsigned int peakdetected = 0, step1flag = 0, step2flag = 0, step3flag = 0;
    double voltage;
    time_t t, starttime, delta_t, dt, charge_duration;
    FILE *fp;


    /* initialisation par défaut et gestion des arguments */
    strcpy(filename, FILENAME);
    argManager(argc, argv);

    printf("Battery test parameters:\n");
    printf("\tBattery capacity : %d\n", capacity); 
    printf("\tMax charge current during precharge charge step will be : \t%d mA\n", (int)(capacity*CURRENT_COEF_STEP1)); 
    printf("\tMax charge current during rapid charge step will be : \t%d mA\n", (int)(capacity*CURRENT_COEF_STEP2)); 
    printf("\tMax charge current during permanent charge  step will be : \t%d mA\n", (int)(capacity*CURRENT_COEF_STEP3)); 
    if(capacity/5 > 400){
	printf("Illegal current value during rapid charge test (must not exceeed 400 mA\n");
	printf("Current is set to 400 mA\n");
	//printf("---> You must declare a lower capacity for your battery\n");
	//exit(1);
    }
    printf("\tRecord mode : ");
	recordMode == RECORD_BY_VOLTAGE ? printf("Record by voltage\n") : printf("Record by time\n");
    printf("\tFilename : %s\n", filename);
    printf("\tOffset : %5.4f\n", offset);
    printf("\tRecord threshold in digits (ingored in record by time mode or verbose mode): %4d\n", recordThreshold);
    printf("\tRecord period in seconds (ignored in record by voltage mode or verbose mode) : %4d\n", recordPeriod);
    printf("\tMode :  ");
	 verbose_concise == CONCISE ? printf("CONCISE\n") : printf("VERBOSE\n");

    /* creating  the file */
    if((fp = fopen(filename,  "w+x"))==NULL){
	printf("Can't open file : %s\n", filename);
	exit(1);
    }

    /* Rpi initialization */
    initcha();

    /* end led (red) off */
    digitalWrite (ENDLED, HIGH) ;
    delay (5000);

    /* installing the Ctrl-C handler */
    signal(SIGINT, CtrlCHandler);
    signal(SIGTSTP, CtrlZHandler);

    /* initializing time in second */
    time(&starttime);

    /* main measurement loop */
    initialData = 4095;
    /* header */
    fprintf(fp, "N°;Secondes;Brut;Volts;Date\n");
    fflush(fp);
    printf("Setting current to zero and waiting 5 seconds ...\n");
    mcp4921write(0);
    delay(5000);
    for(i=0, j=0, delta_t=0; stopflag < 1; i++, j++){
    //for(i=0, j=0, delta_t=0; ; i++, j++){
	currentData = mcp3201read();
        voltage = (double)currentData*5.1/4096;
	voltage += offset;
	time(&t);
	//if (/*voltage >=0.8 &&*/ voltage <=1.0){
	if (/*voltage >=0.8 &&*/ voltage <=1.4){
	    if(step1flag == 0){
		milliamp = min((int)(capacity*CURRENT_COEF_STEP1), 400);
	    	milliampScaled = milliamp*8.0194+2.4817;
	        mcp4921write(milliampScaled);
		step1flag = 1 ;
		printf("Starting precharge with current : %d mA\n", milliamp);
	    }
	}
        ///*else*/if (voltage > 1.0 && voltage <= 1.5  && ((t-starttime) < STEP2_DURATION && !peakdetected) ) {
        else if (voltage > 1.4 && voltage <= 1.6  && ((t-starttime) < STEP2_DURATION) && !peakdetected ) {
	    if(step2flag == 0){
		milliamp = min((int)(capacity*CURRENT_COEF_STEP2),400);
	    	milliampScaled = milliamp*8.0194+2.4817;
	        mcp4921write(milliampScaled);
		step2flag = 1 ;
		printf("Starting rapidcharge with current : %d mA until elapsed time  %d or deltapeak detected\n", milliamp, STEP2_DURATION);
	    }
	    peakdetected = deltapeak(currentData);
	}
	else if ((t-starttime) < TOTAL_DURATION ){
	    if(step3flag == 0){
		milliamp = min((int)(capacity*CURRENT_COEF_STEP3), 400);
	    	milliampScaled = milliamp*8.0194+2.4817;
	        mcp4921write(milliampScaled);
		step3flag = 1 ;
		printf("Starting permanent charge with current : %d mA until elapsed time  %d\n", milliamp, TOTAL_DURATION);
	    }
	}
	else {
	    printf("Charge complete\n");
	    break;
        }

	if(verbose_concise == VERBOSE){ /* mode bavard */
            printf("i=%7d elaps=%6d data=%3d volts=%5.3f %s", i, t-starttime, currentData, voltage, ctime(&t));
            fprintf(fp, "%7d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
	    fflush(fp);
	    delay(10000);
        }
	else { /* mode discret */
	    if(( (recordMode == RECORD_BY_TIME) && ( !(( (dt = t-starttime) ) % recordPeriod) && (dt != delta_t) ) ) ||
              ((recordMode == RECORD_BY_VOLTAGE) && ((initialData - currentData) >= recordThreshold) ) ){
		delta_t = dt;
		initialData = currentData;
                printf("\ni=%7d elaps=%6d data=%3d volts=%5.3f %s", i, t-starttime, currentData, voltage, ctime(&t));
                fprintf(fp, "%7d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
		fflush(fp);
		j=0;
            }
	    else {
		printf("."); fflush(stdout);
                if((j%50) == 0) printf("\n");
            }
	    delay(1000);
	}
    }

    fprintf(fp, "%7d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
    fclose(fp);

    /* testing why the test ends */
    if (stopflag == 0)
    //if (stepnumber == 4)
    	printf ("\nBattery test ends because battery voltage = %5.3f and charge complete (peak detected)\n", voltage);
    else
    	printf ("\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n", voltage);

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
    printf("\nBye !!!\n");

    return 0;
}
