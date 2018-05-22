all: myshell

myshell: myshell.o line_parser.o job_control.o
	gcc -g -Wall -o myshell myshell.o line_parser.o job_control.o

myshell.o: myshell.c
	gcc -g -Wall -c -o myshell.o myshell.c

line_parser.o: line_parser.c line_parser.h
	gcc -g -Wall -c -o line_parser.o line_parser.c

job_control.o: job_control.c job_control.h
	gcc -g -Wall -c -o job_control.o job_control.c

.PHONY: clean

clean: 
	rm -f *.o myshell
