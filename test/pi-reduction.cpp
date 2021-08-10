#include <stdio.h>
#include <assert.h>
//#include <math.h> // fabs // confict with AMD compiler
#include <stdlib.h>
#include <algorithm>
#include <iostream>

double fabs (double a)
{
  if (a<0)
    return -a;
  return a;
}
#include <omp.h>

#define TOLERANCE 1.0e-5
#define num_steps 2000000

#ifdef _OPENMP
void hasGPU()
{
  int runningOnGPU = 0;

  std::cout<<"The number of target devices ="<< omp_get_num_devices() <<std::endl;
  /* Test if GPU is available using OpenMP4.5 */
#pragma omp target map(from:runningOnGPU)
  {
    // This function returns true if currently running on the host device, false otherwise.
    if (!omp_is_initial_device())
      runningOnGPU = 1;
  }
  /* If still running on CPU, GPU must not be available */
  if (runningOnGPU == 1)
    std::cout<<"### Success: able to use the GPU! ### \n"<<std::endl;
  else
    std::cout<<"### Failure: unable to use the GPU, using CPU! ###\n"<< std::endl;
}
#endif

void RelativeDiff(double a, double b)
{
  double fabs_diff = fabs(a-b); // must use fabs, not abs(INTEGER)
  double epsilon_b = TOLERANCE * fabs (b);
  std::cout<<"fabs_diff="<<fabs_diff<< " epsilon*abs(b)="<<epsilon_b   << std::endl; 
  assert (fabs_diff<=epsilon_b);
}


void worker(int i, double interval_width, double& pi)
{
  double x = (i+ 0.5) * interval_width;
  pi += 1.0 / (x*x + 1.0);
}

int main(int argc, char** argv)
{
  double pi = 0.0;
  int i;
  double x, interval_width;
  interval_width = 1.0/(double)num_steps;

#ifdef _OPENMP
  hasGPU();
#endif  

#pragma omp target teams map (tofrom:pi)
#pragma distributed parallel for reduction(+:pi) private(x)
  for (i = 0; i < num_steps; i++) {
#if 0    
    x = (i+ 0.5) * interval_width;
    pi += 1.0 / (x*x + 1.0);
#else // wrap into a function
    worker (i, interval_width, pi);
#endif    
  }

  pi = pi * 4.0 * interval_width;

  std::cout<<"pi="<< pi <<std::endl;
  printf ("pi=%f\n", pi);
  RelativeDiff (pi,3.141593);
  return 0;
}
