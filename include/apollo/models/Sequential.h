#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include <string>

#include "apollo/PolicyModel.h"

namespace Apollo {

class Sequential : public PolicyModel {
    public:
        Sequential(int num_policies);
        ~Sequential();

        int  getIndex(std::vector<float> &features);

    private:

}; //end: Sequential (class)

}; //end: Apollo (namespace)

#endif
