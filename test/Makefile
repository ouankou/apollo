CC=clang++
CXXFLAGS=-O3 -fopenmp -fopenmp-targets=nvptx64
APOLLO_DIR=/path/to/apollo/install-lassen
INC_DIR=$(APOLLO_DIR)/include 
LIB_DIR=$(APOLLO_DIR)/lib
LIBS=-lapollo

daxpy: daxpy.cpp
	$(CC) $(CXXFLAGS) -I $(INC_DIR) -L $(LIB_DIR) -Wl,--rpath,$(LIB_DIR) daxpy.cpp -o daxpy $(LIBS)


.PHONY: clean run

run:
	APOLLO_RETRAIN_ENABLE=0 APOLLO_COLLECTIVE_TRAINING=0 APOLLO_LOCAL_TRAINING=1 \
	APOLLO_STORE_MODELS=1 APOLLO_INIT_MODEL="RoundRobin" ./daxpy

clean:
	rm -rf daxpy

