#!/bin/bash

EXE_FILE=matrixMultiplication.out
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
NODE_NAME=`uname -n`
HARDWAREE_NAME=`uname -m`
make clean
make ./$EXE_FILE

export OMP_NUM_THREADS=88
# let the program set it automatically?
# run 50 times to collect enough training data
# 10k to 1000k, step 20k
# or run up to 25 times only
# 40k to 1040k, step 40k
counter=""
for size in {256..7000..256};
do
 let "counter += 1"
 echo "running count=$counter, problem size=$size"

# make the trace folder name more descriptive:  machine, compiler, thread count, program, timestamp 
  APOLLO_TRACE_FOLDER_SUFFIX="-$NODE_NAME-$HARDWAREE_NAME-clang-12.0.0-$OMP_NUM_THREADS-threads-$EXE_FILE-$TIMESTAMP" APOLLO_CROSS_EXECUTION=1 ./$EXE_FILE $size
done
