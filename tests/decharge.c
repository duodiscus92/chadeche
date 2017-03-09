/*
 * connect.c by J. Ehrlich
 */
#include <stdio.h>
#include <wiringPi.h>

#define A0	0	//GPIO 17
#define A1	2	//GPIO 2 
#define	CS0	4	//GPIO 23 CS0
#define	CS1	5	//GPIO 24 CS1
#define	REL	7	//GPIO 4 commande relais
#define ENDLED  6	//GPIO 6 End of test (red led)

#define CHARGE HIGH
#define DISCHARGE LOW


/* chadeche initialisation */
void initchadeche(void)
{
    wiringPiSetup () ;
    pinMode (A0, OUTPUT) ;
    pinMode (A1, OUTPUT) ;
    pinMode (CS0, OUTPUT) ;
    pinMode (CS1, OUTPUT) ;
    pinMode (REL, OUTPUT) ;
    pinMode (ENDLED, OUTPUT) ;

    digitalWrite (A0, LOW) ;   // adresse A0=0
    digitalWrite (A1, LOW) ;   // adresse A1=0
    digitalWrite (CS0, HIGH) ; // deselect DAC
    digitalWrite (CS1, HIGH) ; // deselect CAN
    digitalWrite (REL, DISCHARGE) ;  // discharge mode

    //wiringPiSPISetup(0, 100000); // init SPI interface
}

// RELAY Pin - wiringPi pin 4 is BCM_GPIO 23.

int main (void)
{

    /* Rpi initialization */
    initchadeche();

    /* end led (red) off */
    digitalWrite (ENDLED, HIGH) ;

    printf ("Position décharge\n") ;

    digitalWrite (REL, DISCHARGE) ;

  return 0 ;
}
