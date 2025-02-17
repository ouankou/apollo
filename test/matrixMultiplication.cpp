/*
   Naive matrix-matrix multiplication(mmm)
   c = a * b

a: N*M
b: M*K
c: N*K

By C. Liao
*/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <apollo/Apollo.h>
#include <apollo/Region.h>

#define REAL float 

REAL *alloc2D(int m, int n) // m row, n elements each
{
  // the linearized array elements storage for 2-D
  REAL *m_buffer= (REAL*)malloc(sizeof(REAL)* m * n);
  if (m_buffer== NULL)
  {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  return m_buffer;
}
void init(REAL* a, REAL* b, REAL* c1, REAL* c2, int N, int M, int K)
{
  int i,j;
#pragma omp parallel for collapse(2)  
  for (i=0;i<N;i++)
    for(j=0;j<M;j++)
      a[i*M+j]=3.0*i*j/N/M;

#pragma omp parallel for collapse(2)  
  for (i=0;i<M;i++)
    for(j=0;j<K;j++)
      b[i*K+j]=5.0*j*i/N/M;

#pragma omp parallel for collapse(2)  
  for (i=0;i<N;i++)
    for(j=0;j<K;j++)
    {
      c1[i*K+j]=0.0;
      c2[i*K+j]=0.0;
    }
}

/*
TODO: try different i,j,k orders

a b     e f    a*e+ b*g , a*f+ b*h
c d  x  g h  = c*e+ d*g,  c*f+ d*h

*/

int mm_gpu(REAL* a, REAL* b, REAL* c, int N, int M, int K)
{
  int i,j,k;
#pragma omp target map(tofrom:c[0:N*K]), map(to:a[0:N*M],b[0:M*K])
#pragma omp teams distribute parallel for private(i,j,k) collapse(2)
  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      for (k = 0; k < K; k++)
        c[i*K+j]= c[i*K+j]+a[i*M+k]*b[k*K+j];

  return 0;
}

int mm_cpu(REAL* a, REAL* b, REAL* c,int N, int M, int K)
{
  int i,j,k;
#pragma omp parallel for private(i,j,k) collapse (2)
  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      for (k = 0; k < K; k++)
        c[i*K+j]= c[i*K+j]+a[i*M+k]*b[k*K+j];

  return 0;
}


int verify(REAL* c1, REAL* c2, int N, int K)
{
  int i,j;
  REAL sum1=0.0, sum2=0.0;
#pragma omp parallel for reduction(+:sum1, sum2) collapse (2)
  for (i=0;i<N;i++)
    for(j=0;j<K;j++)
    {
      sum1+=c1[i*K+j];
      sum2+=c2[i*K+j];
    }
  printf("sum of c1[i][j] is %f\n",sum1);
  printf("sum of c2[i][j] is %f\n",sum2);
  assert (sum1 == sum2);
  return 0;
}

int main(int argc, char* argv[])
{
  int N, M, K;
  if (argc !=2)
  {
    printf("Usage: %s array_size\n", argv[0]);
    return 0;
  }

#pragma omp parallel 
    {
#pragma omp master          
      {
        int thread_count = omp_get_num_threads();
        printf ("Using %d out of max %d threads...\n", thread_count, omp_get_max_threads());
      }
    }
  // test if GPU offloading is supported
  int runningOnGPU = 0;

  /* Test if GPU is available using OpenMP4.5 */
#pragma omp target map(from:runningOnGPU)
  {
    // This function returns true if currently running on the host device, false otherwise.
    if (!omp_is_initial_device())
      runningOnGPU = 1;
  }
  assert (runningOnGPU == 1);
 
  N=M=K=atoi(argv[1]);
  printf("matrix size : %d\n", N);

  // linearized 2-D arrays
  REAL* a  = alloc2D(N,M);
  REAL* b  = alloc2D(M,K);
  REAL* c1 = alloc2D(N,K);
  REAL* c2 = alloc2D(N,K);

  init(a, b, c1, c2, N, M, K);


  bool b_cpu=false, b_gpu=false;
  // original code does not have an iteration enclosing two choices
  // round robin policy may not work, using random is better, plus the execution drivers repeating the executions.
  // Or the driver specify the policies!
  // Another choice: repeat it anyway by adding a loop
  // for smaller data sizes, we repeat mores to counter system noises
  int limit= 4;
  if (N<2000) 
    limit = 20;
  for (int repeat=0; repeat<limit; repeat++)
  {
    // Create Apollo region if needed
    Apollo::Region *region = Apollo::instance()->getRegion(
        /* id */ "mm",
        /* NumFeatures */ 1,
        /* NumPolicies */ 2);

    region->begin( /* feature vector */ { (float)N } );

    int policy = region->getPolicyIndex();

    switch (policy)
    {
      case 0: 
        {
          mm_cpu(a,b,c1, N, M, K); 
          b_cpu = true; 
          break;
        }
      case 1: 
        {
          mm_gpu(a,b,c2, N, M, K); 
          b_gpu = true;
          break;
        }

      default: assert ("Invalid policy\n");
    }  
    region->end();
  }

  
// It is possible only one version is executed in this adaptive version.
  if (b_cpu && b_gpu)
    verify(c1,c2, N, K);

  free(a);
  free(b);
  free(c1);
  free(c2);

  return 0;
}

