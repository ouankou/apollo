#!/bin/bash

EXE_FILE=matrixMultiplication.out
#TIMESTAMP=$(date +%Y%m%d_%H%M%S)
# continue the interrupted execution
TIMESTAMP=20210427_102809  
NODE_NAME=`uname -n`
HARDWAREE_NAME=`uname -m`
make -f makefile-corona clean
make -f makefile-corona ./$EXE_FILE
# run 50 times to collect enough training data
# 10k to 1000k, step 20k
# or run up to 25 times only
# 40k to 1040k, step 40k
#counter=""
counter=21
#for size in {256..7000..256};
for size in {5632..7000..256};
do
 let "counter += 1"
 echo "running count=$counter, problem size=$size"

# make the trace folder name more descriptive:  machine, program, timestamp 
  APOLLO_TRACE_CSV_FOLDER_SUFFIX="-$NODE_NAME-$HARDWAREE_NAME-$EXE_FILE-$TIMESTAMP" APOLLO_CROSS_EXECUTION=1 ./$EXE_FILE $size
done
