CC = gcc
CFLAGS = -O0 -std=c99 -pedantic-errors

watercress: watercress.c
	$(CC) $(CFLAGS) -o watercress watercress.c
