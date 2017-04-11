#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define N 729
#define reps 100
#include <omp.h>

double a[N][N], b[N][N], c[N];
int jmax[N];

int cnt_ldd_ts[reps]; //number of loaded threads per rep
int P; //number of threads
int tf; //transfer factor
int seg_sz; //iterations allocated to each thread except last
int seg_szP;
int* itr_left; //iterations left in each thread
int* d_seg_sz; // seg assigned to each thread
int* t_itr_end; // end of segments
int* ldd_tarr; //array of loaded threads
int* thread_rng; //indicate thread is running
int* is_assigned;
int ut;
omp_lock_t lock_ut;
int r; //reps iterator
//int tot_itr = N; //total iterations left
//int t_no;
//int* itr_prf;
int itr_prf[N] = {0};
int max_threads;
omp_lock_t* lock_itr_left;

void init1(void);
void init2(void);
void loop1(void);
void loop2(void);
void valid1(void);
void valid2(void);
void debug_race(void);
void loop1bounds(int lb, int ub);

int main(int argc, char *argv[]) {
  //int r;
  double start1,start2,end1,end2;
  int t, lk;
  #pragma omp parallel default(none) \
  shared(P)
  {
    #pragma omp single
    {
      P = omp_get_num_threads();
      max_threads = omp_get_max_threads();
    }
  }

  //tf = P/(float) (P - 1);

  //Constant values
  tf = P;
  printf("number of threads: %d\n", P);
  for (int i =0; i < reps; i++){
    cnt_ldd_ts[i] = P;
  }
  ut = P;
  seg_sz = (int) (N/P);
  seg_szP = seg_sz + N - (seg_sz * P);
  lock_itr_left = (omp_lock_t*)malloc(sizeof(omp_lock_t) * (P + 1));

  //Variable values
  d_seg_sz = (int *) malloc(sizeof(int) * P);
  itr_left = (int *) malloc(sizeof(int) * P);
  t_itr_end = (int *) malloc(sizeof(int) * P);
  ldd_tarr = (int *) malloc(sizeof(int) * P);
  thread_rng = (int *) malloc(sizeof(int) * P);
  is_assigned = (int *) malloc(sizeof(int) * P);

  if ((itr_left == NULL) || (d_seg_sz == NULL) || (lock_itr_left == NULL) || (t_itr_end == NULL) ) {
    printf("FATAL: Malloc failed ... \n");
  }
  else{
    printf("Allocation: OKAY\n");
  }
  //Initialise lock
  omp_init_lock(&lock_ut);
  for (lk = 0; lk < P; lk++){
    omp_init_lock(&(lock_itr_left[lk]));
    itr_left[lk] = 0;
    d_seg_sz[lk] = seg_sz;
    ldd_tarr[lk] = lk; //at the start all threads are loaded
    thread_rng[lk] = 0;
    t_itr_end[lk] = 0;
    is_assigned[lk] = 0;
  }
  is_assigned[P] = 0;
  d_seg_sz[P-1]+= N - (seg_sz*P);
  //t_itr_end[P-1] = N;

  init1();
  //printf("iterations allocated to each thread: %d\n", seg_sz);

  start1 = omp_get_wtime();

  #pragma omp parallel default(none) \
  shared(P, tf, seg_sz,seg_szP, itr_left,d_seg_sz, t_itr_end) \
  shared(ldd_tarr, ut, lock_ut, thread_rng, is_assigned) private(r)
  {
  for (r=0; r<reps; r++){
    //#pragma omp critical(printf_lock)
    //{printf("main function r: %d\n",r);}
    loop1();
  // }
  }
  }
  end1  = omp_get_wtime();

  valid1();

  printf("Total time for %d reps of loop 1 = %f\n",reps, (float)(end1-start1));


  init2();

  start2 = omp_get_wtime();

  for (r=0; r<reps; r++){
    loop2();
  }

  end2  = omp_get_wtime();

  valid2();

  printf("Total time for %d reps of loop 2 = %f\n",reps, (float)(end2-start2));
  //debug_race();
  free(d_seg_sz);
  free(itr_left);
  free(lock_itr_left);
  free(t_itr_end);
  free(ldd_tarr);
  free(thread_rng);
}

void init1(void){
  int i,j;

  for (i=0; i<N; i++){
    for (j=0; j<N; j++){
      a[i][j] = 0.0;
      b[i][j] = 3.142*(i+j);
    }
  }

}

void init2(void){
  int i,j, expr;

  for (i=0; i<N; i++){
    expr =  i%( 3*(i/30) + 1);
    if ( expr == 0) {
      jmax[i] = N;
    }
    else {
      jmax[i] = 1;
    }
    c[i] = 0.0;
  }

  for (i=0; i<N; i++){
    for (j=0; j<N; j++){
      b[i][j] = (double) (i*j+1) / (double) (N*N);
    }
  }

}

