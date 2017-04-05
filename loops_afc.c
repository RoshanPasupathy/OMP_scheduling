#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define N 729
#define reps 100
#include <omp.h>

double a[N][N], b[N][N], c[N];
int jmax[N];
int P; //number of threads
int th_alloc; //iterations allocated to each thread except last
int* itr_left; //iterations left in each thread
int* seg_asgn; // seg assigned to each thread
int tot_itr = N; //total iterations left
int t_no;

void init1(void);
void init2(void);
void loop1(void);
void loop2(void);
void valid1(void);
void valid2(void);


int main(int argc, char *argv[]) {

  double start1,start2,end1,end2;
  int r, t;

  #pragma omp parallel default(none) \
  shared(P)
  {
    #pragma omp single
    {
      P = omp_get_num_threads();
    }
  }

  th_alloc = (int) (N/P);
  itr_left = (int *) malloc(sizeof(int) * P);
  seg_asgn = (int *) malloc(sizeof(int) * P);
  init1();
  //printf("iterations allocated to each thread: %d\n", th_alloc);

  start1 = omp_get_wtime();

  // #pragma omp parallel default(none) \
  // shared(r, itr_left, seg_asgn, P, th_alloc, tot_itr) \
  // private (t_no)
  // {
  // t_no = omp_get_thread_num();
  for (r=0; r<reps; r++){
    // #pragma omp parallel default(none) \
    // shared(r, itr_left, seg_asgn, P, th_alloc, tot_itr) \
    // private (t_no)
    // {
    // t_no = omp_get_thread_num();
    // #pragma omp atomic write
    // seg_asgn[t_no] = t_no;
    // if (t_no == P - 1){
    //   //not exactly needed
    //   #pragma omp atomic write
    //   itr_left[t_no] = th_alloc;
    // }
    // else{
    //   // excess values go to last thread
    //   #pragma omp atomic write
    //   itr_left[t_no] = th_alloc + (N - (th_alloc * P));
    // }
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
  free(seg_asgn);
  free(itr_left);

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
  int i,j,t, listhead;
  int pad, chnk_sz, chnk_end;
  //bool run_flg;
  //maximum load, load, loaded thread
  int max_val, val, ldd_t;
  int seg, itr;
  #pragma omp parallel default(none) \
  shared(itr_left, seg_asgn, P, th_alloc, tot_itr,a, b) \
  private (t_no, pad, chnk_sz, chnk_end, i ,j ,t, seg, itr) \
  private(listhead, max_val, val, ldd_t)
  {
  /* default initial segment is
     corresponds to thread number
  */
  t_no = omp_get_thread_num();
  seg = t_no;
  itr = th_alloc;
  if (t_no == P - 1){
    itr += N - (th_alloc * P);
  }
  #pragma omp atomic write
  itr_left[t_no] = itr;

  listhead = seg * th_alloc;
  pad = chnk_sz = (int) (itr/P);
  #pragma omp atomic update
  itr_left[t_no] -= pad;
    //tmp = itr_left[t_no];

  while (tot_itr > 0){
    while (itr > 0){
      //#pragma omp critical
      //{
      //  printf("thread %d executing inner loop\n", t_no);
      //}
      chnk_end = i + chnk_sz;

      #pragma omp atomic update
      itr_left[t_no] -= chnk_sz;
        //tmp = itr_left[t_no];
      for (i = listhead;i < chnk_end; i++){
        for (j=N-1; j>i; j--){
          a[i][j] += cos(b[i][j]);
        }
      }
      listhead = i;

      #pragma omp atomic read
      itr = itr_left[t_no];

      itr += pad;
      // New chunk size
      if ((chnk_sz = (int) (itr/P)) < 1){
        chnk_sz = 1;
      }
    } // Ran out of iterations in assigned segment

    /* Load transfer block */
    #pragma omp critical (load_transfer)
    {
    max_val = 0;
    //iterate to get loaded thread
    for (t = 0; t < P; t++){
      #pragma omp atomic read
      val = itr_left[t];
      tot_itr += val;
      if (val > max_val){
        max_val = val;
        ldd_t = t;
      }
    }
    //transfer laod
    #pragma omp atomic capture
    {itr = itr_left[ldd_t]; itr_left[ldd_t] = 0;}
    //get segment
    #pragma omp atomic read
    seg = seg_asgn[ldd_t];

    itr_left[t_no] = itr;
    }
    pad = chnk_sz = (int) (itr/P);

    #pragma omp atomic update
    itr_left[t_no] -= pad;

    listhead = (seg == P - 1)? N - itr: ((seg + 1)*th_alloc) - itr;
  }
  //#pragma omp barrier
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


