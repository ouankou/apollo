#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include <string>
#include <vector>
#include <map>

#include "apollo/PolicyModel.h"

namespace Apollo {

class RoundRobin : public PolicyModel {
    public:
        RoundRobin(int num_policies);
        ~RoundRobin();

        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename) {};

    private:
        std::map< std::vector<float>, int > policies;

}; //end: RoundRobin (class)

}; //end: Apollo (namespace)
#endif
