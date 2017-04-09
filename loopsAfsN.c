#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define N 729
#define reps 100
#include <omp.h>

double a[N][N], b[N][N], c[N];
int jmax[N];
int P; //number of threads
int tf; //transfer factor
int seg_sz; //iterations allocated to each thread except last
int* itr_left; //iterations left in each thread
int* d_seg_sz; // seg assigned to each thread
int* t_itr_end; // end of segments
int* ldd_tarr; //array of loaded threads
int* thread_rng; //indicate thread is running
int cnt_ldd_ts; //number of loaded threads
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

int main(int argc, char *argv[]) {
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
  cnt_ldd_ts = P;
  ut = P;
  seg_sz = (int) (N/P);
  d_seg_sz = (int *) malloc(sizeof(int) * P);
  lock_itr_left = (omp_lock_t*)malloc(sizeof(omp_lock_t) * P);

  //Variable values
  itr_left = (int *) malloc(sizeof(int) * P);
  t_itr_end = (int *) malloc(sizeof(int) * P);
  ldd_tarr = (int *) malloc(sizeof(int) * P);
  thread_rng = (int *) malloc(sizeof(int) * P);

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
  }
  d_seg_sz[P-1]+= N - (seg_sz*P);

  init1();
  //printf("iterations allocated to each thread: %d\n", seg_sz);

  start1 = omp_get_wtime();

  #pragma omp parallel default(none) \
  shared(P, tf, seg_sz,itr_left,d_seg_sz, t_itr_end) \
  shared(ldd_tarr, cnt_ldd_ts, ut, lock_ut, thread_rng) private(r)
  {
  for (r=0; r<reps; r++){
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

  /*
  default initial segment is
  corresponds to thread number
  */
  t_no = omp_get_thread_num();

  //set iterations
  itr = d_seg_sz[t_no];
  /*
  no lock needed because
  load transfer thread will
  change its end only if it has iterations
  currently iter_left[t_no] = 0
  */
  t_itr_end[t_no] = (t_no+1)*seg_sz;
  if (t_no == P - 1){
    //itr += N - (seg_sz * P);
    t_itr_end[t_no] = N;
  }

  max_val = itr;

  lb = t_no * seg_sz;
  chnk_sz = (int) (itr/P);

  omp_set_lock(&(lock_itr_left[t_no]));
  itr_left[t_no] = itr;

  #pragma omp atomic update
  thread_rng[t_no]++;
  // DEBUG
  //#pragma omp critical (printf_lock)
  //{printf("Thread %d start iterating from %d\n",t_no, lb);}
  while (max_val > tf){
    while (itr > 0){
      itr_left[t_no] -= chnk_sz;
      omp_unset_lock(&(lock_itr_left[t_no]));
      ub = lb + chnk_sz;
      /*//Uncomment for DEBUG segmentation fault
      if (((seg*seg_sz > lb || lb > (seg+1)*seg_sz) && (seg != P - 1)) ||
         ((seg*seg_sz > lb || lb > N) && (seg == P - 1)))
      {
        #pragma omp critical (lock_printf)
        {printf(" lb = %d is out of bounds of seg %d | THREAD NO %d\n",lb , seg, t_no);}
      }
      if (((seg*seg_sz > ub || ub > (seg+1)*seg_sz) && (seg != P - 1)) ||
         ((seg*seg_sz > ub || ub > N) && (seg == P - 1)))
      {
        #pragma omp critical (lock_printf)
        {printf(" ub = %d is out of bounds of seg %d | THREAD NO %d\n",ub , seg,t_no);}
      }
      // else{
      //   #pragma omp critical (lock_printf)
      //   {printf("Bounds okay\n");}
      // }
      */

      for (;lb < ub; lb++){
        //#pragma omp atomic update //DEBUG
        //itr_prf[i]++;             //DEBUG
        for (j=N-1; j>lb; j--){
          a[lb][j] += cos(b[lb][j]);
        }
      }

      omp_set_lock(&(lock_itr_left[t_no]));
      itr = itr_left[t_no];
      // New chunk size
      chnk_sz = (itr < P)? 1: (int) (itr/P);
    } // Ran out of iterations in assigned segment

    omp_unset_lock(&(lock_itr_left[t_no]));
   // Load transfer block //
    max_val = tf;
    ldd_t = t_no;
    lt = 0;
    #pragma omp critical (load_transfer)
    {
    // Total number of threads - loaded threads
    //iterate to get loaded thread
    for (t = 0; t < cnt_ldd_ts; t++){
      threadt = ldd_tarr[t];
      #pragma omp atomic read
      checkThreadRng = thread_rng[threadt];
      //omp_set_lock(&(lock_itr_left[t]));
      #pragma omp atomic read
      val = itr_left[threadt];
      //omp_unset_lock(&(lock_itr_left[t]));

      if (val > max_val){
        max_val = val;
        ldd_t = threadt;
        ldd_tarr[lt] = threadt;
        lt++;
      }
      //val greater than or equal to P or thread hasnt started yet
      else if ((val > P - 1) || (checkThreadRng < (r + 1))){
        ldd_tarr[lt] = threadt;
        lt++;
      }
    }
    cnt_ldd_ts = lt; //reset number of loaded threads
    //transfer load if new thread found else do nothing ?
    if (ldd_t != t_no){
      omp_set_lock(&(lock_itr_left[ldd_t]));
      itr = (int) (itr_left[ldd_t]/tf);
      itr_left[ldd_t] -= itr;
      omp_unset_lock(&(lock_itr_left[ldd_t]));
      // uncomment for DEBUG
      //#pragma omp critical (printf_lock)
      //{printf("Reassign %d itrs from T%d to T%d ....",itr, ldd_t, t_no);}

      t_itr_end[t_no] = t_itr_end[ldd_t];
      t_itr_end[ldd_t] -= itr;

      itr_left[t_no] = itr;
      lb = t_itr_end[t_no] - itr;
      omp_set_lock(&(lock_itr_left[t_no]));
    }
    } //END LOAD TRANSFER

    chnk_sz = (itr < P)? 1: (int) (itr/P);
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
    cnt_ldd_ts = P;
  }
  if (omp_test_lock(&(lock_itr_left[t_no]))){
    omp_unset_lock(&(lock_itr_left[t_no]));
  }
  else{
    omp_unset_lock(&(lock_itr_left[t_no]));
  }
  //#pragma omp barrier
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
