all: main.o prompt.o
	gcc main.o prompt.o -o shell -Wall

main.o: main.c
	gcc -c main.c

prompt.o: prompt.c
	gcc -c prompt.c

clean:
	rm -rf main.o shell
