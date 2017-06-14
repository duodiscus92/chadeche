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

/* chadeche hardware initialisation */
//int firstcall = 1;
void initchadeche(void)
{
    sem_wait(semaphore);
    wiringPiSetup () ;
    /* only the first process called can initialize RPi hardware */
    if(firstcall() == 1){
    	pinMode (A0, OUTPUT) ;
    	pinMode (A1, OUTPUT) ;
    	pinMode (CS0, OUTPUT) ;
    	pinMode (CS1, OUTPUT) ;
    	pinMode (CS2, OUTPUT) ;
	pinMode (REL, OUTPUT) ;
    	//pinMode (ENDLED, OUTPUT) ;

    	digitalWrite (CS0, LOW) ; 	// deselect DAC
    	digitalWrite (CS1, LOW) ; 	// deselect CAN
    	digitalWrite (CS2, LOW) ; 	// deselect LATCH
    	digitalWrite (REL, DISCHARGE) ; // discharge mode
     }
     wiringPiSPISetup(0, 100000); 	// init SPI interface
     sem_post(semaphore);
}

/* relay on charge position */
int charge(void)
{
    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   	// adresse A0=0
    digitalWrite (A1, dba&0x02) ;	// adresse A1=0
    //delay(2); // attente stabilisation
    digitalWrite (REL, CHARGE) ;  	// charge mode
    digitalWrite (CS2, HIGH) ;  	// select LATCH
    delay(5);
    digitalWrite (CS2, LOW) ;  		// deselect LATCH
    sem_post(semaphore);
    return 1;
}

/* relay on discharge position */
int discharge(void)
{
    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   	// adresse A0=0
    digitalWrite (A1, dba&0x02) ;   	// adresse A1=0
    //delay(2); // attente stabilisation
    digitalWrite (REL, DISCHARGE) ;  	// discharge mode
    digitalWrite (CS2, HIGH) ;  	// select LATCH
    delay(5);
    digitalWrite (CS2, LOW) ;  		// deselect LATCH
    sem_post(semaphore);
    return 0;
}

/* write avalue into the dac */
void mcp4922write(unsigned int value, unsigned int channel)
{
    unsigned char tmp;
    union uval{
        unsigned short int i;
	unsigned char buf[2];
    }val;

    sem_wait(semaphore);
    digitalWrite (A0, dba&0x01) ;   	// adresse A0=0
    digitalWrite (A1, dba&0x02) ;   	// adresse A1=0
    //delay(2); // attente stabilisation
    val.i = value+4096+8192+16384;
    if(channel == 1) val.i += 32768;
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
    //delay(2); // attente stabilisation
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

/* led début essai */
void begled(void)
{
    mcp4922write(4000, 1);
}

/* led essai en cours*/
void ongoingled(void)
{
    mcp4922write(2000, 1);
}

/* led fin essai */
void endled(void)
{
    mcp4922write(0, 1);
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
