# Makefile for the FEEG6003 coursework

#
# C compiler and options for system
#
CC=	clang-omp
CFLAGS=-O3 -fopenmp
LIB= -lm

SRCS=$(wildcard loop*.c)
#EXECUTABLES=(${SOURCES[@]/%.c/})
EXCS=$(patsubst %.c,%,$(SRCS))
all: $(EXCS)

#
# Compile
#
% : %.c
	$(CC) $(CFLAGS) -o $@ $< $(LIB)

#
# Clean out object files and the executable.
#
clean:
	rm $(EXCS)

