
SRCDIR = ../

CC = gcc
CCFLAGS += -Wall -g -std=gnu11
CCFLAGS += -I$(SRCDIR)

bin:
	mkdir bin

test-fat: bin
	$(CC) test-fat.c $(SRCDIR)/fat.c $(CCFLAGS) -o bin/test-fat
