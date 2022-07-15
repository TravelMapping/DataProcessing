# Makefile for archivegraphs C program

PROGRAM=archivegraphs
LIBMETALGRAPH=-lmetalgraph
LIBMETALGRAPHPATH=$(HOME)/travelmapping/TMInstructorTools/CGraphProgram/libmetalgraph
CC=gcc
CFLAGS=-Wall -g -I$(LIBMETALGRAPHPATH)

$(PROGRAM):	$(PROGRAM).o $(LIBMETALGRAPHPATH)/libmetalgraph.a
	$(CC) $(CFLAGS) -o $(PROGRAM) $(PROGRAM).o -L$(LIBMETALGRAPHPATH) $(LIBMETALGRAPH) -lreadline -lm

clean::
	/bin/rm $(PROGRAM) $(PROGRAM).o
