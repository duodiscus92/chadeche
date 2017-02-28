/*
 * connect.c by J. Ehrlich
 */

#include <stdio.h>
#include <wiringPi.h>

// RELAY Pin - wiringPi pin 4 is BCM_GPIO 23.

#define	RELAY	4
int main (void)
{
  printf ("Connecting battery\n", RELAY) ;

  wiringPiSetup () ;
  pinMode (RELAY, OUTPUT) ;

  digitalWrite (RELAY, HIGH) ;
  return 0 ;
}
