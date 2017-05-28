/*
 * connect.c by J. Ehrlich
 */
#include <stdio.h>
#include <wiringPi.h>

#define A0	0	//GPIO 17
#define A1	2	//GPIO 27 
#define	CS0	9	//GPIO 3 CS0
#define	CS1	22	//GPIO 6 CS1
#define	CS2	21	//GPIO 5 CS2
#define	REL	7	//GPIO 4 commande relais

#define CHARGE HIGH
#define DISCHARGE LOW

int dba;

/* chadeche initialisation */
void initchadeche(void)
{
    wiringPiSetup () ;
    pinMode (A0, OUTPUT) ;
    pinMode (A1, OUTPUT) ;
    pinMode (CS0, OUTPUT) ;
    pinMode (CS1, OUTPUT) ;
    pinMode (CS2, OUTPUT) ;
    pinMode (REL, OUTPUT) ;

    digitalWrite (A0, dba & 0x01) ;
    digitalWrite (A1, dba & 0x02) ;
    digitalWrite (CS0, LOW) ;
    digitalWrite (CS1, LOW) ;
    digitalWrite (CS2, LOW) ;
    digitalWrite (REL, DISCHARGE) ;  // discharge mode
}

// RELAY Pin - wiringPi pin 4 is BCM_GPIO 23.

int main (int argc, char **argv)
{
    dba = atoi(argv[1]);

    /* Rpi initialization */
    initchadeche();

    digitalWrite (REL, DISCHARGE) ;  // charge mode
    digitalWrite (CS2, HIGH) ;  // select LATCH
    delay(5);
    digitalWrite (CS2, LOW) ;  // deselect LATCH
    printf ("Position charge\n") ;

  return 0 ;
}
