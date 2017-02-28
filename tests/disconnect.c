/*
 * disconnect.c by J. Ehrlich
 */

#include <stdio.h>
#include <wiringPi.h>

// RELAY Pin - wiringPi pin 4 is BCM_GPIO 23.

#define	RELAY	4
int main (void)
{
  printf ("Disconnecting battery\n", RELAY) ;

  wiringPiSetup () ;
  pinMode (RELAY, OUTPUT) ;

  digitalWrite (RELAY, LOW) ;	// Off
  return 0 ;
}
