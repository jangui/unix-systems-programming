all: server client

server: server.o clientList.o clientHandling.o input.o
	gcc server.o clientList.o clientHandling.o input.o -pthread -o server -Wall

client: client.o input.o
	gcc client.o input.o -o client -Wall

server.o: server.c 
	gcc -c server.c

client.o: client.c
	gcc -c client.c

input.o: input.c input.h
	gcc -c input.c

clientList.o: clientList.c clientList.h
	gcc -c clientList.c

clientHandling.o: clientHandling.c clientHandling.h
	gcc -c clientHandling.c

clean:
	rm -f server client server.o client.o input.o clientList.o clientHandling.o

