# Student's Makefile for the CS:APP Performance Lab
TEAM = bovik
VERSION = 1
HANDINDIR = 

CC = gcc
CFLAGS = -Wall -O2 -m32
LIBS = -lm

OBJS = driver.o kernels.o fcyc.o clock.o

all: driver test

driver: $(OBJS) fcyc.h clock.h defs.h config.h
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o driver

test: $(OBJS) test.o fcyc.h clock.h defs.h config.h
	$(CC) $(CFLAGS) test.o $(LIBS) -o test

handin:
	cp kernels.c $(HANDINDIR)/$(TEAM)-$(VERSION)-kernels.c

clean: 
	-rm -f $(OBJS) driver test core *~ *.o


