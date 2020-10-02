
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

#ifndef APOLLO_LOGGING_H
#define APOLLO_LOGGING_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include <cstring>
#include <string>
#include <utility>
#include <iostream>
#include <type_traits>

#ifndef APOLLO_VERBOSE
#define APOLLO_VERBOSE 0
#endif

#ifndef APOLLO_LOG_FIRST_RANK_ONLY
#define APOLLO_LOG_FIRST_RANK_ONLY 1
#endif

namespace Apollo {

template <typename Arg, typename... Args>
void log(Arg&& arg, Args&&... args)
{
    if (APOLLO_VERBOSE < 1) return;

    static int rank = -1;
    if (rank < 0) {
        const char *rank_str = getenv("SLURM_PROCID");
        if ((rank_str != NULL) && (strlen(rank_str) > 0))
        {
            rank = atoi(rank_str);
        }
    }

    if (APOLLO_LOG_FIRST_RANK_ONLY > 0)
    {
        if (rank != 0) return;
    }

    std::cout << "== APOLLO(" << rank << "): ";
    std::cout << std::forward<Arg>(arg);
    using expander = int[];
    (void)expander{0, (void(std::cout << std::forward<Args>(args)), 0)...};
    std::cout << std::endl;
}; //end: log (template)

}; //end: Apollo (namespace)
#endif
