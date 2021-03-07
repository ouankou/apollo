
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <chrono>
#include <memory>
#include <map>
#include <fstream>

#include "apollo/Apollo.h"
#include "apollo/PolicyModel.h"
#include "apollo/TimingModel.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif //ENABLE_MPI

class Apollo::Region {
    public:
        Region(
                const int    num_features,
                const char   *regionName,
                int           numAvailablePolicies,
                Apollo::CallbackDataPool *callbackPool = nullptr,
                const std::string &modelYamlFile="");
        ~Region();

        typedef struct Measure {
            int       exec_count;
            double    time_total;
            Measure(int e, double t) : exec_count(e), time_total(t) {}
        } Measure;

        char     name[64];

        // DEPRECATED interface assuming synchronous execution, will be removed
        void     end();
        // lower == better, 0.0 == perfect
        void     end(double synthetic_duration_or_weight);
        int      getPolicyIndex(void);
        void     setFeature(float value);
        // END of DEPRECATED

        Apollo::RegionContext *begin();
        Apollo::RegionContext *begin(std::vector<float>);
        void end(Apollo::RegionContext *);
        void end(Apollo::RegionContext *, double);
        int  getPolicyIndex(Apollo::RegionContext *);
        void setFeature(Apollo::RegionContext *, float value);

        int idx;
        int      num_features;
        int      reduceBestPolicies(int step);
        //
        // Application specific callback data pool associated with the region, deleted by apollo.
        Apollo::CallbackDataPool *callback_pool;

        std::map<
            std::vector< float >,   // a feature vector -> corresponding best policy and associated execution time
            std::pair< int, double > > best_policies;

        std::map<
            std::pair< std::vector<float>, int >,
            std::unique_ptr<Apollo::Region::Measure> > measures;
        //^--Explanation: < features, policy >, value: < time measurement >

        std::unique_ptr<TimingModel> time_model;
        std::unique_ptr<PolicyModel> model;

        // Support cross execution profling/modeling/adaptation, we need to load previous measures of a region
        // return true if load is successful
        bool loadPreviousMeasures(std::string& prev_data_file); 

    private:
        //
        Apollo        *apollo;
        // DEPRECATED wil be removed
        Apollo::RegionContext *current_context;
        //
        std::ofstream trace_file;

        std::vector<Apollo::RegionContext *> pending_contexts;
        void collectPendingContexts();
        void collectContext(Apollo::RegionContext *, double);

        // save region information into a file, enable cross-execution optimization
        void serialize(int training_steps);

        // load previous saved data file for a region
        bool loadPreviousMeasures(const std::string&); 

}; // end: Apollo::Region

struct Apollo::RegionContext
{
    std::chrono::steady_clock::time_point exec_time_begin;
    std::chrono::steady_clock::time_point exec_time_end;
    std::vector<float> features;
    int policy;
    int idx; // a unique id of the calling times , starting from 0. The count a region context is executed. 
    // Arguments: void *data, bool *returnMetric, double *metric (valid if
    // returnsMetric == true).
    bool (*isDoneCallback)(void *, bool *, double *);
    void *callback_arg;
}; //end: Apollo::RegionContext

struct Apollo::CallbackDataPool {
    virtual ~CallbackDataPool() = default;
    virtual void put(void *) = 0;
    virtual void *get() = 0;
};

#endif
