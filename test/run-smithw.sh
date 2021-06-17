#!/bin/bash -xe
# -x will show the expanded commands
# -e abor on any error

EXE_FILE=omp_smithW-v8-apollo.out
#TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TIMESTAMP="debug"
NODE_NAME=`uname -n`
HARDWAREE_NAME=`uname -m`
make clean
make ./$EXE_FILE

# data size ranges used in metadirective iwomp'19 paper
# fixed M: 2,000
#  varying N: 400 - 60,000

# run 50 times to collect enough training data
# 10k to 1000k, step 20k
# or run up to 25 times only
# 40k to 1040k, step 40k

#M_SIZE=2000

# for each N size
counter=""
# for n_size in {256..7000..256}; do
# sampling range and data points are essential to capture the trend 
for n_size in {5000..10000..256}; do
 let "counter += 1"
 echo "running count=$counter, problem m_size=$M_SIZE n_size=$n_size"

  M_SIZE=$n_size

# SW has 3 variants: one input size, one policy only each time
  for policy in 0 1 2;   do
# run 3 times each so we can get average: no need to repeat. kernel will be called many times already within outer loop.
#    for repeat in 1 2 3;   do
#    for repeat in 1;   do
    echo "Policy=$policy, Repeat=$repeat"
# ./$EXE_FILE
#. why do we use static model here??  Why not Round-Robin?
# well, each execution will have only 1 data point for nDiag!
# Round-Robin will try different policies for the same nDiag value, which is not desired. 
# we want the entire execution to try only one policy!!

# another question: are all execution time added into one single value?  Or we should promote it to be outside of the inner loop?    
# I think so: measures are added into the feature + policy    
       APOLLO_TRACE_CSV_FOLDER_SUFFIX="-$NODE_NAME-$HARDWAREE_NAME-$EXE_FILE-$TIMESTAMP" APOLLO_CROSS_EXECUTION=1 APOLLO_INIT_MODEL="Static,$policy" ./$EXE_FILE $M_SIZE $n_size
#    done
  done 

done
