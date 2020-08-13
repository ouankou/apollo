
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

#ifndef APOLLO_TRACE_H
#define APOLLO_TRACE_H

namespace Apollo
{

class Trace
{
    public:
        ~Trace();
        //disallow copy constructor
        Trace(const Trace&) = delete;
        Trace& operator=(const Trace&) = delete;

    bool          enabled;
    bool          emitOnline;
    bool          emitAllFeatures;
    bool          outputIsActualFile;
    std::string   outputFileName;
    std::ofstream outputFileHandle;
    //
    typedef std::tuple<
        double,
        std::string,
        int,
        std::string,
        int,
        int,
        int,
        double,
        std::string
        > TraceLine_t;
    typedef std::vector<TraceLine_t> TraceVector_t;
    //
    void storeLine(TraceLine_t &t);
    //
    void writeHeaderImpl(std::ostream &sink);
    void writeHeader(void);
    void writeLineImpl(TraceLine_t &t, std::ostream &sink);
    void writeLine(TraceLine_t &t);
    void writeVector(void);
    //
    //////////

}; //end: Trace (class)
}; //end: Apollo (namespace)

#endif
