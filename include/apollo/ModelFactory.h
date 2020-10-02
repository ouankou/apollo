
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

#ifndef APOLLO_MODEL_FACTORY_H
#define APOLLO_MODEL_FACTORY_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif


#include <memory>
#include <vector>

#include "apollo/PolicyModel.h"
#include "apollo/TimingModel.h"

namespace Apollo
{

// Factory
class ModelFactory {
    public:
        static std::unique_ptr<PolicyModel> createStatic(int num_policies, int policy_choice );
        static std::unique_ptr<PolicyModel> createRandom(int num_policies);
        static std::unique_ptr<PolicyModel> createRoundRobin(int num_policies);

        static std::unique_ptr<PolicyModel> loadDecisionTree(int num_policies,
                std::string path);
        static std::unique_ptr<PolicyModel> createDecisionTree(int num_policies,
                std::vector< std::vector<float> > &features,
                std::vector<int> &responses );

        static std::unique_ptr<TimingModel> createRegressionTree(
                std::vector< std::vector<float> > &features,
                std::vector<float> &responses );
}; //end: ModelFactory (class)

}; //end: Apollo (namespace)
#endif
