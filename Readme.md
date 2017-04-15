# OpenMP Coursework

Various schedulers to maximise threaded performance while executing loops. The Non affinity schedulers werent difficult to write and are merely for benchmarking purposes and completeness.

## Instructions
* Modify Makefile to use compiler of your choice
* To initialise threads use `export OMP_NUM_THREADS=n`

## Makefile
* Run make to compile all c scripts with name `loop*.c`
* Use `make` as indicated to build files
* Use `make clean` to clear all executables of scripts `loop*.c`
* Use `make rclean` for redundant executables. 

***

## Current Scripts

## 1. loopsAfsOlpN.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism **N**ew
* State: Working
* Affinity scheduling with thread generation outside loops function
* Minimal locks. Main overhead in load transfer

## 2. loopsAfsN.c
**loops** **Af**finity **S**cheduling **N**ew
* State: Working. Fastest right now but only slightly faster than `loopsAfsOlpN.c`
* Tried reducing the overhead in `loopsAfsOlpN.c`by removing unloaded threads from the search list 
* While this reduces the overhead of searching through all threads, the extra statements do add an equal overhead 

## 3. loopsAfsN\_.c
**loops** **Af**finity **S**cheduling **N**ew **Dash**
* State: Working. As fast as `loopsAfsN.c`.*Extra effort may not be worth it.*  
* Tried reducing the overhead in `loopsAfsN.c` by offloading work from threads which havent started to threads which are done with their local set. 
* This is advantageous in systems in which thread performance greatly differs. Otherwise it induces an extra overhead in large locked reagions.
* **Faster than guided scheduling**

## 4. loopsRef.c
**loops** **Ref**erence
* State: Working
* Reference file provided in assignment

## 5. loopsEs.c 
**loops** **E**xplicit **S**cheduling
* State: Working
* Explicit scheduling for loop 1

## 6. loopsGs.c 
**loops** **G**uided **S**cheduling
* State: Working
* Guided scheduling 
* Minimal wiritng time (5 minutes as opposed to a week writing `loopsAfsN_.c`)

## 7. loopsAfsComp2.c
**loops** **Af**finity **S**cheduling **Comp**ressed **2**
* State: Working loop1 and loop2 parallelism
* **Current Best version: Reasonabl fast and robust**
* No barrier needed at the end of each rep
* Needs some cleanup

***

## Old Scripts

## 1. loopsAfsIlp.c
**loops** **Af**finity **S**cheduling **I**nside **L**oop **P**arallelism
* State: Working
* Affinity scheduling with thread generation inside loops function

## 2. loopsAfsOlp.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism
* State: Working
* Affinity scheduling with thread generation outside loops function

## 3. loopsAfsOlp\_.c
**loops** **Af**finity **S**cheduling **O**utside **L**oop **P**arallelism **Dash**
* State: Working
* Affinity scheduling with thread generation outside loops function
* An Improvement of `loopsAfsOlpN.c` :> Runs a check to see if all threads have started executing  loop1.
* Sets the ground work for elminating that barrier at the end of loop1.

## 4. loopsAfs\_.c
**loops** **Af**finity **S**cheduling **Dash**
* State: Working But horribly slow with an increase in threads
* Affinity scheduling with thread generation outside loops function
* An Improvement of `loopsAfsOlpN.c` :> Runs a check to see if all threads have started executing  loop1.
* **No Barrier at the end of loop1**
* Although it's slow, it sets the foundation for further work:
> Try getting fast threads to assign themselves unassigned segments

## 5. loopsTest.c
* State: Not Working
> Out of bounds array accesses on certain ocassions
* Used for testing ideas
* Redundant at the moment

***
