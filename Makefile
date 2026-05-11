CC = gcc

all: p1-dataProgram interface

p1-dataProgram: p1-dataProgram.c common.h
	$(CC) -o p1-dataProgram p1-dataProgram.c

interface: interface.c common.h
	$(CC) -o interface interface.c -lm

clean:
	rm -f p1-dataProgram interface juegos.bin tuberia_envio tuberia_respuesta
