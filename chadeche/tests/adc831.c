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
    int i;

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


#define OFFSET 0.030
int main (int argc, char **argv)
{
    int i, j, recordThreshold;
    unsigned char initialData, currentData;
    char filename[80];
    double voltage, seuil;
    time_t t, starttime;
    FILE *fp;

    /*printf ("ADC831 starting : MISO:%2d CLK:%2d CS:%2d\n", MISO, CLK, CS);*/

    wiringPiSetup ();
    pinMode (MISO, INPUT);
    pinMode (CLK, OUTPUT);
    pinMode (CS, OUTPUT);
    pinMode (RELAIS, OUTPUT);

    /* Testing and getting parameters */
    /* Paramters are 
    - argv[1] : filename to record results
    - argv[2] : end test threshold (such as not to destroy battery)
    - argv[3] : threshold for recording */
    if(argc < 3){
	printf("Mauvais nombre d'argument : fournir 2 ou 3 arguments : \n\t \
		Nom du fichier résultat (obligatoire)\n\t \
		Seuil d'arrêt du test(obligatoire\n\t \
		Seuil d'enregistrement(facultatif)\n");
	exit(1);
    }
    strcpy(filename, argv[1]);
    seuil = atof(argv[2]);
    if(argc == 4)
	recordThreshold = atoi(argv[3]);

    printf ("Battery test parameters:\n\tFilename : %s\n", filename);
    printf("\tEnd test threshold : %5.3f\n", seuil);
    if(argc == 4)
	printf("\tRecord threshold : %4d\n", recordThreshold);

    printf("Connecting the battery ... Wait a while ...\n");
    digitalWrite (RELAIS, HIGH) ;
    delay (5000);

    /* installing the Ctrl-C handler */
    signal(SIGINT, CtrlCHandler);

    /* creating  the file */
    if((fp = fopen(filename,  "w+"))==NULL){
	printf("Can't open file : %s\n", filename);
	exit(1);
    }

    printf("Starting battery test in ");
    if (argc>3) 
	printf("laconic mode\n");
    else
	printf("loquacious mode\n");

    /* initializing time in second */
    time(&starttime);

    /* main measurement loop */
    initialData = 255;
    /* header */
    fprintf(fp, "N°;Secondes;Brut;Volts;Date\n");
    for(i=0,j=0; stopflag < 1; i++, j++){
	currentData = adc831read();
        voltage = (double)currentData*5.0/256;
	voltage += OFFSET;
	time(&t);
	if(argc == 3){
            printf("i=%6d elaps=%6d data=%3d volts=%5.3f %s", i, t-starttime, currentData, voltage, ctime(&t));
            fprintf(fp, "%6d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
        }
	else {
            if((initialData - currentData) >= recordThreshold){
		initialData = currentData;
                printf("\ni=%6d elaps=%6d data=%3d volts=%5.3f %s", i, t-starttime, currentData, voltage, ctime(&t));
                fprintf(fp, "%6d;%6d;%3d;%5.3f;%s", i, t-starttime, currentData, voltage, ctime(&t));
		j=0;
            }
	    else 
		printf("."); fflush(stdout);
            if((j%50) == 0) printf("\n");
	}
        delay(10000);
	if(voltage < seuil)
	    break;
    }

    fclose(fp);

    /* testing why the test ends */
    if (stopflag == 0)
    	printf ("\nBattery test ends because battery voltage = %5.3f <= threshold = %5.3f\n", voltage, seuil);
    else
    	printf ("\nBattery test ends due to Ctrl-C occurence. Battery voltage is = %5.3f\n", voltage);

    printf("Disconnecting the battery in 5 seconds\n");
    delay(5000);
    digitalWrite (RELAIS, LOW) ;

    printf("Bye\n");

    return 0;
}
