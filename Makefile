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
#S=(loop*.c)
#E=(${S[@]/.c/})
#R=(${E[@]/#/ -not -name })
#E=$(patsubst 
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

rclean: 
	find . -maxdepth 1 -type f \( -not -name "loop*" -not -iname "*.c" -not -iname "*.md" -not -iname "makefile*" \) -exec rm -i {} \;