void loop1(void) {
  int i,j,t, lb;
  int chnk_sz, ub;
  int threadt, lt;
  int checkThreadRng;
  //maximum load, load, loaded thread
  int max_val, val, ldd_t;
  int itr, t_no;
  int curr_rep;
  int seg;

  int cnt_nt_strt;
  /*
  default initial segment is
  corresponds to thread number
  */
  t_no = omp_get_thread_num();
  seg = t_no;
  //set iterations
  lb = t_no * seg_sz;
  max_val = tf;

  omp_set_lock(&(lock_itr_left[t_no]));
  //#pragma omp critical (printf_lock)
  //{printf(" Attempting to read seg size T%d for r: %d\n",t_no, r);}
  //#pragma omp atomic read
  //#pragma omp critical (Set_Seg_Sz)

  //#pragma omp critical (printf_lock)
  //{printf("T: %d, Rep %d\n", t_no, thread_rng[t_no]);}
  curr_rep = thread_rng[t_no];
  #pragma omp atomic read
  itr = d_seg_sz[t_no];
  t_itr_end[t_no] = (t_no*seg_sz) + itr;
  itr_left[t_no] = itr;
  #pragma omp atomic update
  is_assigned[seg]++; //update info that something is running in this seg
  //#pragma omp critical (printf_lock)
  //{printf("Thread %d attempting to update thread running\n",t_no);
  #pragma omp atomic update
  thread_rng[t_no]++;
  //printf("Thread %d, update thread running\n", t_no);}

  //reset d_seg_sz
  // at this point the thread is initialised so
  // no more attempts to write to or read from it
  d_seg_sz[t_no] = (t_no == P - 1)? seg_szP: seg_sz;
  //if (t_no == P - 1){
  //  d_seg_sz[t_no] += N - (seg_sz*P);
  //}

  // DEBUG
  //#pragma omp critical (printf_lock)
  //{printf("Thread %d start iterating from %d\n",t_no, lb);}
  while (max_val > 0){
    while (itr > 0){
      // New chunk size
      chnk_sz = (itr < P)? 1: (int) (itr/P);
      itr_left[t_no] -= chnk_sz;
      omp_unset_lock(&(lock_itr_left[t_no]));
      ub = lb + chnk_sz;
      for (;lb < ub; lb++){
        //#pragma omp atomic update //DEBUG
        //itr_prf[lb]++;             //DEBUG
        for (j=N-1; j>lb; j--){
          a[lb][j] += cos(b[lb][j]);
        }
      }
      omp_set_lock(&(lock_itr_left[t_no]));
      itr = itr_left[t_no];
    } // LOCAL SET COMPLETE
    omp_unset_lock(&(lock_itr_left[t_no]));
    //indicate to all threads that one less thread is running in seg
    #pragma omp atomic update
    is_assigned[seg]--;

    // Load transfer block //
    max_val = tf;
    ldd_t = t_no;
    lt = 0;
    cnt_nt_strt = 0;
    #pragma omp critical (load_transfer)
    {
    #pragma omp critical
    //{printf("------------------\n");
    //printf("R%d: T%d sifting through %d threads\n",curr_rep, t_no, cnt_ldd_ts[curr_rep]);}
    //iterate over threads which have not completed their local set
    for (t = 0; t < cnt_ldd_ts[curr_rep]; t++){
      threadt = ldd_tarr[t];
      #pragma omp atomic read
      checkThreadRng = thread_rng[threadt];
      #pragma omp atomic read
      val = itr_left[threadt];
      // maxloaded
      if (val > max_val){
        max_val = val;
        ldd_t = threadt;
        ldd_tarr[lt] = threadt;
        lt++;
      }
      // loaded but not the max value right now
      else if (val > P - 1){
        ldd_tarr[lt] = threadt;
        lt++;
      }
      // Thread has not started solving it default segment in curr_rep
      else if (checkThreadRng < curr_rep + 1){ //Begin unloading non assigned segment
        //printf("T%d acquiring lock for T%d\n",t_no, threadt);
        cnt_nt_strt++;
        omp_set_lock(&(lock_itr_left[threadt]));
        //check if any change to value of checkThreadRng and if segment is being worked on
        if ((thread_rng[threadt] == checkThreadRng) && (is_assigned[threadt] == 0)){
          //#pragma omp critical (printf_lock)
          //{printf("R%d: T%d unloaded\n", curr_rep, threadt);}
          itr = (int) (d_seg_sz[threadt]/tf);
          //#pragma omp critical (printf_lock)
          //{printf("R%d: T%d iterations left %d\n", curr_rep, threadt, itr);}
          //printf("T%d deacquiring lock for T%d\n",t_no, threadt);
          if (itr > 0){
            // perform load transfer internally
            //t_itr_end[t_no] = (threadt*seg_sz) + d_seg_sz[threadt];
            d_seg_sz[threadt] -= itr;
            lb = (threadt*seg_sz) + d_seg_sz[threadt];
            omp_unset_lock(&(lock_itr_left[threadt]));
            seg = P;
            //break out of loop
            lt += cnt_ldd_ts[curr_rep] - t;
            ldd_t = t_no;
            //#pragma omp critical (printf_lock)
            //{printf("R%d: T%d acquired %d iterations from T%d - %d - %d\n", curr_rep, t_no, itr, threadt,  lb, lb + itr);}
            //omp_set_lock(&(lock_itr_left[t_no]));
            break;
          }
        }
        // If loop not broken
        omp_unset_lock(&(lock_itr_left[threadt]));
        ldd_tarr[lt] = threadt;
        lt++;
      } // END UNLOADING threads segment
    }
    cnt_ldd_ts[curr_rep] = lt; //reset number of loaded threads
    //transfer load if new thread found else do nothing ?
    if (ldd_t != t_no){
      omp_set_lock(&(lock_itr_left[ldd_t]));
      itr = (int) (itr_left[ldd_t]/tf);
      itr_left[ldd_t] -= itr;
      omp_unset_lock(&(lock_itr_left[ldd_t]));
      //load is transfered only from threads in their original segment
      seg = ldd_t;
      #pragma omp atomic update
      is_assigned[seg]++;
      // uncomment for DEBUG
      //#pragma omp critical (printf_lock)
      //{printf("Reassign %d itrs from T%d to T%d ....\n",itr, ldd_t, t_no);}

      /*t_itr_end[t_no] = t_itr_end[ldd_t];
      t_itr_end[ldd_t] -= itr;
      itr_left[t_no] = itr;
      lb = t_itr_end[t_no] - itr;*/

      //ub = t_itr_end[ldd_t];
      t_itr_end[ldd_t] -= itr;
      //itr_left[t_no] = itr;
      lb = t_itr_end[ldd_t];

      //omp_set_lock(&(lock_itr_left[t_no]));
    }
    // same thread :> no itr but wait
    /*else if (cnt_nt_strt > 0){
      omp_set_lock(&(lock_itr_left[t_no]));
    }*/
    // Same thread, no unstarted, no work ==> stop

    //#pragma omp critical (printf_lock)
    //{printf("R%d: T%d: Number of iterations: %d, threads running: %d, loaded thread: %d\n",curr_rep,t_no, itr, lt, ldd_t);}
    } //END LOAD TRANSFER
    ub = lb + itr;
    //chnk_sz = (itr < P)? 1: (int) (itr/P);
    if (itr > 0){
      //something to evaluate
      //loop1bounds(lb, ub);
      for (; lb < ub; lb++){
        for (j=N-1; j>lb; j--){
          a[lb][j] += cos(b[lb][j]);
        }
      }
      max_val = tf;
      itr = 0;
      // go on to next iteration
      // set lock to prevent seg fault while unsetting lock
      omp_set_lock(&(lock_itr_left[t_no]));
    }
    // No threads pending and no iterations
    else if (cnt_nt_strt == 0){
      // end the loop
      max_val = 0;
    }
    else{
      // threads havent started but current threads have evaluated what they could
      omp_set_lock(&(lock_itr_left[t_no]));
    }
    // Uncomment for DEBUG
    //#pragma omp critical (printf_lock)
    //{printf(" eval segment %d. bounds %d - %d\n", seg, lb, lb + itr);}
  }
  omp_set_lock(&lock_ut);
  ut--;
  omp_unset_lock(&lock_ut);
  //reset ldd_tarr
  ldd_tarr[ut] = t_no;
  if (ut == 0){
    //reset ut
    ut = P;
  }
  /*if (omp_test_lock(&(lock_itr_left[t_no]))){
    omp_unset_lock(&(lock_itr_left[t_no]));
  }
  else{
    omp_unset_lock(&(lock_itr_left[t_no]));
  }*/
  //#pragma omp barrier
}

