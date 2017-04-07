# Readme

## Instructions
* Run executables as normal
* Use make as indicated to build files
* To initialise threads use `export OMP_NUM_THREADS=n`

## loopsRef.c
**loops** **Ref**erence
* State: Working
* Reference file provided in assignment

## loopsEs.c 
**loops** **E**xplicit **S**cheduling
* State: Working
* Explicit scheduling for loop 1

## loopsAfsIlp.c
**loops** **Af**finity **S**cheduling **I**nside **L**oop **P**arallelism
* State: Working
* Affinity scheduling with thread generation inside loops function

## loopsAfsOlp.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism
* State: Working
* Affinity scheduling with thread generation outside loops function

## loopsAfsOlpN.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism **N**ew
* State: Not Working (In Debug Mode)
* Affinity scheduling with thread generation outside loops function
* Threads are not evaluating first few iterations of their segments

## loopsAfsOlp\_.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism **Dash**
* State: Working
* Affinity scheduling with thread generation outside loops function
* An Improvement of loopsAfsOlpN.c :> Runs a check to see if all threads have started executing  loop1.
* Sets the ground work for elminating that barrier at the end of loops1().

## loopsTest.c
* State: Not Working
> Out of bounds array accesses on certain ocassions
* Used for testing ideas
* Redundant at the moment

## Makefile
* Run make to compile all c scripts with name `loop\*.c`
* Make clean to clean out all executables
