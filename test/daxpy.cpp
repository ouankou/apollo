#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include <apollo/Apollo.h>
#include <apollo/Region.h>

#define ITERS 1000
#define TRAIN_ITERS 100

void daxpy_cpu(double A, double *x, double *y, double *r, size_t size)
{
#pragma omp parallel for
    for(size_t i=0; i<size; i++)
        r[i] = A*x[i] + y[i];
}

void daxpy_gpu(double A, double *x, double *y, double *r, size_t size)
{
#pragma omp target teams distribute parallel for map(to:A,x[0:size],y[0:size],size) map(from:r[0:size])
    for(size_t i=0; i<size; i++)
        r[i] = A*x[i] + y[i];
}

// Example for the DAXPY kernel tunable on different input sizes 
// for CPU/GPU variants
int main()
{
    size_t size = 10000000;
    double A = 1.0;
    double *x = new double[size];
    double *y = new double[size];
    double *r = new double[size];
    // Init
    printf("Init...\n");
    for(int i=0; i<size; i++) {
        x[i] = random() % 2;
        y[i] = random() % 2;
    }

    // Get Apollo instance
    Apollo *apollo = Apollo::instance();

    // Create Apollo region
    Apollo::Region *region = new Apollo::Region(
            /* NumFeatures */ 1,
            /* id */ "daxpy",
            /* NumPolicies */ 2);

    printf("Processing...\n");
    // Run for ITERS iterations, train after the first TRAIN_ITERS
    for(int j=0; j<ITERS; j++) {
        // Pick a random test size
        size_t test_size = (random() % size) + 1;

        // Begin region and set feature vector
        region->begin( /* feature vector */ { (float)test_size } );
        // Get the policy to execute from Apollo
        int policy = region->getPolicyIndex();

        double start = omp_get_wtime();
        switch(policy) {
            case 0: daxpy_cpu(A, x, y, r, test_size); break;
            case 1: daxpy_gpu(A, x, y, r, test_size); break;
            default: assert("invalid policy\n");
        }
        double end = omp_get_wtime();
        printf("%s Elapsed time %.3lf ms (size %lu)\n", (policy ? "GPU" : "CPU"), 1000.0*(end - start), test_size);

        // End region execution
        region->end();

        // Verify
#pragma omp parallel for
        for(int i=0; i<test_size; i++)
            assert(r[i] <= 2.0);

        // Train an optimized selection model after TRAIN_ITERS iterations
        if(j == TRAIN_ITERS) {
            apollo->flushAllRegionMeasurements(j);
            printf("==========\n DONE TRAINING \n==========\n");
        }
    }

    delete[] x;
    delete[] y;
    delete[] r;

    return 0;
}
