# Readme

## Instructions
* Run executables as normal
* Use make as indicated to build files
* To initialise threads use `export OMP_NUM_THREADS=n`

## loopsAfsOlpN.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism **N**ew
* State: Working
* Affinity scheduling with thread generation outside loops function
* Minimal locks. Main overhead in load transfer

## loopsAfsN.c
**loops** **Af**finity **S**cheduling **N**ew
* State: Working. Fastest right now but only slightly faster than `loopsAfsOlpN.c`
* Tried reducing the overhead in `loopsAfsOlpN.c`by removing unloaded threads from the search list 
* While this reduces the overhead of searching through all threads, the extra statements do add an equal overhead 

## loopsAfsN\_.c
**loops** **Af**finity **S**cheduling **N**ew **Dash**
* State: Working. As fast as `loopsAfsN.c`.*Extra effort may not be worth it.*  
* Tried reducing the overhead in `loopsAfsN.c` by offloading work from threads whihc havent started to threads which are done with their local set. 
* This is advantageous in systems in which thread performance greatly differs. Otherwise it induces an extra overhead in large locked reagions.

## loopsRef.c
**loops** **Ref**erence
* State: Working
* Reference file provided in assignment

## loopsEs.c 
**loops** **E**xplicit **S**cheduling
* State: Working
* Explicit scheduling for loop 1

Old Scripts
===========

## loopsAfsIlp.c
**loops** **Af**finity **S**cheduling **I**nside **L**oop **P**arallelism
* State: Working
* Affinity scheduling with thread generation inside loops function

## loopsAfsOlp.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism
* State: Working
* Affinity scheduling with thread generation outside loops function

## loopsAfsOlp\_.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism **Dash**
* State: Working
* Affinity scheduling with thread generation outside loops function
* An Improvement of loopsAfsOlpN.c :> Runs a check to see if all threads have started executing  loop1.
* Sets the ground work for elminating that barrier at the end of loop1.

## loopsAfs\_.c
**loops** **Af**finity **S**cheduling **Dash**
* State: Working But horribly slow with an increase in threads
* Affinity scheduling with thread generation outside loops function
* An Improvement of loopsAfsOlpN.c :> Runs a check to see if all threads have started executing  loop1.
* **No Barrier at the end of loop1**
* Although it's slow, it sets the foundation for further work:
> Try getting fast threads to assign themselves unassigned segments

## loopsTest.c
* State: Not Working
> Out of bounds array accesses on certain ocassions
* Used for testing ideas
* Redundant at the moment

## Makefile
* Run make to compile all c scripts with name `loop\*.c`
* Make clean to clean out all executables
