#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

static sem_t *semaphore = NULL;

void main(void)
{
	sem_unlink("/semaphore");
}

