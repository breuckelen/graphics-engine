CC = gcc

all: mat4.o parse_util.o proj.o
	$(CC) mat4.o parse_util.o proj.o -o proj

proj.o:
	$(CC) -c proj.c -o proj.o

mat4.o:
	$(CC) -c mat4.c -o mat4.o

parse_util.o:
	$(CC) -c parse_util.c -o parse_util.o

clean:
	rm *.o
	rm proj
