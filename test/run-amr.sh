#!/bin/bash

EXE_FILE=amr_stencil.out
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
NODE_NAME=`uname -n`
HARDWAREE_NAME=`uname -m`
make clean
make ./$EXE_FILE

export OMP_NUM_THREADS=88
counter=""
for size in {128..1025..64};
do
 let "counter += 1"
 echo "running count=$counter, problem size=$size"

# make the trace folder name more descriptive:  machine, compiler, thread count, program, timestamp 
  APOLLO_TRACE_CSV_FOLDER_SUFFIX="-$NODE_NAME-$HARDWAREE_NAME-clang-12.0.0-$OMP_NUM_THREADS-threads-$EXE_FILE-$TIMESTAMP" APOLLO_CROSS_EXECUTION=1 ./$EXE_FILE $size 2
done
