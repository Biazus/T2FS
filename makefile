all:
	gcc -o chamador chamado.c libapidisk.a
	./chamador