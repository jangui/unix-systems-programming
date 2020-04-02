all: main.o prompt.o input.o
	gcc main.o prompt.o input.o -o shell -Wall

main.o: main.c
	gcc -c main.c

prompt.o: prompt.c
	gcc -c prompt.c

input.o: input.c
	gcc -c input.c

clean:
	rm -rf main.o prompt.o input.o shell
