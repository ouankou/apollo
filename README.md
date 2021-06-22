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
