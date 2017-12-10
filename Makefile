#
# Makefile for the malloc lab driver
#
# Regular compiler
CC = gcc

# Change this to -O0 (big-Oh, numeral zero) if you need to use a debugger on your code
COPT = -O0
CFLAGS = -Wall -Wextra -Werror $(COPT) -g -DDRIVER -Wno-unused-function -Wno-unused-parameter
LIBS = -lm -lrt

COBJS = memlib.o fcyc.o clock.o stree.o
NOBJS = mdriver.o mm-native.o $(COBJS)

all: mdriver

# Regular driver
mdriver: $(NOBJS)
	$(CC) $(CFLAGS) -o mdriver $(NOBJS) $(LIBS)

# The lab comes with Conctech.cpp precompiled as Contech.so
# This bit of magic builds on the LLVM compiler infrastructure
# Contech.so: Contech.cpp Contech.h ct_event_st.h
#	$(CC) -shared -o Contech.so -I/usr/include/llvm -L/usr/lib64/llvm Contech.cpp -std=c++11 -D__STDC_CONSTANT_M ACROS -D__STDC_LIMIT_MACROS -fPIC

mdriver.o: mdriver.c fcyc.h clock.h memlib.h config.h mm.h stree.h
fcyc.o: fcyc.c fcyc.h
clock.o: clock.c clock.h
stree.o: stree.c stree.h

clean:
	rm -f *~ *.o mdriver
