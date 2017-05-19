/*
 * adresse.c by J. Ehrlich
 * test decodage adresse
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

    digitalWrite (A0, LOW) ;   // adresse A0=0
    digitalWrite (A1, LOW) ;   // adresse A1=0
    digitalWrite (CS0, LOW) ;  // deselect DAC
    digitalWrite (CS1, LOW) ;  // deselect CAN
    digitalWrite (CS2, LOW) ;  // deselect LATCH
    digitalWrite (REL, DISCHARGE) ;  // discharge mode

    //wiringPiSPISetup(0, 100000); // init SPI interface
}

// RELAY Pin - wiringPi pin 4 is BCM_GPIO 23.

int main (void)
{

    /* Rpi initialization */
    initchadeche();


    printf ("Tapez enter pour selectionner le DAC\n");
    fgetc(stdin);
    digitalWrite (CS0, HIGH) ;  // select DAC
    printf("DAC selectionné\n");

    printf ("Tapez enter pour selectionner le CAN\n");
    fgetc(stdin);
    digitalWrite (CS0, LOW) ;  // deselect DAC
    digitalWrite (CS1, HIGH) ;  // select CAN
    printf("CAN selectionné\n");

    printf ("Tapez enter pour mettre le relais en C\n");
    fgetc(stdin);
    digitalWrite (CS1, LOW) ;  // deselect CAN
    digitalWrite (REL, CHARGE) ;  // relais en C
    digitalWrite (CS2, HIGH) ;  // select LATCH
    wait(5000);
    digitalWrite (CS2, LOW) ;  // deselect LATCH
    printf("Realis en C\n");

    printf ("Tapez enter pour mettre le relais en D\n");
    fgetc(stdin);
    digitalWrite (REL, DISCHARGE) ;  // relais en D
    digitalWrite (CS2, HIGH) ;  // select LATCH
    wait(5000);
    digitalWrite (CS2, LOW) ;  // deselect LATCH
    printf("Relais en D\n");

    printf ("Tapez enter pour terminer\n");
    fgetc(stdin);

  return 0 ;
}
