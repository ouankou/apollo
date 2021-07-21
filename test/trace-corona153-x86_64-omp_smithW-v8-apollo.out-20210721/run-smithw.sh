#!/bin/bash -xe
# -x will show the expanded commands
# -e abor on any error

EXE_FILE=omp_smithW-v8-apollo.out
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

LOGFILE=$0-log-$TIMESTAMP.txt  
exec 6>&1           # Link file descriptor #6 with stdout.
exec 7>&2
exec &> $LOGFILE     # stdout replaced with file "logfile.txt".

#TIMESTAMP="debug"
NODE_NAME=`uname -n`
HARDWAREE_NAME=`uname -m`

#FOLDER_SUFFIX="-$NODE_NAME-$HARDWAREE_NAME-$EXE_FILE-$TIMESTAMP"
# For Debug 
# using a fixed folder, with previous built model or collected data, continue the execution
# Be careful, no overlapping of input sizes, or the timing will be accumulated wrongfully. (No accumulation across execution in static model)
FOLDER_SUFFIX=-corona153-x86_64-omp_smithW-v8-apollo.out-20210721

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
#using the physical core count as default thread count, avoiding hyper threading
PHYSICAL_CORE_COUNT=`lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l`
export OMP_NUM_THREADS=$PHYSICAL_CORE_COUNT
echo "Set to physical core count: OMP_NUM_THREADS="$OMP_NUM_THREADS

# need 75 points at least to trigger model building
# 256 * 75= 19200
# for each N size
counter=""
# sampling range and data points are essential to capture the trend 
#for n_size in {256..25000..256}; do
# we have 117 x 3 = 351 records in datasets, more than enough to trigger model building
# we collect roughly 80 data points *3 = 240 records
for n_size in {32..20000..256}; do
 let "counter += 1"
 echo "running count=$counter, problem m_size=$M_SIZE n_size=$n_size"

  M_SIZE=$n_size

# SW has 3 variants: one input size, one policy only each time
# on tux385, we only run 2 variants.
#  for policy in 0 1;   do
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
  APOLLO_TRACE_CSV_FOLDER_SUFFIX=$FOLDER_SUFFIX \
  APOLLO_CROSS_EXECUTION=1 APOLLO_USE_TOTAL_TIME=1 APOLLO_INIT_MODEL="Static,$policy" \
  APOLLO_CROSS_EXECUTION_MIN_DATAPOINT_COUNT=80 APOLLO_TRACE_CROSS_EXECUTION=1 \
  ./$EXE_FILE $M_SIZE $n_size
#    done
  done 

done

exec 1>&6 6>&-      # Restore stdout and close file descriptor #6.
exec 2>&7 7>&- # restore 2 and cancel 7
