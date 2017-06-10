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

/* data for shared memory */
static int mid;
static struct procdesc {
    int nprocess;
    int tprocess[NB_BOARD];
} *p;

/* creation du sémaphore */
int initsem(void)
{
    /* semaphore creation and initialization */
    semaphore = sem_open("/semaphore", O_CREAT | O_RDWR | O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
	//perror("/semaphore");
	semaphore = sem_open("/semaphore", O_RDWR);
	if (semaphore == SEM_FAILED) {
	    perror("/semaphore : Unable to open existing semaphore\n");
	    return -1;
	 }
     }
     else{
	firstcall = 0;
#ifdef DEBUG
	printf("semaphore  created\n");
#endif
     }
     return 0;
}

/* destruction du sempahore */
void termsem(void)
{
    int i;

    sem_wait(semaphore);
    i = p->nprocess;
    sem_post(semaphore);
    sem_unlink("/semaphore");
}

/* creation mémoire partagée */
int initmem(void)
{

#ifdef SHAREDMEMORY
#ifdef DEBUG
    printf("Trying to create shared memorey using name = %s\n", MEM_NAME);
#endif
    if ((mid= shm_open(MEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )) == -1) {
	//perror("shm_open : warning : shared memory already exists");
	if ((mid= shm_open(MEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )) == -1) {
	    perror("shm_open : error : memory already exists and unable to opent it");
	    return -1;
	}
#ifdef DEBUG
	else
	   printf("An already exisitng shared memory will be used with mid = %ld\n", mid);
#endif
    }
#ifdef DEBUG
    else
	printf("A new shared memory is created with mid = %ld\n", mid);
#endif
    /* shared memory mapping */
    ftruncate(mid, sizeof(struct procdesc));
    if ((p = mmap(p, sizeof(struct procdesc), PROT_READ | PROT_WRITE, MAP_SHARED, mid, 0)) == (struct procdesc *)-1) {
	perror("mmap: unable to map shared memory");
    	if (shm_unlink(MEM_NAME) == -1)
	    perror("shm_unlink: unable to free shared memory");
	return -1;
    }
#ifdef DEBUG
    else
	printf("mmap: memory mapped\n");
#endif
#endif
    return 0;
}

/* destruction mémoire partagée */
int termmem(void)
{

   int i;
#ifdef SHAREDMEMORY
    sem_wait(semaphore);
    i = p->nprocess;
    sem_post(semaphore);
    if (munmap(p, sizeof(struct procdesc)) == -1)
	perror("munmap : unable to unmap shared memory");
#ifdef DEBUG
    else
	printf("Unmapping shared memory befor exiting\n");
#endif
    /* if this process is the last one */
    if(i == 0){
#ifdef DEBUG
	printf("Last process ending : removing shared memory\n");
#endif
	if (shm_unlink(MEM_NAME) == -1)
	    perror("shm_unlink: unable to free shared memory");
    	//sem_unlink("/semaphore");
    }
#endif
    return i;
}

/* enregistrer un process chadeche */
int subscribe(int dba)
{
   int i ;
#ifdef SHAREDMEMORY
     /* shared memory test and initialisation */
     sem_wait(semaphore);
     if(p->tprocess[dba] != 0){
	sem_post(semaphore);
	printf(language == EN ? "Program aborted !!! A process nr. %d is already using this board nr. %d\n" : \
				"Progamme abandonné !!! Un processus n° %d utilise déjà cetee carte n° %d\n", p->tprocess[dba], dba);
	return -1;
     }
     else {
     	p->nprocess++;
     	p->tprocess[dba] = p->nprocess; 
     	sem_post(semaphore);
     }
#ifdef DEBUG
	printf("Process nr. %d launched : Using the board %d\n", p->nprocess, dba);
#endif
#endif
    return p->nprocess;;
}

/* desenregistrer un process chadeche */
int unsubscribe(int dba)
{
   int i;
#ifdef SHAREDMEMORY
    sem_wait(semaphore);
    p->nprocess--;
    p->tprocess[dba] = 0; 
    i = p->nprocess;
    sem_post(semaphore);
#endif
   return i;
}
