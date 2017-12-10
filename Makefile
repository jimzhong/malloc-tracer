#
# Makefile for the malloc lab driver
#

CC = gcc
CFLAGS = -Wall -Wextra -Werror -O0 -g
LIBS = -lm -pthread

all: mdriver runtrace

mdriver: mdriver.c
	$(CC) $(CFLAGS) -o mdriver mdriver.c $(LIBS)

runtrace: runtrace.c cmd.h
	$(CC) $(CFLAGS) -o runtrace runtrace.c

clean:
	rm -f *~ *.o mdriver
