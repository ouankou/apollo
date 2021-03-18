#!/bin/bash

make clean
make ./daxpy-v2
# run 50 times to collect enough training data
# 10k to 1000k, step 20k
# or run up to 25 times only
# 40k to 1040k, step 40k
counter=""
for size in {40000..1040000..40000};
#for size in {10000..1000000..250000};
# it may never trigger model building, if repeating the same sizes which are already sampled!!
do
 let "counter += 1"
 echo "running count=$counter, problem size=$size"
  APOLLO_CROSS_EXECUTION=1 ./daxpy-v2 $size
done
