CC = gcc
CFLAGS = -O0 -std=c99 -pedantic-errors

all: bin bin/watercress

bin:
	mkdir bin

bin/watercress: src/**
	$(CC) $(CFLAGS) -o bin/watercress src/watercress.c
