
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
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <algorithm>
#include <iomanip>

#include "apollo/Apollo.h"
#include "apollo/Util.h"

// -----

namespace Apollo
{

Util::Util()
{
    return;
}


Util::~Util()
{
    return;
}

std::string
Util::strToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) {
            return std::toupper(c);
        });
    return s;
}

void
Util::strReplaceAll(std::string& input, const std::string& from, const std::string& to) {
	size_t pos = 0;
	while ((pos = input.find(from, pos)) != std::string::npos) {
		input.replace(pos, from.size(), to);
		pos += to.size();
	}
}

const char*
Util::safeGetEnv(
        const char *var_name,
        const char *use_this_if_not_found,
        bool        silent)
{
    char *c = getenv(var_name);
    if (c == NULL) {
        if (not silent) {
            std::cout << "== APOLLO: Looked for " << var_name << " with getenv(), found nothing, using '" \
                << use_this_if_not_found << "' (default) instead." << std::endl;
    }
        return use_this_if_not_found;
    } else {
        return c;
    }
}

bool
Util::strOptionIsEnabled(std::string so) {
    std::string sup = strToUpper(so);
    if ((sup.compare("1")           == 0) ||
        (sup.compare("TRUE")        == 0) ||
        (sup.compare("YES")         == 0) ||
        (sup.compare("ENABLED")     == 0) ||
        (sup.compare("VERBOSE")     == 0) ||
        (sup.compare("ON")          == 0)) {
        return true;
    } else {
        return false;
    }
}


} //end: Apollo (namespace)
