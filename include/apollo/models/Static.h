#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include "apollo/PolicyModel.h"

namespace Apollo {

class Static : public PolicyModel {
    public:
        Static(int num_policies, int policy_choice) :
           PolicyModel(num_policies, "Static", false), policy_choice(policy_choice) {};
        ~Static() {};

        //
        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename) {};

    private:
        int policy_choice;

}; //end: Static (class)

}; //end: Apollo (namespace)
#endif
