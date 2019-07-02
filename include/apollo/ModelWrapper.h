
#ifndef APOLLO_MODELWRAPPER_H
#define APOLLO_MODELWRAPPER_H

#include <memory>

#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"

class Apollo::ModelWrapper {
    public:
        ModelWrapper(
                Apollo         *apollo_ptr,
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

        // ----------
        // NOTE: Deprecated because we're not dlopen'ing models
        //       as external shared objects.
        //       Keeping this as function signature reference.
        //Apollo::ModelObject* (*create)();
        // NOTE: Destruction is handled by std::shared_ptr now.
        //void (*destroy)(Apollo::ModelObject*);

}; //end: Apollo::Model


#endif

