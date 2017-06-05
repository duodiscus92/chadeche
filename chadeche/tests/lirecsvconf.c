#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef struct config {
	int adr;
	char cop;
	int  milliamp;
	int duration;
	//char toolow[2];
	//char toohigh[2];
	//char controlflag[2];
	char toolow[5];
	char toohigh[5];
	char controlflag[5];
	char comment[80];
} CONFIG;

CONFIG tconfig[50];

int main (int argc, char **argv)
{
	FILE *fp;
	int i = 0, j ;
	char buffer[80], *endptr, *str;
	long val;

	if ((fp = fopen(argv[1], "r")) == NULL){
	    printf("impossible ouvrir fichier %s\n", argv[1]);
	    return -1;
	}

	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    //printf("%s\n", buffer);
	    //sscanf(buffer, "%c%d%d%c%%c%%c",
	    //sscanf(buffer, "%c%d%d%s%s%s",
	    sscanf(buffer, "%d;%c;%d;%d;%[A-Z0-9\\-];%[A-Z0-9\\-];%[A-Z0-9\\-]\n",
		&(tconfig[i].adr),
		&(tconfig[i].cop),
		&(tconfig[i].milliamp),
		&(tconfig[i].duration),
		tconfig[i].toolow,
		tconfig[i].toohigh,
		tconfig[i].controlflag
	    );
	    strcpy(tconfig[i].comment, strchr(buffer, '#')+1);
	    i++;
	}
	fclose(fp);
	for(j=0; j < i-1; j++){
	    //printf("%c\t%d\t%d\t%c\t%c\t%c\t%s\n",
	    val = strtol(str = (tconfig[j].toolow),  &endptr, 10);
	    endptr == str ? printf("%c", str[0]) : printf("%d", val);
	    val = strtol(str = (tconfig[j].toohigh),  &endptr, 10);
	    endptr == str ? printf("\t%c", str[0]) : printf("\t%d", val);
	    val = strtol(str = (tconfig[j].controlflag),  &endptr, 10);
	    endptr == str ? printf("\t%c", str[0]) : printf("\t%d", val);
	    printf("\t%d\t%c\t%d\t%d\t%s",
		(tconfig[j].adr),
		(tconfig[j].cop),
		(tconfig[j].milliamp),
		(tconfig[j].duration),
		tconfig[j].comment);
	}
}


