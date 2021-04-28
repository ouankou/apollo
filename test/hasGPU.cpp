#include <stdio.h>
#include <iostream>
#ifdef _OPENMP
#include <omp.h>
#else
#if 0
 int omp_get_num_threads()
{
return 1;
}

int omp_get_max_threads()
{
  return 1;
}

int omp_get_num_devices ()
{
  return 0;
}

int omp_is_initial_device()
{
  return 1;
}
#endif
#endif

using namespace std;

int main()
{
#pragma omp parallel 
    {
#pragma omp master          
      { 
        int thread_count = omp_get_num_threads();
        printf ("Using %d out of max %d threads...\n", thread_count, omp_get_max_threads());
      }
    }

  int runningOnGPU = 0;

  printf ("The number of target devices =%d\n", omp_get_num_devices());
  /* Test if GPU is available using OpenMP4.5 */
#pragma omp target map(from:runningOnGPU)
  {
    // This function returns true if currently running on the host device, false otherwise.
    if (!omp_is_initial_device())
      runningOnGPU = 1;
  }
  /* If still running on CPU, GPU must not be available */
  if (runningOnGPU == 1)
    cout<<"### Success: able to use the GPU! ### \n"<<endl;
  else
    cout<<"### Failure: Unable to use the GPU, using CPU! ###\n"<< endl;

  return 0;
}

