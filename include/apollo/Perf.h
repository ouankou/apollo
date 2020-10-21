
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

#ifndef APOLLO_PERF_H
#define APOLLO_PERF_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif


#include <cstdint>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "apollo/Region.h"

namespace Apollo
{
class Perf
{
    public:
        Perf();
        ~Perf();

        class Record
        {
            //TODO[cdw]: Make this like the ApolloContext in the develop branch.
        };

        void queueRecord(uint32_t streamId, Apollo::Perf::Record *pr);
        Apollo::Perf::Record* dequeueRecord(uint32_t streamId);

        void bindRecord(uint32_t correlationId, Apollo::Perf::Record *pr);
        void unbindRecord(uint32_t correlationId);

        Apollo::Region* find(uint32_t id);

    private:
        // For binding async Apollo kernel launches w/CUDA streamId:
        std::unordered_map<uint32_t, std::queue<Apollo::Perf::Record*> > stream_map;
        std::mutex stream_map_mutex;
        // For looking up Perf::Record data by CUPTI correlationId:
        std::unordered_map<uint32_t, Apollo::Perf::Record*> record_map;
        std::mutex record_map_mutex;


        // ---

}; //end: Perf (class)

}; //end: Apollo (namespace)


#endif
