#!/bin/bash -xe
# -x will show the expanded commands
# -e abor on any error

#EXE_FILE=smithw-serial.out
EXE_FILE=smithw-omp-cpu.out
#EXE_FILE=smithw-omp-offload.out
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

#M_SIZE=2000
#using the physical core count as default thread count, avoiding hyper threading
PHYSICAL_CORE_COUNT=`lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l`
export OMP_NUM_THREADS=$PHYSICAL_CORE_COUNT
echo "Set to physical core count: OMP_NUM_THREADS="$OMP_NUM_THREADS

# for each N size
counter=""
#for n_size in {256..7000..256}; do
for n_size in {32..21000..512}; do  # 
 M_SIZE=$n_size
 let "counter += 1"
 echo "running count=$counter, problem m_size=$M_SIZE n_size=$n_size"
    for repeat in 1 2 3;   do
    echo " Repeat=$repeat"
       ./$EXE_FILE $M_SIZE $n_size
    done

done
