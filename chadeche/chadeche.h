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
#include <sys/shm.h>
//#include <sys/ipc.h>
#include <sys/mman.h>


#define NB_BOARD 4	//max number of daugther board

#define A0	0	//GPIO 17
#define A1	2	//GPIO 27 
#define	CS0	9	//GPIO 3 CS0
#define	CS1	22	//GPIO 6 CS1
#define	CS2	21	//GPIO 5 CS2
#define	REL	7	//GPIO 4 commande relais

#define CHARGE HIGH
#define DISCHARGE LOW
//#define MEM_KEY	3245617	// chared memory key for shmget()
#define MEM_NAME "/sharedmem"

/* default values */
#define CINIT 0 	/* mAh */
#define CMIN 100 	/* mAh */
#define CMAX 1300 	/* mAh */
#define VMAX_OPEN 1.65 /* Vcell max when I = 0 */
#define VMAX 1.75 	/* Vcell max during charge */
#define VMIN 1.0 	/* Vcell min */
#define OFFSET 0	/* Voltage and current offset */
#define SLOPE 1		/* voltage and current slope */
#define RESULTS_FILENAME "chadeche-results.csv"
#define CONFIG_FILENAME "chadeche-config.csv"
#define PRM_FILENAME "chadecheX.prm" /* X will be replaced by daugther board adress 0, 1, 2, 3 */
//#define RECORDTHRESHOLD 1	/* used only when record mode is RECORD_BY_VOLTAGE */
#define CONCISE 0
#define VERBOSE 1
#define RECORDPERIOD 120 	/* used only when record mode is RECORD_BY_TIME */
#define NCYCLE 1
#define DAUGHTER_BOARD_ADRESS 0
#define MAX_STEP 500 		/* Max number of step in a configfile */

typedef enum lang{FR=0, EN=1} LANGUAGE;

typedef struct config {
	int adr;
	char cop;
	int  milliamp;
	int duration;
	char toolow[6];
	char toohigh[6];
	char toolowcapacity[6];
	char toohighcapacity[6];
	char always[6];
	char controlflag[6];
	char comment[80];
} CONFIG;


extern LANGUAGE language;
extern
int	verbose_concise,
	recordPeriod,
	cmin,
	cmax,
	cinit,
	ncycle,
	dba,
	langage,
	test;
extern
double 	offset, slope ,		// pente et offset tension
        ioffset, islope, 	// pente et offset courant
	vmax,
	vmin;
extern
char 	results_filename[80],
	config_filename[80];

extern CONFIG tconfig[MAX_STEP];
extern sem_t *semaphore;
//extern int firstcall;

/* interface de param.c */
extern int prmManager(void);
extern void argManager(int argc, char **argv);
extern void displayarg(void);

/* interface de config.c */
extern void readconf(char *filename);
extern void displayconf(void);

/* interface de hw.c */
extern void initchadeche(void);
extern int charge(void);
extern int discharge(void);
extern void mcp4922write(unsigned int value, unsigned int channel);
extern unsigned int mcp3201read();
extern void begled(void);
extern void ongoingled(void);
extern void endled(void);
extern int deltapeak(int rawdata);

/* interface de sem.c */
extern int initsem(void);
extern void termsem(void);
extern int initmem(void);
extern int termmem(void);
extern int  subscribe(int dba);
extern int  unsubscribe(int dba);
extern int firstcall();
