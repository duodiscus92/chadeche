#include <stdio.h>
#include <string.h>

typedef struct config {
	char cop;
	int  milliamp;
	int duration;
	char comment[80];
} CONFIG;

CONFIG tconfig[50];

int main (void)
{
	FILE *fp;
	int i = 0, j ;
	char buffer[80];

	if ((fp = fopen("chadeche.txt", "r")) == NULL){
	    printf("impossible ouvrir fichier config\n");
	    return -1;
	}

	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    sscanf(buffer, "%c%d%d\n",
		&(tconfig[i].cop),
		&(tconfig[i].milliamp),
		&(tconfig[i].duration));
	    strcpy(tconfig[i].comment, strchr(buffer, '#')+1);
	    i++;
	}
	for(j=0; j < i-1; j++)
	    printf("%c\t%d\t%d\t%s",
		(tconfig[j].cop),
		(tconfig[j].milliamp),
		(tconfig[j].duration),
		tconfig[j].comment);
	fclose(fp);
}


