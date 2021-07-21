# Apollo

Apollo is a distributed tuning framework for parallel applications.  You
instrument your code with the Apollo API, tell Apollo what the tuning
options are, and Apollo recommends tuning options so that your code
runs faster.

## Contributing

To contribute to Apollo please send a
[pull request](https://help.github.com/articles/using-pull-requests/) on the
`develop` branch of this repo. Apollo follows Gitflow for managing development.

## Installation

CMake 3.9 or higher is required.

Tested using GCC 9.2.0
* Note that old GCC 4.x may cause errors during spack's openCV installation. Please use more recent GCC versions like GCC 8.x or 9.x

Apollo depends on OpenCV for machine learning support.  We use lassen@Livermore Computing as an example to show the installation steps: 

First install OpenCV using Spack: 

```

# Get Spack first
git clone git@github.com:spack/spack.git

source spack/share/spack/setup-env.sh

# Using spack to install OpenCV, with its machine learning library explicitly enabled. 
 
spack install opencv+powerpc+vsx~zlib~vtk~videostab~video~ts~tiff~superres~stitching~png~openclamdfft~openclamdblas~gtk~highgui~eigen+python~openmp~videoio~calib3d~features2d~dnn~flann~imgproc~ipp~ipp_iw~jasper~java~jpeg~lapack~opencl~opencl_svm~pthreads_pf+ml

# On Lassen, IBM Power platform, use:
spack install opencv+vsx+powerpc+python+ml

# Note the options: 
# vsx [off] on, off Enable POWER8 and above VSX (64-bit little-endian)
# powerpc [off] on, off Enable PowerPC for GCC

# on Intel platform: just use
spack install opencv+python+ml

spack load opencv  # load the installed opencv

```


Now install Apollo:
```
git clone git@github.com:LLNL/apollo.git
cd apollo
mkdir build
cd build

# MPI has to be turned off for now
cmake ../. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=../install-lassen -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_MPI=Off -Wno-dev

make install

```
## Testing

Preparing environment: 
```
# apollo specific stuff
export APOLLO_HOME=/home/user1/apollo/install
source spack/share/spack/setup-env.sh
spack load opencv@4.5.2%gcc@9.2.0
```

Run builtin tests

```
cd apollo/test
# customize makefile as needed
make

```

## Config Options

Apollo uses a set of environment variables to customize its features. The environment variables are declared in https://github.com/chunhualiao/apollo/blob/develop/include/apollo/Config.h

Some of them are
* static int APOLLO_COLLECTIVE_TRAINING; // Using MPI to have multiple compute nodes for training
* static int APOLLO_LOCAL_TRAINING;         // Using a single computation node for traning
* static int APOLLO_SINGLE_MODEL;  // a single model for all code regions
* static int APOLLO_REGION_MODEL;  // each region has its own model 
* static int APOLLO_NUM_POLICIES;
* static int APOLLO_RETRAIN_ENABLE;
* static float APOLLO_RETRAIN_TIME_THRESHOLD;
* static float APOLLO_RETRAIN_REGION_THRESHOLD;

Cross execution tuning options
* static int APOLLO_CROSS_EXECUTION;  // enable cross execution profiling/ modeling/adaptation       
* static int APOLLO_CROSS_EXECUTION_MIN_DATAPOINT_COUNT;  // minimum number of data points for each feature needed to trigger model building, default is 25
* static int APOLLO_USE_TOTAL_TIME;  //For some training data , we don't average the measures, but use total accumulated time for the current execution. This is used for some experiment when a single execution with a given input data will only explore one policy. All executions of the same region should be added into a total execution time, instead of calculating their average values.
         
Debugging and tracing configurations
* static int APOLLO_STORE_MODELS;
* static int APOLLO_TRACE_MEASURES; // trace Apollo::Region::reduceBestPolicies(), generating a log file containing measures and labeled training data with best policies
* static int APOLLO_TRACE_POLICY;
* static int APOLLO_TRACE_RETRAIN;
* static int APOLLO_TRACE_ALLGATHER;
* static int APOLLO_TRACE_BEST_POLICIES;
* static int APOLLO_TRACE_CROSS_EXECUTION;
* static int APOLLO_FLUSH_PERIOD;
* static int APOLLO_TRACE_CSV;        
* static std::string APOLLO_INIT_MODEL; // initial policy selection modes: random, roundRobin, static,0 (always 1st one), etc
* static std::string APOLLO_TRACE_CSV_FOLDER_SUFFIX;

## Internals

[Data Processing Process](https://github.com/chunhualiao/apollo/blob/develop/docs/training-data-processing.md): This file gives more detailed information about how Apollo collects training data, geneates models, and triggers model-driven adaptation. 


## Authors

Apollo is currently developed by Giorgis Georgakoudis, georgakoudis1@llnl.gov,
Chad Wood, wood67@llnl.gov, and other
[contributors](https://github.com/LLNL/apollo/graphs/contributors).

Apollo was originally created by David Beckingsale, david@llnl.gov

If you are referencing Apollo in a publication, please cite this repo and
the following paper:

* David Beckingsale and Olga Pearce and Ignacio Laguna and Todd Gamblin.
  [**Apollo: Fast, Dynamic Tuning for Data-Dependent Code**](https://computing.llnl.gov/projects/apollo/apollo-fast-lightweight-dynamic-tuning-data-dependent-code-llnl-paper_0.pdf). In *IEEE International Parallel & Distributed Processing Symposium (IPDPS'17)*, Orlando, FL, May 29-June 2 2017. LLNL-CONF-723337.

## License

Apollo is distributed under the terms of both the MIT license.  All new
contributions must be made under the MIT license.

See [LICENSE](https://github.com/LLNL/apollo/blob/master/LICENSE) and
[NOTICE](https://github.com/LLNL/apollo/blob/master/NOTICE) for details.

SPDX-License-Identifier: MIT

LLNL-CODE-733798
