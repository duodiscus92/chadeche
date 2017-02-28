/*
	 batterytest by J. Ehrlich
 */

#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define	MISO	7 /* BCM_GPIO 4 */
#define	CS	6 /* BCM_GPIO 25 */
#define CLK	5 /* BCM_GPIO 24 */
#define RELAIS  4 /* BCM_GPIO 23 */
#define ENDLED     3 /* BCM_GPIO 22 */
volatile int stopflag=0;
/* interrupt handler */
void CtrlCHandler(int sig)
{
    stopflag=1;
}

/* generate one clk cycle */
void toggleclk(void)
{
    digitalWrite (CLK, HIGH) ;
    delay(20);
    digitalWrite (CLK, LOW) ;
    delay(20);
}

/* read ADC832 using serial interface (slightly different of SPI) */
unsigned char adc831read()
{
    unsigned char rawdata;
    int i, j;
    digitalWrite (CS, HIGH) ;
    digitalWrite (CLK, LOW) ;
    /* select chip and initiate conversion */
    digitalWrite (CS, LOW) ;
    toggleclk();
    rawdata = 0;
    for(i=0; i<8; i++){
	toggleclk();
        rawdata = (rawdata<<1) | digitalRead(MISO);
    }

    /* deselect chip */
    digitalWrite (CS, HIGH) ;

    return rawdata;
}

/* default values */
#define OFFSET 0.030
#define FILENAME "batterytest.csv"
#define RECORDTHRESHOLD 1	/* used only when record mode is RECORD_BY_VOLTAGE */
#define TESTENDTHRESHOLD 1.1
#define CONCISE 0
#define VERBOSE 1
#define RECORDPERIOD 120 	/* used only when record mode is RECORD_BY_TIME */
#define RECORD_BY_TIME 0
#define RECORD_BY_VOLTAGE 1
/* parameters */
int recordThreshold = RECORDTHRESHOLD, verbose_concise = CONCISE, recordPeriod = RECORDPERIOD, recordMode = RECORD_BY_VOLTAGE;
double offset = OFFSET, testEndThreshold = TESTENDTHRESHOLD;
char filename[80];
/* param manager */
void argManager(int argc, char **argv)
{
    int i;
    /* Testing and getting parameters */
    /* Paramters are 
    -f : filename to record results
    -e : end test threshold (such as not to destroy battery)
    -r : threshold for recording
    -o : offset	 
    -h : help */
    //while(i>=1){
    for(i=argc-1; i>=1; i--){
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
	/* is it testEndThreshold */
	if(!strncmp("-e", argv[i], 2)){
	    testEndThreshold = atof(argv[i]+2);
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
	    printf("\t-m record mode : m1 record by voltage, m0 record by time\n");
	    printf("\t-f filename\n");
	    printf("\t-e test end threshold (volts)\n");
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
    unsigned char initialData, currentData;
    double voltage;
    time_t t, starttime, delta_t, dt;
    FILE *fp;


    /* initialisation par défaut et gestion des arguments */
    strcpy(filename, FILENAME);
    argManager(argc, argv);

    printf("Battery test parameters:\n");
    printf("\tRecord mode : ");
	recordMode == RECORD_BY_VOLTAGE ? printf("Record by voltage\n") : printf("Record by time\n");
    printf("\tFilename : %s\n", filename);
    printf("\tOffset : %5.4f\n", offset);
    printf("\tEnd test threshold : %5.3f\n", testEndThreshold);
    printf("\tRecord threshold in digits (ingored in record by time mode or verbose mode): %4d\n", recordThreshold);
    printf("\tRecord period in seconds (ignored in record by voltage mode or verbose mode) : %4d\n", recordPeriod);
    printf("\tMode :  ");
	 verbose_concise == CONCISE ? printf("CONCISE\n") : printf("VERBOSE\n");

    wiringPiSetup ();
    pinMode (MISO, INPUT);
    pinMode (CLK, OUTPUT);
    pinMode (CS, OUTPUT);
    pinMode (RELAIS, OUTPUT);
    pinMode (ENDLED, OUTPUT);

    /* creating  the file */
    if((fp = fopen(filename,  "w+x"))==NULL){
	printf("Can't open file : %s\n", filename);
	exit(1);
    }

    printf("Connecting the battery ... Wait a while ...\n");
    digitalWrite (RELAIS, HIGH) ;
    digitalWrite (ENDLED, HIGH) ;
    delay (5000);

    /* installing the Ctrl-C handler */
    signal(SIGINT, CtrlCHandler);

    /* initializing time in second */
    time(&starttime);

    /* main measurement loop */
    initialData = 255;
    /* header */
    fprintf(fp, "N°;Secondes;Brut;Volts;Date\n");
    fflush(fp);
    for(i=0, j=0, delta_t=0; stopflag < 1; i++, j++){
	currentData = adc831read();
        voltage = (double)currentData*5.0/256;
	voltage += offset;
	time(&t);
	if(verbose_concise == VERBOSE){ /* mode bavard */
            printf("i=%7d elaps=%6d data=%3d volts=%5.3f %s", i, t-starttime, currentData, voltage, ctime(&t));
            fprintf(fp, "%7d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
	    fflush(fp);
	    delay(10000);
        }
	else { /* mode discret */
	    if(( (recordMode == RECORD_BY_TIME) && ( !(( (dt = t-starttime) ) % recordPeriod) && (dt != delta_t) ) ) ||
              ((recordMode == RECORD_BY_VOLTAGE) && ( (initialData - currentData) >= recordThreshold) ) ){
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
	    delay(500);
	}
	if(voltage < testEndThreshold)
	    break;
    }

    fprintf(fp, "%7d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
    fclose(fp);

    /* testing why the test ends */
    if (stopflag == 0)
    	printf ("\nBattery test ends because battery voltage = %5.3f <= threshold = %5.3f\n", voltage, testEndThreshold);
    else
    	printf ("\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n", voltage);

    printf("Disconnecting the battery\n");
    digitalWrite (RELAIS, LOW) ;
    digitalWrite (ENDLED, LOW) ;
    printf("Bye\n");

    return 0;
}
