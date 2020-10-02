
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


#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include <vector>
#include <chrono>
#include <memory>
#include <map>

#include "apollo/PolicyModel.h"
#include "apollo/TimingModel.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif //ENABLE_MPI

namespace Apollo {

class Region {
    public:
        Region(
                const int    num_features,
                const char   *regionName,
                int           numAvailablePolicies);
         Region(
                const int    num_features,
                const char   *regionName,
                int           numAvailablePolicies,
                std::string   loadModelFromThisYamlFile);
        ~Region();

        typedef struct Measure {
            int       exec_count;
            double    time_total;
            Measure(int e, double t) : exec_count(e), time_total(t) {}
        } Measure;

        std::string  name;

        void     begin();
        void     end();
        void     end(double synthetic_duration_or_weight);
                    // lower == better, 0.0 == perfect

        int      getPolicyIndex(void);

        int      current_policy;
        int      current_elem_count;

        int      num_features;
        void     setFeature(float value);
        int      reduceBestPolicies(int step);

        std::map<
            std::vector< float >,
            std::pair< int, double > > best_policies;

        std::map<
            std::pair< std::vector<float>, int >,
            std::unique_ptr<Apollo::Region::Measure> > measures;
        //^--Explanation: < features, policy >, value: < time measurement >

        std::unique_ptr<TimingModel> time_model;
        std::unique_ptr<PolicyModel> model;

    private:
        bool           currently_inside_region;
        //
        std::chrono::steady_clock::time_point current_exec_time_begin;
        std::chrono::steady_clock::time_point current_exec_time_end;
        std::vector<float>            features;
        //
}; //end: Region (class)

}; //end: Apollo (namespace)

#endif
