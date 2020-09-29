
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

#ifndef APOLLO_UTIL_H
#define APOLLO_UTIL_H

namespace Apollo
{
class Util
{

    public:
        Util();
        ~Util();

        inline std::string strToUpper(std::string s);

        inline void strReplaceAll(std::string& input, const std::string& from, const std::string& to);

        inline const char* safeGetEnv(
                const char *var_name,
                const char *use_this_if_not_found,
                bool        silent=false);

        bool strOptionIsEnabled(std::string so);

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


} //end: Util (class)

} //end: Apollo (namespace)

#endif
