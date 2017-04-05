#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define N 729
#define reps 100
#include <omp.h>

double a[N][N], b[N][N], c[N];
int jmax[N];
int P; //number of threads
int seg_sz; //iterations allocated to each thread except last
int* itr_left; //iterations left in each thread
int* seg_asgn; // seg assigned to each thread
//int tot_itr = N; //total iterations left
int t_no;
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
  int r, t, lk;
  #pragma omp parallel default(none) \
  shared(P)
  {
    #pragma omp single
    {
      P = omp_get_num_threads();
      max_threads = omp_get_max_threads();
    }
  }

  printf("number of threads: %d\n", P);

  seg_sz = (int) (N/P);
  itr_left = (int *) malloc(sizeof(int) * P);
  seg_asgn = (int *) malloc(sizeof(int) * P);
  lock_itr_left = (omp_lock_t*)malloc(sizeof(omp_lock_t) * P);
  if ((itr_left == NULL) || (seg_asgn == NULL) || (lock_itr_left == NULL)) {
    printf("FATAL: Malloc failed ... \n");
  }
  else{
    printf("Allocation: OKAY\n");
  }
  //Initialise lock
  for (lk = 0; lk < P; lk++){
    omp_init_lock(&(lock_itr_left[lk]));
    itr_left[lk] = 0;
    seg_asgn[lk] = lk;
  }
  init1();
  //printf("iterations allocated to each thread: %d\n", seg_sz);

  start1 = omp_get_wtime();

  // #pragma omp parallel default(none) \
  // shared(r, itr_left, seg_asgn, P, seg_sz, tot_itr) \
  // private (t_no)
  // {
  // t_no = omp_get_thread_num();
  for (r=0; r<reps; r++){
    loop1();
  // }
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
  free(seg_asgn);
  free(itr_left);
  free(lock_itr_left);
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
  int rsv, chnk_sz, ub;
  //bool run_flg;
  //maximum load, load, loaded thread
  int max_val, val, ldd_t;
  int seg, itr;
  // for (int lk = 0; lk < P; lk++){
  //   omp_set_lock(&(lock_seg_asgn[lk]));
  // }
  #pragma omp parallel default(none) \
  shared(itr_left, seg_asgn, P, seg_sz,a, b, itr_prf, lock_itr_left) \
  private (t_no, rsv, chnk_sz, ub, i ,j ,t, seg, itr) \
  private(lb, max_val, val, ldd_t)
  {
  /* default initial segment is
     corresponds to thread number
  */
  t_no = omp_get_thread_num();

  seg = t_no;

  #pragma atomic write
  seg_asgn[t_no] = t_no;
  //omp_unset_lock(&(lock_seg_asgn[t_no]));
  //set iterations
  itr = seg_sz;
  if (t_no == P - 1){
    itr += N - (seg_sz * P);
  }

  max_val = itr;

  lb = seg * seg_sz;
  rsv = chnk_sz = (int) (itr/P);

  omp_set_lock(&(lock_itr_left[t_no]));
  itr_left[t_no] = itr - rsv;
  omp_unset_lock(&(lock_itr_left[t_no]));
  // #pragma omp critical
  // {
  //   printf("itr from thread %d: %d\n",t_no, itr);
  // }
  while (max_val > 0){
    while (itr > 0){
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
      omp_set_lock(&(lock_itr_left[t_no]));
      itr_left[t_no] -= chnk_sz;
      itr = itr_left[t_no];
      omp_unset_lock(&(lock_itr_left[t_no]));

      for (i = lb ;i < ub; i++){
        //#pragma omp atomic update //DEBUG
        //itr_prf[i]++;             //DEBUG
        for (j=N-1; j>i; j--){
          a[i][j] += cos(b[i][j]);
        }
      }
      lb = i;

      itr += rsv;
      // New chunk size
      chnk_sz = (int) (itr/P);
      if (chnk_sz < 1){
        chnk_sz = 1;
      }
    } // Ran out of iterations in assigned segment

   // Load transfer block //
    #pragma omp critical (load_transfer)
    {
    max_val = 0;
    ldd_t = t_no;
    //iterate to get loaded thread
    for (t = 0; t < P; t++){
      omp_set_lock(&(lock_itr_left[t]));
      val = itr_left[t];
      omp_unset_lock(&(lock_itr_left[t]));

      if (val > max_val){
        max_val = val;
        ldd_t = t;
      }
    }
    //transfer laod

    omp_set_lock(&(lock_itr_left[ldd_t]));
    itr = itr_left[ldd_t];
    itr_left[ldd_t] = 0;
    omp_unset_lock(&(lock_itr_left[ldd_t]));
    // uncomment for DEBUG
    //#pragma omp critical (printf_lock)
    //{printf("T%d has %d itrs left (Org value = %d) Reassign to T%d ....",ldd_t, itr, max_val, t_no);}
    //get segment

    //omp_set_lock(&(lock_seg_asgn[ldd_t]));
    seg_asgn[t_no] = seg_asgn[ldd_t];
    //omp_unset_lock(&(lock_seg_asgn[ldd_t]));

    rsv =  chnk_sz = (int) (itr/P);
    if (itr < P){
      rsv = chnk_sz = 1;
    }
    //Thread says it has completed 'rsv' but hasnt actually
    //omp_set_lock(&(lock_itr_left[t_no]));
    itr_left[t_no] = itr - rsv;
    //omp_unset_lock(&(lock_itr_left[t_no]));

    } //END LOAD TRANSFER

    //if Last thread, segment end = N
    seg = seg_asgn[t_no];
    lb = (seg == P - 1)? N - itr: ((seg + 1)*seg_sz) - itr;
    // Uncomment for DEBUG
    //#pragma omp critical (printf_lock)
    //{printf(" eval segment %d. bounds %d - %d\n", seg, lb, (seg+1) * seg_sz);}
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
