
#ifndef APOLLO_MODELWRAPPER_H
#define APOLLO_MODELWRAPPER_H

#include <memory>

#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"

class Apollo::ModelWrapper {
    public:
        ModelWrapper(
                Apollo::Region *region_ptr,
                int          numPolicies);
        ~ModelWrapper();

        bool configure(const char *model_def);

        // NOTE: This is what RAJA loops use, not the model's
        //       direct model->getIndex() method.
        int          requestPolicyIndex(void);
        //
        std::string  getName(void); //The name of the kind of model, i.e. "RoundRobin"
        uint64_t     getGuid(void);
        int          getPolicyCount(void);
        bool         isTraining(void);

    private:
        Apollo         *apollo;
        Apollo::Region *region;  //What region this ModelWrapper is working for.
        //
        std::string     id;
        int             num_policies;
        //
        std::shared_ptr<Apollo::ModelObject> model_sptr;

}; //end: Apollo::Model


#endif

