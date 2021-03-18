CC=clang++
CXXFLAGS=-O3 -g -fopenmp -fopenmp-targets=nvptx64

# APOLLO_HOME is your environment variable indicating the path to Apollo Installation
# e.g.: 
#   export APOLLO_HOME=/home/xyz/opt/apollo
APOLLO_DIR=$(APOLLO_HOME)
INC_DIR=$(APOLLO_DIR)/include 
LIB_DIR=$(APOLLO_DIR)/lib
LIBS=-lapollo

# This version uses a fixed sized array and random sized portion to collect data
daxpy: daxpy.cpp
	$(CC) $(CXXFLAGS) -I $(INC_DIR) -L $(LIB_DIR) -Wl,--rpath,$(LIB_DIR) daxpy.cpp -o daxpy $(LIBS)

# version 2 accepts an option to specify array size, enabling cross-execution optimiztion  
daxpy-v2: daxpy-v2.cpp
	$(CC) $(CXXFLAGS) -I $(INC_DIR) -L $(LIB_DIR) -Wl,--rpath,$(LIB_DIR) $^ -o $@ $(LIBS)
daxpy-v3: daxpy-v3.cpp
	$(CC) $(CXXFLAGS) -I $(INC_DIR) -L $(LIB_DIR) -Wl,--rpath,$(LIB_DIR) $^ -o $@ $(LIBS)
  
all: daxpy daxpy-v2 daxpy-v3 
.PHONY: clean run
# TEST cross execution: save region information from current execution
run2: daxpy-v2
	APOLLO_CROSS_EXECUTION=1 ./$^ 10000000
run: daxpy 
	./daxpy
# We have proper flags set by default now
#APOLLO_RETRAIN_ENABLE=0 APOLLO_COLLECTIVE_TRAINING=0 APOLLO_LOCAL_TRAINING=1 APOLLO_STORE_MODELS=1 APOLLO_INIT_MODEL="RoundRobin" ./daxpy

clean:
	rm -rf daxpy daxpy-v2

