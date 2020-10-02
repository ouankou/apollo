
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

#ifndef APOLLO_EXEC_H
#define APOLLO_EXEC_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include "apollo/Env.h"
#include "apollo/Util.h"
#include "apollo/Config.h"
#include "apollo/Trace.h"
#include "apollo/Region.h"

namespace Apollo
{

class Exec
{
    public:
       ~Exec();
        // Disallow copy constructor
        Exec(const Exec&) = delete;
        Exec& operator=(const Exec&) = delete;

        // Return a pointer to the singleton instance of Apollo
        static Exec* instance(void) noexcept {
            static Exec the_instance;
            return &the_instance;
        }

        // Apollo runtime core components
        Apollo::Util   util;
        Apollo::Env    env;
        Apollo::Config config;
        Apollo::Trace  trace;

        // Call step() at the bottom of each major simulation loop, after the
        // application's work is done, and potentially overlapping with an I/O
        // phase. This is another place where Apollo can trigger model training
        // for regions with new timing data, perform MPI collectives, export
        // models to the filesystem, or do other assorted heavy lifting.
        // NOTE: Async is not guaranteed to be supported, but is included here as
        //       an optional parameter for the purpose of interface stability.
        //       If Async is true, Apollo may spawn or re-use a thread.

        void step(size_t application_step=0, bool run_async=false);

        // ---

        //
        // TODO(chad): This is serving as an override that is defined by an
        //             environment variable.  Apollo::Region's are able to
        //             have different policy counts, so a global setting here
        //             should indicate that it is an override, or find
        //             a better place to live.  Leaving it for now, as a low-
        //             priority task.
        int num_policies;

        // TODO(chad): Get rid of this usage, it's bad form.
        int numThreads;  // <-- how many to use / are in use

        // Key: region name, value: region raw pointer
        // Key: region name, value: map key: num_elements, value: policy_index, time_avg
        std::map<std::string, Apollo::Region *> regions;
        std::map< std::vector< float >, std::pair< int, double > > best_policies_global;

        // Extract the location in code where a template was instantiated,
        // returning a string with both the module and the offset within.
        // NOTE: We default to walk_distance of 2 so we can
        //       step out of this method, then step out of
        //       some portable policy template, and get to the
        //       module name and offset where that template
        //       has been instantiated in the application code.
        //       For different embeddings, you can adjust the param.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

    private:
        Exec();

        // TODO(chad): Revisit this for generality:
        void packMeasurements(char *buf, int size, void *_reg);
        void gatherReduceCollectiveTrainingData(size_t application_step);


}; //end: Exec (class)

}; //end: Apollo (namespace)

#endif
