all: server client

server: server.o clientList.o clientHandling.o input.o msgQueue.o
	gcc server.o clientList.o clientHandling.o input.o msgQueue.o -pthread -o server -Wall

client: client.o input.o
	gcc client.o input.o -o client -Wall

server.o: server.c 
	gcc -c server.c -Wall

client.o: client.c
	gcc -c client.c -Wall

input.o: input.c input.h
	gcc -c input.c -Wall

clientList.o: clientList.c clientList.h
	gcc -c clientList.c -Wall

clientHandling.o: clientHandling.c clientHandling.h
	gcc -c clientHandling.c -Wall

msgQueue.o: msgQueue.c msgQueue.h
	gcc -c msgQueue.c -Wall

clean:
	rm -f server client server.o client.o input.o clientList.o clientHandling.o msgQueue.o

