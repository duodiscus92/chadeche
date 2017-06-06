chadeche: chadeche.o config.o param.o hw.o sem.o
	gcc -o chadeche chadeche.o config.o param.o hw.o sem.o -lpthread -lwiringPi -lrt

#chadeche: chadeche.o config.o param.o hw.o
#	gcc -o chadeche chadeche.o config.o param.o hw.o  -lpthread -lwiringPi -lrt

chadeche.o: chadeche.c chadeche.h
	gcc -c -o chadeche.o chadeche.c -DSHAREDMEMORY

config.o: config.c chadeche.h
	gcc -c -o config.o config.c -DSHAREDMEMORY

param.o: param.c chadeche.h
	gcc -c -o param.o param.c -DSHAREDMEMORY

hw.o: hw.c chadeche.h
	gcc -c -o hw.o hw.c -DSHAREDMEMORY

sem.o: sem.c chadeche.h
	gcc -c -o sem.o sem.c -DSHAREDMEMORY
