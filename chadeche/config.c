#include "chadeche.h"

CONFIG tconfig[MAX_STEP];

/* read the config file and store values into tconfig */
void readconf(char *filename)
{
	FILE *fp;
	int i = 0;
	char buffer[80];

	if ((fp = fopen(filename, "r")) == NULL){
	    printf(language == EN ? "Program aborted : can't open config file %s\n" : \
				    "Programme abandonné : impossible ouvir fichier de configuration %s\n", filename);
	    exit(1);
	}

	/* first line always ignored */
	fgets(buffer, 80, fp);
	while (!feof(fp)) {
	    fgets(buffer, 80, fp);
	    //sscanf(buffer, "%d;%c;%d;%d;%[A-Z0-9\\-];%[A-Z0-9\\-];%[A-Z0-9\\-];%[A-Z0-9\\-]\n",
	    sscanf(buffer, "%d\t%c\t%d\t%d\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\t%[A-Z0-9\\-\\:]\n",
		&(tconfig[i].adr),
		&(tconfig[i].cop),
		&(tconfig[i].milliamp),
		&(tconfig[i].duration),
		tconfig[i].toolow,
		tconfig[i].toohigh,
		tconfig[i].toolowcapacity,
		tconfig[i].toohighcapacity,
		tconfig[i].always,
		tconfig[i].controlflag);
	    strcpy(tconfig[i].comment, strchr(buffer, '#')+1);
	    if (i++ > MAX_STEP){
		printf(language == EN ? "Program aborted : too much step in a configuation file. Must not exceed %d\n" : \
					"Programme abandonné : trop de séquence dans un fichier de configuration. Ne peut  pas dépasser %d\n", MAX_STEP);
		exit(1);
	    }
	}
	tconfig[i-1].cop = 0; // pour marquer le fin de config
}


