#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

//sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
//int sem_wait(sem_t *sem);
//int sem_post(sem_t *sem);
//int sem_unlink(const char *name);

static sem_t *semaphore = NULL;

void mngrsrc(void)
{
    int i;

    for(i=0; i <10; i++){
	printf("Waiting\n");
	sem_wait(semaphore);
	printf("Taking for 3 sec\n");
	usleep(3000000);
	printf("Freeing\n");
	sem_post(semaphore);
	printf("Ignoring for 3 sec\n");
	usleep(3000000);
    }
}


void main(void)
{
	semaphore = sem_open("/semaphore", O_CREAT | O_RDWR | O_EXCL, 0600, 1);
	if (semaphore == SEM_FAILED) {
	    perror("/semaphore");
	    semaphore = sem_open("/semaphore", O_RDWR);
	    if (semaphore == SEM_FAILED) {
	    	perror("/semaphore");
		exit(EXIT_FAILURE);
	    }
	}

	mngrsrc();
	sem_unlink("/semaphore");

}

