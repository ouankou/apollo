#!/bin/bash

EXE_FILE=matrixMultiplication.out
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

make clean
make ./$EXE_FILE
# run 50 times to collect enough training data
# 10k to 1000k, step 20k
# or run up to 25 times only
# 40k to 1040k, step 40k
counter=""
for size in {256..7000..256};
do
 let "counter += 1"
 echo "running count=$counter, problem size=$size"
  APOLLO_TRACE_CSV_FOLDER_SUFFIX="-$EXE_FILE-$TIMESTAMP" APOLLO_CROSS_EXECUTION=1 ./$EXE_FILE $size
done
