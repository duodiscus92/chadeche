#include <stdio.h>
#include <string.h>

typedef struct config {
	char cop;
	int  milliamp;
	int duration;
	//char toolow[2];
	//char toohigh[2];
	//char controlflag[2];
	char toolow;
	char toohigh;
	char controlflag;
	char comment[80];
} CONFIG;

CONFIG tconfig[50];

int main (int argc, char **argv)
{
	FILE *fp;
	int i = 0, j ;
	char buffer[80];

	if ((fp = fopen(argv[1], "r")) == NULL){
	    printf("impossible ouvrir fichier %s\n", argv[1]);
	    return -1;
	}

	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    //printf("%s\n", buffer);
	    //sscanf(buffer, "%c%d%d%c%%c%%c",
	    //sscanf(buffer, "%c%d%d%s%s%s",
	    sscanf(buffer, "%c%d%d%*c%c%*c%c%*c%c\n",
		&(tconfig[i].cop),
		&(tconfig[i].milliamp),
		&(tconfig[i].duration),
		&(tconfig[i].toolow),
		&(tconfig[i].toohigh),
		&(tconfig[i].controlflag)
	    );
	    strcpy(tconfig[i].comment, strchr(buffer, '#')+1);
	    i++;
	}
	for(j=0; j < i-1; j++)
	    //printf("%c\t%d\t%d\t%c\t%c\t%c\t%s\n",
	    printf("%c\t%d\t%d\t%c\t%c\t%c\t%s\n",
		(tconfig[j].cop),
		(tconfig[j].milliamp),
		(tconfig[j].duration),
		//(tconfig[j].toolow[0]),
		//(tconfig[j].toohigh[0]),
		//(tconfig[j].controlflag[0]),
		(tconfig[j].toolow),
		(tconfig[j].toohigh),
		(tconfig[j].controlflag),
		tconfig[j].comment);
	fclose(fp);
}


