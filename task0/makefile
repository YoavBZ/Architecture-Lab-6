all: myshell

myshell: myshell.o line_parser.o
	gcc -Wall -o myshell myshell.o line_parser.o

myshell.o: myshell.c
	gcc -g -Wall -c -o myshell.o myshell.c

line_parser.o: line_parser.c line_parser.h
	gcc -g -Wall -c -o line_parser.o line_parser.c

.PHONY: clean

clean: 
	rm -f *.o myshell
