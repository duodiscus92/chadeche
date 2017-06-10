chadeche: chadeche.o config.o param.o hw.o sem.o
	gcc -o chadeche chadeche.o config.o param.o hw.o sem.o -lpthread -lwiringPi -lrt

#chadeche: chadeche.o config.o param.o hw.o
#	gcc -o chadeche chadeche.o config.o param.o hw.o  -lpthread -lwiringPi -lrt

chadeche.o: chadeche.c chadeche.h
	gcc -c -o chadeche.o chadeche.c

config.o: config.c chadeche.h
	gcc -c -o config.o config.c

param.o: param.c chadeche.h
	gcc -c -o param.o param.c

hw.o: hw.c chadeche.h
	gcc -c -o hw.o hw.c

sem.o: sem.c chadeche.h
	gcc -c -o sem.o sem.c -DSHAREDMEMORY
