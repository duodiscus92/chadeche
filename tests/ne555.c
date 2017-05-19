/*
 * blink.c:
 *	Standard "blink" program in wiringPi. Blinks an LEND connected
 *	to the first GPIO pin.
 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdlib.h>

#define A0	0	//GPIO 17
#define A1	2	//GPIO 27 
#define	CS0	9	//GPIO 3 CS0
#define	CS1	22	//GPIO 6 CS1
#define	CS2	21	//GPIO 5 CS2
#define	REL	7	//GPIO 4 commande relais

#define CHARGE HIGH
#define DISCHARGE LOW


unsigned char halfbyterev(unsigned char direct)
{
	unsigned char rev;
	
	rev = direct & 0x0F;
	rev <<= 4;
	direct >>= 4;
	rev += direct;
	return rev;
}

unsigned char bitreverse(unsigned char direct){
	unsigned char rev;
	int i;
	
	for (i=0; i<8; i++, direct <<= 1, rev >>= 1)
		if(direct & 0x80)
			rev |= 0x80;
	return rev;
}

int main (int argc, char *argv[])
{
	unsigned char buf[2], high, low, tmp;
	unsigned int i, filtered, dba;
	union uval{
		unsigned short int i;
		unsigned char buf[2];
	}val;
	//char buffer[2] = "\x05\x55";
	printf ("Raspberry Pi Test MCP4922\n") ;

	dba = atoi(argv[1]);
	wiringPiSetup () ;
	pinMode (A0, OUTPUT) ;
	pinMode (A1, OUTPUT) ;
	pinMode (CS0, OUTPUT) ;
	pinMode (CS1, OUTPUT) ;
	//pinMode (REL, OUTPUT) ;

	digitalWrite (A0, dba & 0x01) ;
	digitalWrite (A1, dba & 0x02) ;
	digitalWrite (CS0, LOW) ;
	digitalWrite (CS1, LOW) ;
	//digitalWrite (REL, LOW) ;


	wiringPiSPISetup(0, 100000);
	for (;;) {
		for(i= 0; i<4096; i++){
			printf("Valeur:"); fflush(stdout);
			scanf("%d", &i);
			//val.i = i;
			val.i = i+4096+8192+16384+32768;
			//val.buf[1] = halfbyterev(val.buf[1]);
			//val.buf[0] = halfbyterev(val.buf[0]);
			tmp= val.buf[0];
			val.buf[0] = val.buf[1];
			val.buf[1] = tmp;
			//printf("%02x%02x\n", val.buf[1], val.buf[0]);
			digitalWrite (CS0, HIGH) ;
			wiringPiSPIDataRW(0, val.buf, 2);
			digitalWrite (CS0, LOW) ;
			for(;;){
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
			   //filtered = 0.95*filtered + 0.05*val.i;
			   printf("Lecture : %d\n", filtered>>3);
			   //printf("Lecture : %d\n", val.i);
			   //delay (100);
			}
		}
	}
	return 0;
  }
