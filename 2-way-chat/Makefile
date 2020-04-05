all: server client

server: server.o input.o
	gcc server.o input.o -o server -Wall 

server.o: server.c
	gcc -c server.c

client: client.o input.o
	gcc client.o input.o -o client -Wall

client.o: client.c
	gcc -c client.c

input.o: input.c
	gcc -c input.c

clean:
	rm -rf server client server.o client.o input.o
