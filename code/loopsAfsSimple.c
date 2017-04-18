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
int* itrLeftArr; //iterations left in each thread
int* d_seg_sz; // seg assigned to each thread
int* localSetEnd; // end of segments
int* ldd_tarr; //array of loaded threads
int* default_ldd_tarr;
int* getThread;

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
omp_lock_t* lock_itrLeftArr;

void init1(void);
void init2(void);
void loop1(void);
void loop2(void);
void valid1(void);
void valid2(void);
void debug_race(void);

int main(int argc, char *argv[]) {
  //int r;
  double start1,start2,end1,end2;
  int t, lk, i;
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
  for (i =0; i < reps; i++){
    cnt_ldd_ts[i] = P;
  }
  ut = P;
  seg_sz = (int) (N/P);
  seg_szP = seg_sz + N - (seg_sz * P);
  lock_itrLeftArr = (omp_lock_t*)malloc(sizeof(omp_lock_t) * (P + 1));

  //Variable values
  d_seg_sz = (int *) malloc(sizeof(int) * P);
  itrLeftArr = (int *) malloc(sizeof(int) * P);
  localSetEnd = (int *) malloc(sizeof(int) * P);
  ldd_tarr = (int *) malloc(sizeof(int) * P);
  default_ldd_tarr = (int *) malloc(sizeof(int) * P);
  thread_rng = (int *) malloc(sizeof(int) * P);
  is_assigned = (int *) malloc(sizeof(int) * P);

  if ((itrLeftArr == NULL) || (d_seg_sz == NULL) || (lock_itrLeftArr == NULL) || (localSetEnd == NULL) ) {
    printf("FATAL: Malloc failed ... \n");
  }
  else{
    printf("Allocation: OKAY\n");
  }
  //Initialise lock
  omp_init_lock(&lock_ut);
  for (lk = 0; lk < P; lk++){
    omp_init_lock(&(lock_itrLeftArr[lk]));
    itrLeftArr[lk] = 0;
    d_seg_sz[lk] = seg_sz;
    ldd_tarr[lk] = lk; //at the start all threads are loaded
    default_ldd_tarr[lk] = lk;
    thread_rng[lk] = 0;
    localSetEnd[lk] = (lk + 1) * seg_sz;
    is_assigned[lk] = 0;
  }
  getThread = default_ldd_tarr;
  localSetEnd[P-1] = N;
  is_assigned[P] = 0;
  d_seg_sz[P-1]+= N - (seg_sz*P);
  //localSetEnd[P-1] = N;

  init1();
  //printf("iterations allocated to each thread: %d\n", seg_sz);

  start1 = omp_get_wtime();

  #pragma omp parallel default(none) \
  shared(P, tf, seg_sz,seg_szP, itrLeftArr,d_seg_sz, localSetEnd) \
  shared(ldd_tarr, ut, lock_ut, thread_rng, is_assigned, default_ldd_tarr, getThread) private(r)
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
  for (i =0; i < reps; i++){
    cnt_ldd_ts[i] = P;
  }
  getThread = default_ldd_tarr;
  for (t = 0; t < P; t++){
    thread_rng[t] = 0;
  }
  init2();
  start2 = omp_get_wtime();
  #pragma omp parallel default(none) \
  shared(P, tf, seg_sz,seg_szP, itrLeftArr,d_seg_sz, localSetEnd) \
  shared(ldd_tarr, ut, lock_ut, thread_rng, is_assigned, default_ldd_tarr, getThread) private(r)
  {
  for (r=0; r<reps; r++){
    loop2();
  }
  }
  end2  = omp_get_wtime();

  valid2();

  printf("Total time for %d reps of loop 2 = %f\n",reps, (float)(end2-start2));
  //debug_race();
  free(d_seg_sz);
  free(itrLeftArr);
  free(lock_itrLeftArr);
  free(localSetEnd);
  free(ldd_tarr);
  free(default_ldd_tarr);
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
  //maximum load, load, loaded thread
  int max_val, val, ldd_t;
  int itr, t_no;
  /*
  default initial segment is
  corresponds to thread number
  */
  t_no = omp_get_thread_num();
  //set iterations
  max_val = tf;
  lb = t_no * seg_sz;
  omp_set_lock(&(lock_itrLeftArr[t_no]));
  itr = (t_no == P - 1)? seg_szP: seg_sz;
  itrLeftArr[t_no] = itr;
  localSetEnd[t_no] = (t_no * seg_sz) + itr;
  while (max_val > 0){
    while (itr > 0){
      chnk_sz = (itr < P)? 1: (int) (itr/P);
      itrLeftArr[t_no] -= chnk_sz;
      omp_unset_lock(&(lock_itrLeftArr[t_no]));
      ub = lb + chnk_sz;
      for (;lb < ub; lb++){
        //pragma omp atomic update //DEBUG
        //itr_prf[lb]++;             //DEBUG
        for (j=N-1; j>lb; j--){
          a[lb][j] += cos(b[lb][j]);
        }
      }
      omp_set_lock(&(lock_itrLeftArr[t_no]));
      itr = itrLeftArr[t_no];
      // New chunk size
    } // Ran out of iterations in assigned segment
    //indicate to all threads that one less thread is running in seg
    omp_unset_lock(&(lock_itrLeftArr[t_no]));

   // Load transfer block //
    max_val = tf;
    ldd_t = t_no;
    #pragma omp critical (load_transfer)
    {
    for (t = 0; t < P; t++){
      #pragma omp atomic read
      val = itrLeftArr[t];
      if (val > max_val){
        max_val = val;
        ldd_t = t;
      }
    }
    //transfer load if new thread found else do nothing ?
    if (ldd_t != t_no){
      omp_set_lock(&(lock_itrLeftArr[ldd_t]));
      itr = (int) (itrLeftArr[ldd_t]/tf);
      itrLeftArr[ldd_t] -= itr;
      omp_unset_lock(&(lock_itrLeftArr[ldd_t]));
      localSetEnd[t_no] = localSetEnd[ldd_t];
      localSetEnd[ldd_t] -= itr;
      itrLeftArr[t_no] = itr;
      // Get starting value
      #pragma omp atomic read
      lb = localSetEnd[ldd_t];
    }
    } //END LOAD TRANSFER
    if (ldd_t == t_no) {
      max_val = 0;
    }

    else {
      omp_set_lock(&(lock_itrLeftArr[t_no]));
    }
  }
  #pragma omp barrier
}



