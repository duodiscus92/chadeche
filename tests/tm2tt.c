#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

void main(int argc, char *argv[])
{

	struct tm *pt; 
	time_t t, t1;
	int h, m;

	t = time(NULL);

	pt = localtime(&t);
	printf("%s\n", asctime(pt));

	pt->tm_sec = 0;
	pt->tm_min  = atoi(argv[2]);
	pt->tm_hour  = atoi(argv[1]);
	pt->tm_isdst = 0;

	printf("%d %d --> %ld -> %ld\n ", pt->tm_hour, pt->tm_min, t1=mktime(pt), (t1-t) <0 ? t1-t + (24*3600) : t1-t );


}
