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

// LEND Pin - wiringPi pin 0 is BCM_GPIO 17.

//#define	LEND	7	//GPIO 4 commande relais
//#define	LEND	4	//GPIO 23 CS0
//#define	LEND	5	//GPIO 24 CS1
#define	LEND	6	//GPIO 6
#define A0	0	//GPIO 17
#define A1	2	//GPIO 2 

int main (void)
{
  printf ("Raspberry Pi blink Pin Nr : %d\n", LEND) ;

  wiringPiSetup () ;
  pinMode (LEND, OUTPUT) ;
  pinMode (A0, OUTPUT) ;
  pinMode (A1, OUTPUT) ;

  digitalWrite (A0, LOW) ;	// On
  digitalWrite (A1, LOW) ;	// On

  for (;;)
  {
    digitalWrite (LEND, HIGH) ;	// On
    delay (1000) ;		// mS
    digitalWrite (LEND, LOW) ;	// Off
    delay (1000) ;
  }
  return 0 ;
}
