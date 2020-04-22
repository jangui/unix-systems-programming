all: server client

server: server.o input.o
	gcc server.o input.o -o server -Wall

client: client.o input.o
	gcc client.o input.o -o client -Wall

server.o: server.c
	gcc -c server.c

client.o: client.c
	gcc -c client.c

input.o: input.c
	gcc -c input.c

clean:
	rm -f server client server.o client.o input.o

