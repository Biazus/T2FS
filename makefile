all: clear t2fs.o
	gcc -o chamador chamado.c t2fs.o libapidisk.a -W
	./chamador

t2fs.o:
	gcc -c t2fs.c -W

clear:
	rm -rf *.o