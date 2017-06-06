chadeche: chadeche.o config.o param.o hw.o
	gcc -o chadeche chadeche.o config.o param.o hw.o -lpthread -lwiringPi -lrt

chadeche.o: chadeche.c chadeche.h
	gcc -c -o chadeche.o chadeche.c -DSHARED_MEMORY

config.o: config.c chadeche.h
	gcc -c -o config.o config.c -DSHARED_MEMORY

param.o: param.c chadeche.h
	gcc -c -o param.o param.c -DSHARED_MEMORY

hw.o: hw.c chadeche.h
	gcc -c -o hw.o hw.c -DSHARED_MEMORY
