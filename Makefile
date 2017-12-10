#
# Makefile for the malloc lab driver
#

CC = gcc
CFLAGS = -Wall -Wextra -Werror -O0 -g
LIBS = -lm -lrt

all: mdriver

mdriver: mdriver.c
	$(CC) $(CFLAGS) -o mdriver mdriver.c $(LIBS)

clean:
	rm -f *~ *.o mdriver
