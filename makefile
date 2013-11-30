all: clear t2fs.o
	gcc -o chamador chamado.c t2fs.o listas.o libapidisk.a -W
	./chamador

t2fs.o: listas.o
	gcc -c t2fs.c -W

listas.o:
	gcc -c listas.c -W

clear:
	rm -rf *.o