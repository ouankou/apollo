/*
 * A test program to demonstrate cross-execution profling, modeling, and adaptation.
 * Embedding all runtime calls inside the loop body.
 *  This is needed when we only see the body of a loop, without access to its caller sites. 
 * Liao, 3/17/2021
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>

#include <omp.h>

#include <apollo/Apollo.h>
#include <apollo/Region.h>

// repeat 10 times for a given array size
#define ITERS 10 

// Code variant 0, or policy 0
void daxpy_cpu(double A, double *x, double *y, double *r, size_t size)
{
#pragma omp parallel for
  for(size_t i=0; i<size; i++)
    r[i] = A*x[i] + y[i];
}

// Code variant 1, or policy 1
void daxpy_gpu(double A, double *x, double *y, double *r, size_t size)
{
#pragma omp target teams distribute parallel for map(to:A,x[0:size],y[0:size],size) map(from:r[0:size])
  for(size_t i=0; i<size; i++)
    r[i] = A*x[i] + y[i];
}

// Example for the DAXPY kernel tunable on different input sizes 
// for CPU/GPU variants
int main(int argc, char** argv)
{
  if (argc!=2)
  {
    printf ("Usage: %s vector_size\n", argv[0]);
    printf ("Example: %s 10000000\n", argv[0]);

    return 0; 
  }

  size_t size = std::stoi(argv[1]);
  printf ("DAXPY size is set to be %zu\n", size);

  double A = 1.0;
  double *x = new double[size];
  double *y = new double[size];
  double *r = new double[size];

  // Init
  for(int i=0; i<size; i++) {
    x[i] = random() % 2;
    y[i] = random() % 2;
  }

  // Run for ITERS iterations, train after the first TRAIN_ITERS
  for(int j=0; j<ITERS; j++) {
    //Using the user specified size
    size_t test_size = size;

    // Create Apollo region if needed
    Apollo::Region *region = Apollo::instance()->getRegion(
        /* id */ "daxpy",
        /* NumFeatures */ 1,
        /* NumPolicies */ 2);

    // Begin region and set feature vector's value
    // Feature vector has only 1 element for this example
    region->begin( /* feature vector */ { (float)test_size } );

    // Get the policy to execute from Apollo
    int policy = region->getPolicyIndex();

    switch(policy) {
      case 0: daxpy_cpu(A, x, y, r, test_size); break;
      case 1: daxpy_gpu(A, x, y, r, test_size); break;
      default: assert("invalid policy\n");
    }

    // End region execution
    region->end();

    // Verify
#pragma omp parallel for
    for(int i=0; i<test_size; i++)
      assert(r[i] <= 2.0);

  }

  delete[] x;
  delete[] y;
  delete[] r;

  return 0;
}
