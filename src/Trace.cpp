
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

namespace Apollo
{

Trace::Trace()
{
    Apollo::Exec *apollo = Apollo::Exec::instance();

    std::string tmp_enabled           = apollo->util.safeGetEnv("APOLLO_TRACE_ENABLED", "false", true);
    std::string tmp_emit_online       = apollo->util.safeGetEnv("APOLLO_TRACE_EMIT_ONLINE", "false", true);
    std::string tmp_emit_all_features = apollo->util.safeGetEnv("APOLLO_TRACE_EMIT_ALL_FEATURES", "false", true);
    std::string tmp_output_file       = apollo->util.safeGetEnv("APOLLO_TRACE_OUTPUT_FILE", "stdout", true);
    //
    enabled          = apollo->util.strOptionIsEnabled(tmp_enabled);
    emitOnline       = apollo->util.strOptionIsEnabled(tmp_emit_online);
    emitAllFeatures  = apollo->util.strOptionIsEnabled(tmp_emit_all_features);
    //
    outputFileName   = tmp_output_file;
    if (enabled) {
        if (outputFileName.compare("stdout") == 0) {
            outputIsActualFile = false;
        } else {
            outputFileName += ".";
            outputFileName += std::to_string(mpiRank);
            outputFileName += ".csv";
            try {
                outputFileHandle.open(outputFileName, std::fstream::out);
                outputIsActualFile = true;
            } catch (...) {
                std::cerr << "== APOLLO: ** ERROR ** Unable to open the filename specified in" \
                    << "APOLLO_TRACE_OUTPUT_FILE environment variable:" << std::endl;
                std::cerr << "== APOLLO: ** ERROR **    \"" << outputFileName << "\"" << std::endl;
                std::cerr << "== APOLLO: ** ERROR ** Defaulting to std::cout ..." << std::endl;
                outputIsActualFile = false;
            }
            if (emitOnline) {
                writeTraceHeader();
            }
        }
    }
    //
    //////////
} //end: Trace  (constructor)


Trace::~Trace()
{
    if (enabled and (not emitOnline)) {
        // We've been storing up our trace data to emit now.
        writeHeader();
        writeVector();
        if (outputIsActualFile) {
            outputFileHandle.close();
        }
    }

}


// TODO(cdw): Generalize the fields slightly for CUDA also...
void
Trace::writeHeader(void) {
    if (not enabled) { return; }

    if (outputIsActualFile) {
        writeTraceHeaderImpl(outputFileHandle);
    } else {
        writeTraceHeaderImpl(std::cout);
    }
}
//
void
Trace::writeHeaderImpl(std::ostream &sink) {
    if (not enabled) { return; }

    std::string optional_column_label;
    if (emitAllFeatures) {
        optional_column_label = ",all_features_json\n";
    } else {
        optional_column_label = "\n";
    }
    sink.precision(11);
    sink \
        << "type," \
        << "time_wall," \
        << "node_id," \
        << "step," \
        << "region_name," \
        << "policy_index," \
        << "num_threads," \
        << "num_elements," \
        << "time_exec" \
        << optional_column_label;
}
//
void
Trace::storeLine(TraceLine_t &t) {
    if (not enabled) { return; }

    trace_data.push_back(t);
}
//
void
Trace::writeLine(TraceLine_t &t) {
    if (not enabled) { return; }
    if (outputIsActualFile) {
        writeTraceLineImpl(t, outputFileHandle);
    } else {
        writeTraceLineImpl(t, std::cout);
    }
}
//
void
Trace::writeLineImpl(TraceLine_t &t, std::ostream &sink) {
    if (not enabled) { return; }
    sink \
       << "TRACE," << std::fixed \
       << std::get<0>(t) /* exec_time_end */ << "," \
       << std::get<1>(t) /* node_id */       << "," \
       << std::get<2>(t) /* comm_rank */     << "," \
       << std::get<3>(t) /* region_name */   << "," \
       << std::get<4>(t) /* policy_index */  << "," \
       << std::get<5>(t) /* num_threads */   << "," \
       << std::get<6>(t) /* num_elements */  << "," \
       << std::fixed << std::get<7>(t) /* (exec_time_end - exec_time_begin) */ \
       << std::get<8>(t) /* (", " + all features, optionally) */ \
       << "\n";
}
//
void
Trace::writeVector(void) {
    if (not enabled) { return; }
    for (auto &t : trace_data) {
        writeLine(t);
    }
}
//



} //end: Apollo (namespace)
