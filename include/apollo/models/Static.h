#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include "apollo/PolicyModel.h"
#include "apollo/Config.h"
#include <iostream>
class Static : public PolicyModel {
    public:
        Static(int num_policies, int policy_choice) : 
           PolicyModel(num_policies, "Static", false), policy_choice(policy_choice) 
        {
          // special adjustment: for cross execution tuning mode, we sometimes use static policy as a training tool
          if (Config::APOLLO_CROSS_EXECUTION)
          {
            if (Config::APOLLO_TRACE_CROSS_EXECUTION)
                  std::cout<<"Static Model is tagged as training model in Cross Execution mode"<<std::endl;
            training = true;
          }
        };
        ~Static() {};

        //
        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename) {};

    private:
        int policy_choice;

}; //end: Static (class)


#endif
