
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

#include "apollo/Config.h"
#include "apollo/Region.h"
#include "apollo/Trace.h"

namespace Apollo
{

class Exec
{
    public:
       ~Exec();
        // disallow copy constructor
        Exec(const Exec&) = delete;
        Exec& operator=(const Exec&) = delete;

        static Exec* instance(void) noexcept {
            static Exec the_instance;
            return &the_instance;
        }

        Apollo::Config config;
        Apollo::Env    env;
        Apollo::Trace  trace;

        //
        //TODO(cdw): This is serving as an override that is defined by an
        //           environment variable.  Apollo::Region's are able to
        //           have different policy counts, so a global setting here
        //           should indicate that it is an override, or find
        //           a better place to live.  Leaving it for now, as a low-
        //           priority task.
        int  num_policies;

        // TODO(chad): Get rid of this usage, it's bad form.
        int numThreads;  // <-- how many to use / are in use

        // NOTE(chad): We default to walk_distance of 2 so we can
        //             step out of this method, then step out of
        //             some portable policy template, and get to the
        //             module name and offset where that template
        //             has been instantiated in the application code.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

        void flushAllRegionMeasurements(int step);

    private:
        Apollo();
        //
        TraceVector_t trace_data;
        //
        void packMeasurements(char *buf, int size, void *_reg);
        void gatherReduceCollectiveTrainingData(int step);
        // Key: region name, value: region raw pointer
        std::map<std::string, Apollo::Region *> regions;
        // Key: region name, value: map key: num_elements, value: policy_index, time_avg
        std::map< std::vector< float >, std::pair< int, double > > best_policies_global;

} //end: Exec (class)

} //end: Apollo (namespace)

#endif
