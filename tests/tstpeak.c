/*
	 tstpeak by J. Ehrlich
 */

#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>

/* détection du pic de fin de charge */
int deltapeak(int rawdata)
{
    static int derivative = 0, previous = 0, delta;

    delay(1000);
    printf("derivative = %d\n", derivative);
    delta = rawdata - previous;
    if(delta > 0){
	derivative++;
	if (derivative > 4) derivative = 4;
    }
    else if(delta  < 0) {
	derivative--;
	if (derivative  < -4)
	return 1;
    }
    previous = rawdata;
    return 0;
}

void main(void)
{
    FILE *fp;
    unsigned int i;

    /* creating  the file */
    if((fp = fopen("peakfile.txt",  "r"))== NULL){
	printf("Can't open file peakfile.txt");
	exit(1);
    }

    while(!feof(fp)) {
	fscanf(fp, "%d", &i);
	printf("%d\n", i);
	if (deltapeak(i)){
	   printf("Pic detecté\n");
	   break;
        }
    }
}