void loop1bounds(int lowerBound, int upperBound){
  int i, j;
  for (i = lowerBound; i < upperBound; i++){
    for (j=N-1; j>i; j--){
      a[i][j] += cos(b[i][j]);
    }
  }
}

void loop2(void) {
  int i,j,k;
  double rN2;

  rN2 = 1.0 / (double) (N*N);

  for (i=0; i<N; i++){
    for (j=0; j < jmax[i]; j++){
      for (k=0; k<j; k++){
	c[i] += (k+1) * log (b[i][j]) * rN2;
      }
    }
  }

}

void valid1(void) {
  int i,j;
  double suma;

  suma= 0.0;
  for (i=0; i<N; i++){
    for (j=0; j<N; j++){
      suma += a[i][j];
    }
  }
  printf("Loop 1 check: Sum of a is %lf\n", suma);

}


void valid2(void) {
  int i;
  double sumc;

  sumc= 0.0;
  for (i=0; i<N; i++){
    sumc += c[i];
  }
  printf("Loop 2 check: Sum of c is %f\n", sumc);
}

void debug_race(void){
  int i,seg, ub;
  printf("running debug... \n");
  for (seg = 0; seg < P; seg++){
    printf("Segment %d... \n", seg);
    ub = (seg_sz + 1) * seg;
    if (seg == P - 1){
      ub = N;
    }
    for (i = seg_sz * seg; i < ub ; i++){
      if (itr_prf[i] > reps){
        printf("At %d th value a race is occuring, excess it: %d\n", i, itr_prf[i] - reps);
      }
      else if (itr_prf[i] < reps){
        printf("At %d th value insufficient iterations have been executed, lacking: %d\n", i, reps-itr_prf[i]);
      }
    }
    printf("\n");
  }
}
