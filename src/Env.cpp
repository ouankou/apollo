
// Copyright (c) 2020, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#ifdef _OPENMP
#include <omp.h>
#endif

#include "apollo/Exec.h"
#include "apollo/Env.h"
#include "apollo/Logging.h"

namespace Apollo
{

Env::Env()
{
    clear();
    load(detect());
    loaded = validate();
}; //end: Env (constructor)

Env::~Env()
{
    clear();
}; //end: ~Env (deconstructor)

void
Env::clear(void)
{
    //TODO[cdw]: Clear out the values before loading them from the environment.
    name = "unknown";
    return;
} //end: clear

std::string
Env::detect(void)
{
    //TODO[cdw]: Add support for LSF environments.
    return "slurm";
}


void
Env::load(std::string from_env)
{
    Apollo::Exec *apollo = Apollo::Exec::instance();

    log("Reading SLURM env...");
    numNodes      = std::stoi(apollo->util.safeGetEnv("SLURM_NNODES", "1"));
    numProcs      = std::stoi(apollo->util.safeGetEnv("SLURM_NPROCS", "1"));
    numCPUsOnNode = std::stoi(apollo->util.safeGetEnv("SLURM_CPUS_ON_NODE", "36"));
    log("    numCPUsOnNode ...........: ", numCPUsOnNode);
    log("    numNodes ................: ", numNodes);
    log("    numProcs ................: ", numProcs);

    std::string envProcPerNode = apollo->util.safeGetEnv("SLURM_TASKS_PER_NODE", "1");
    // Sometimes SLURM sets this to something like "4(x2)" and
    // all we care about here is the "4":
    auto pos = envProcPerNode.find('(');
    if (pos != envProcPerNode.npos) {
        numProcsPerNode = std::stoi(envProcPerNode.substr(0, pos));
    } else {
        numProcsPerNode = std::stoi(envProcPerNode);
    }
    log("    numProcsPerNode .........: ", numProcsPerNode);

    numThreadsPerCPUCap = std::max(1, (int)(numCPUsOnNode / numProcsPerNode));
    log("    numThreadsPerCPUCap ....: ", numThreadsPerCPUCap);

#ifdef _OPENMP
    log("Reading OMP env...");
    ompDefaultSchedule   = omp_sched_static;       //<-- libgomp.so default
    ompDefaultNumThreads = numThreadsPerCPUCap;    //<-- from SLURM calc above
    ompDefaultChunkSize  = -1;                     //<-- let OMP decide

    // Override the OMP defaults ONLY if there is an environment variable set:
    char *val = NULL;
    val = getenv("OMP_NUM_THREADS");
    if (val != NULL) {
        ompDefaultNumThreads = std::stoi(val);
    }

    val = NULL;
    // We assume nonmonotinicity and chunk size of -1 for now.
    val = getenv("OMP_SCHEDULE");
    if (val != NULL) {
        std::string sched = getenv("OMP_SCHEDULE");
        if ((sched.find("static") != sched.npos)
            || (sched.find("STATIC") != sched.npos)) {
            ompDefaultSchedule = omp_sched_static;
        } else if ((sched.find("dynamic") != sched.npos)
            || (sched.find("DYNAMIC") != sched.npos)) {
            ompDefaultSchedule = omp_sched_dynamic;
        } else if ((sched.find("guided") != sched.npos)
            || (sched.find("GUIDED") != sched.npos)) {
            ompDefaultSchedule = omp_sched_guided;
        }
    }

    //TODO[cdw]: This 'global' should go away if not in use.
    Apollo::Exec::instance()->numThreads = ompDefaultNumThreads;
#endif

} //end: load


bool
Env::validate(void)
{
    //TODO[cdw]: Sanity-check all the values loaded from environment or defaults.
    return true;
} //end: validate


bool
Env::refresh(void)
{
    clear();
    name = detect();
    load(name);
    return validate();
} //end: refresh


} //end: Apollo (namespace)

