
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
        Apollo::RegionContext *begin(const std::vector<float>&);
        void end(Apollo::RegionContext *);
        void end(Apollo::RegionContext *, double);
        int  getPolicyIndex(Apollo::RegionContext *);
        void setFeature(Apollo::RegionContext *, float value);

        int idx;
        int num_region_policies;  // region level policies
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

        // set the policy model of this region
        void setPolicyModel (int numAvailablePolicies, const std::string &modelYamlFile);

        // Support cross execution profling/modeling/adaptation, we need to load previous measures of a region
        // return true if load is successful
        // Save measures of the current execution
        void serialize(int training_steps);
        // Load previuos execution's data file
        bool loadPreviousMeasures(); 

        // Per region model building, after checking if we have enough data, if so. 
        void checkAndFlushMeasurements(int step);

        // save region information into a file, enable cross-execution optimization
        std::string getHistoryFilePath();
        std::string getHistoryFileName(bool timeStamp=false);

        std::string getPolicyModelFileName(bool timeStamp=false);
        std::string getTimingModelFileName(bool timeStamp=false);

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

        // check if a region has enough training data collected
        int min_record_count; // a threshold to decide if we have collected enough data
        void setDataCollectionThreshold();
        bool hasEnoughTrainingData();

}; // end: Apollo::Region

struct Apollo::RegionContext
{
    double exec_time_begin;
    double exec_time_end;
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
