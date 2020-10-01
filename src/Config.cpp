
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

#include <string>

#include "apollo/Exec.h"
#include "apollo/Util.h"
#include "apollo/Config.h"

namespace Apollo
{

Config::Config()
{
    loadSettings();
    return;
}

Config::~Config()
{
    return;
}


void
Config::loadSettings(void)
{
    Apollo::Exec *apollo = Apollo::Exec::instance();

    // Initialize config with defaults
    APOLLO_INIT_MODEL          = apollo->util.safeGetEnv( "APOLLO_INIT_MODEL", "Static,0" );
    APOLLO_COLLECTIVE_TRAINING = std::stoi( apollo->util.safeGetEnv( "APOLLO_COLLECTIVE_TRAINING", "1" ) );
    APOLLO_LOCAL_TRAINING      = std::stoi( apollo->util.safeGetEnv( "APOLLO_LOCAL_TRAINING", "0" ) );
    APOLLO_SINGLE_MODEL        = std::stoi( apollo->util.safeGetEnv( "APOLLO_SINGLE_MODEL", "0" ) );
    APOLLO_REGION_MODEL        = std::stoi( apollo->util.safeGetEnv( "APOLLO_REGION_MODEL", "1" ) );
    APOLLO_TRACE_MEASURES      = std::stoi( apollo->util.safeGetEnv( "APOLLO_TRACE_MEASURES", "0" ) );
    APOLLO_NUM_POLICIES        = std::stoi( apollo->util.safeGetEnv( "APOLLO_NUM_POLICIES", "0" ) );
    APOLLO_TRACE_POLICY        = std::stoi( apollo->util.safeGetEnv( "APOLLO_TRACE_POLICY", "0" ) );
    APOLLO_STORE_MODELS        = std::stoi( apollo->util.safeGetEnv( "APOLLO_STORE_MODELS", "0" ) );
    APOLLO_TRACE_RETRAIN       = std::stoi( apollo->util.safeGetEnv( "APOLLO_TRACE_RETRAIN", "0" ) );
    APOLLO_TRACE_ALLGATHER     = std::stoi( apollo->util.safeGetEnv( "APOLLO_TRACE_ALLGATHER", "0" ) );
    APOLLO_TRACE_BEST_POLICIES = std::stoi( apollo->util.safeGetEnv( "APOLLO_TRACE_BEST_POLICIES", "0" ) );
    APOLLO_RETRAIN_ENABLE      = std::stoi( apollo->util.safeGetEnv( "APOLLO_RETRAIN_ENABLE", "1" ) );
    APOLLO_RETRAIN_TIME_THRESHOLD   = std::stof( apollo->util.safeGetEnv( "APOLLO_RETRAIN_TIME_THRESHOLD", "2.0" ) );
    APOLLO_RETRAIN_REGION_THRESHOLD = std::stof( apollo->util.safeGetEnv( "APOLLO_RETRAIN_REGION_THRESHOLD", "0.5" ) );

    return;
}

bool
Config::sanityCheck(bool abort_on_fail)
{

#ifndef ENABLE_MPI
    // MPI is disabled...
    if ( Config::APOLLO_COLLECTIVE_TRAINING ) {
        std::cerr << "Collective training requires MPI support to be enabled" << std::endl;
        abort();
    }
    //TODO[chad]: Deepen this sanity check when additional collectives/training
    //            backends are added to the code.
#endif //ENABLE_MPI

    if( Config::APOLLO_COLLECTIVE_TRAINING && Config::APOLLO_LOCAL_TRAINING ) {
        std::cerr << "Both collective and local training cannot be enabled" << std::endl;
        abort();
    }

    if( ! ( Config::APOLLO_COLLECTIVE_TRAINING || Config::APOLLO_LOCAL_TRAINING ) ) {
        std::cerr << "Either collective or local training must be enabled" << std::endl;
        abort();
    }

    if( Config::APOLLO_SINGLE_MODEL && Config::APOLLO_REGION_MODEL ) {
        std::cerr << "Both global and region modeling cannot be enabled" << std::endl;
        abort();
    }


    if( ! ( Config::APOLLO_SINGLE_MODEL || Config::APOLLO_REGION_MODEL ) ) {
        std::cerr << "Either global or region modeling must be enabled" << std::endl;
        abort();
    }


    return true;
}

void
Config::print(void)
{
    //TODO[cdw]: Show all the currently known settings on STDOUT
    return;
}


} //end: Apollo (namespace)