void loop2(void) {
  int i,j,k;
  double rN2 = 1.0 / (double) (N*N);;
  int t, lb;
  int chnk_sz, ub;
  //maximum load, load, loaded thread
  int max_val, val, ldd_t;
  int itr, t_no;
  /*
  default initial segment is
  corresponds to thread number
  */
  t_no = omp_get_thread_num();
  //set iterations
  max_val = tf;
  lb = t_no * seg_sz;

  omp_set_lock(&(lock_itrLeftArr[t_no]));
  itr = (t_no == P - 1)? seg_szP: seg_sz;
  itrLeftArr[t_no] = itr;
  localSetEnd[t_no] = (t_no * seg_sz) + itr;
  while (max_val > 0){
    while (itr > 0){
      chnk_sz = (itr < P)? 1: (int) (itr/P);
      itrLeftArr[t_no] -= chnk_sz;
      omp_unset_lock(&(lock_itrLeftArr[t_no]));
      ub = lb + chnk_sz;
      for (; lb<ub; lb++){
        for (j=0; j < jmax[lb]; j++){
          for (k=0; k<j; k++){
            c[lb] += (k+1) * log (b[lb][j]) * rN2;
          }
        }
      }
      omp_set_lock(&(lock_itrLeftArr[t_no]));
      itr = itrLeftArr[t_no];
      // New chunk size
    } // Ran out of iterations in assigned segment

    //indicate to all threads that one less thread is running in seg
    omp_unset_lock(&(lock_itrLeftArr[t_no]));
   // Load transfer block //
    max_val = tf;
    ldd_t = t_no;
    #pragma omp critical (load_transfer)
    {
    for (t = 0; t < P; t++){
      #pragma omp atomic read
      val = itrLeftArr[t];
      if (val > max_val){
        max_val = val;
        ldd_t = t;
      }
    }
    //transfer load if new thread found else do nothing ?
    if (ldd_t != t_no){
      omp_set_lock(&(lock_itrLeftArr[ldd_t]));
      itr = (int) (itrLeftArr[ldd_t]/tf);
      itrLeftArr[ldd_t] -= itr;
      omp_unset_lock(&(lock_itrLeftArr[ldd_t]));
      localSetEnd[t_no] = localSetEnd[ldd_t];
      localSetEnd[ldd_t] -= itr;
      itrLeftArr[t_no] = itr;
      // Get starting value
      #pragma omp atomic read
      lb = localSetEnd[ldd_t];
    }
    } //END LOAD TRANSFER
    if (ldd_t == t_no) {
      max_val = 0;
    }
    else{
      omp_set_lock(&(lock_itrLeftArr[t_no]));
    }
  }
  #pragma omp barrier
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
