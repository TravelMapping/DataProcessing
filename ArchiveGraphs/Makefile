# Makefile for archivegraphs C program

PROGRAM=archivegraphs
OTHERCFILES=stringfuncs.c
CFILES=$(PROGRAM).c $(OTHERCFILES)
OFILES=$(CFILES:.c=.o)
LIBMETALGRAPH=-lmetalgraph
LIBMETALGRAPHPATH=$(HOME)/travelmapping/TMInstructorTools/CGraphProgram/libmetalgraph
CC=gcc
CFLAGS=-Wall -g -I$(LIBMETALGRAPHPATH)

$(PROGRAM):	$(OFILES) $(LIBMETALGRAPHPATH)/libmetalgraph.a
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OFILES) -L$(LIBMETALGRAPHPATH) $(LIBMETALGRAPH) -lreadline -lm

clean::
	/bin/rm $(PROGRAM) $(OFILES)
