all: server client

server: server.o
	gcc server.o -o server -Wall 

server.o: server.c
	gcc -c server.c

client: client.o
	gcc client.o -o client -Wall

client.o: client.c
	gcc -c client.c

clean:
	rm -rf server.o server client.o client
