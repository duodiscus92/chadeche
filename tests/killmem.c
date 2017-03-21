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
#include <sys/ipc.h>
#include <sys/mman.h>

#define NB_BOARD 4	//max number of daugther board
#define MEM_KEY	3245617	// chared memory key for shmget()
#define MEM_NAME "/sharedmem"

/* data for shared memory */
struct procdesc {
    int nprocess;
    int tprocess[NB_BOARD];
};
struct procdesc *p;

void main(void)
{

    int i;
    int mid, memexist = 0;

    //printf("Sysconf : %ld\n", sysconf(_SC_PAGESIZE));

#ifdef CREATE
    /* shared memory creation */
    printf("Trying to create shared memorey using name = %s\n", MEM_NAME);
    if ((mid= shm_open(MEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
	perror("shm_open : attention : la mémoire existe déjà");
	if ((mid= shm_open(MEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
	    perror("shm_open : erreur la mémoire existe déja et impossible y acceder");
	    exit(1);
	}
	else {
	   printf("An already exisitng shared memory will be used with mid = %ld\n", mid);
	   memexist = 1;
	}
    }
    else
	printf("A new shared memory is created with mid = %ld\n", mid);
    /* shared memory mapping */
    ftruncate(mid, sizeof(struct procdesc));
    if ((p = mmap(NULL, sizeof(struct procdesc), PROT_READ | PROT_WRITE, MAP_SHARED, mid, 0)) == (struct procdesc *)-1) {
	perror("mmap: unable to map shared memory");
    	if (shm_unlink(MEM_NAME) == -1)
	    perror("shm_unlink: unable to free shared memory");
	exit(1);
	//goto endchadeche;
    }
    else {
	printf("mmap: memory mapped\n");
	p->nprocess = 0;
	printf("Nprocess = %d\n", p->nprocess);
    }
#endif
    /* free shared memory */
if(memexist){
    if (munmap(p, sizeof(struct procdesc)) == -1)
    	perror("munmap : unable to unmap shared memry");
    else
	printf("Memory unmapped\n");
    if (shm_unlink(MEM_NAME) == -1)
    	perror("shm_unlink: unable to free shared memory");
    else
	printf("Memory unlinked\n");
}

}

