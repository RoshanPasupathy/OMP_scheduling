# Readme

# Instructions
* Run executables as normal
* Use make as indicated to build files
* To initialise threads use `export OMP\_NUM\_THREADS=n`

## loopsRef.c
*loops REFerence*
* State: Working
* Reference file provided in assignment

## loopsEs.c 
*loops Explicit Scheduling*
* State: Working
* Explicit scheduling for loop 1

## loopsAfsIlp.c
*loops AFfinity Scheduling Inside Loop Parallelism*
* State: Working
* Affinity scheduling with thread generation inside loops function

## loopsAfsOlp.c
*loops AFfinity Scheduling Outside Loop Parallelism*
* State: Working
* Affinity scheduling with thread generation outside loops function

## loopsTest.c
* State: Not Working
> Out of bounds array accesses on certain ocassions
* Used for testing ideas
* Redundant at the moment

## Makefile
* Run make to compile all c scripts with name `loop\*.c`
* Make clean to clean out all executables
