#!/bin/bash -xe
# -x will show the expanded commands
# -e abor on any error

EXE_FILE=omp_smithW-v8-apollo.out
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
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

M_SIZE=2000

# for each N size
counter=""
for n_size in {256..7000..256}; do
 let "counter += 1"
 echo "running count=$counter, problem m_size=$M_SIZE n_size=$n_size"

# SW has 3 variants
  for policy in 0 1 2;   do
# run 3 times each so we can get average
    for repeat in 1 2 3;   do
    echo "Policy=$policy, Repeat=$repeat"
# ./$EXE_FILE
       APOLLO_TRACE_CSV_FOLDER_SUFFIX="-$NODE_NAME-$HARDWAREE_NAME-$EXE_FILE-$TIMESTAMP" APOLLO_CROSS_EXECUTION=1 APOLLO_INIT_MODEL="Static,$policy" ./$EXE_FILE $M_SIZE $n_size
    done
  done 

done
