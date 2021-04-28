SHELL=/bin/bash

# AMD GPU
ifeq ($(findstring corona,$(HOSTNAME)), corona)
  CC=/opt/rocm-4.1.0/llvm/bin/clang++
  CXXFLAGS=-O3 -target x86_64-pc-linux-gnu -fopenmp -fopenmp-version=50 -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -march=gfx906

# on lassen, Nvidia GPU  
else
  CC=clang++
  CXXFLAGS=-O3 -g -fopenmp -fopenmp-targets=nvptx64
endif 

# APOLLO_HOME is your environment variable indicating the path to Apollo Installation
# e.g.: 
#   export APOLLO_HOME=/home/xyz/opt/apollo
APOLLO_DIR=$(APOLLO_HOME)
INC_DIR=$(APOLLO_DIR)/include 
LIB_DIR=$(APOLLO_DIR)/lib
LIBS=-lapollo

LINK_FALGS= -L$(LIB_DIR) -Wl,--rpath,$(LIB_DIR)

ifeq ($(findstring corona,$(HOSTNAME)), corona)
# must use alternative libstdc++ /usr/tce/packages/gcc/gcc-4.9.3/lib64/ 
# # opencv and apollo are built using gcc 4.9.3
 LINK_FALGS+= -L/usr/tce/packages/gcc/gcc-4.9.3/lib64/ -Wl,--rpath,/usr/tce/packages/gcc/gcc-4.9.3/lib64/
endif  


CPP_SRC_FILES = \
daxpy.cpp \
daxpy-v2.cpp \
daxpy-v3.cpp \
matrixMultiplication.cpp \
hasGPU.cpp

OBJ_FILES = $(addsuffix .out, $(basename $(CPP_SRC_FILES)))

%.out:%.cpp
	$(CC) $(CXXFLAGS) -I$(INC_DIR) $(LINK_FALGS) $^ -o $@ $(LIBS)

all: $(OBJ_FILES)
test: hasGPU.out
	./$<

.PHONY: clean run
# TEST cross execution: save region information from current execution
run2: daxpy-v2
	APOLLO_CROSS_EXECUTION=1 ./$^ 10000000
run: daxpy 
	./daxpy
# We have proper flags set by default now
#APOLLO_RETRAIN_ENABLE=0 APOLLO_COLLECTIVE_TRAINING=0 APOLLO_LOCAL_TRAINING=1 APOLLO_STORE_MODELS=1 APOLLO_INIT_MODEL="RoundRobin" ./daxpy

clean:
	rm -rf daxpy daxpy-v2 *.out

