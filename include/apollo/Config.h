#ifndef APOLLO_CONFIG_H
#define APOLLO_CONFIG_H

class Config {
    public:
      // Using MPI to have multiple compute nodes for training
        static int APOLLO_COLLECTIVE_TRAINING;
        // Using a single computation node for traning
        static int APOLLO_LOCAL_TRAINING;

        static int APOLLO_SINGLE_MODEL;  // a single model for all code regions
        static int APOLLO_REGION_MODEL;  // each region has its own model 
        static int APOLLO_NUM_POLICIES;
        static int APOLLO_RETRAIN_ENABLE;
        static float APOLLO_RETRAIN_TIME_THRESHOLD;
        static float APOLLO_RETRAIN_REGION_THRESHOLD;

        static int APOLLO_CROSS_EXECUTION;  // enable cross execution profiling/ modeling/adaptation
        // minimum number of data points for each feature needed to trigger model building
        static int APOLLO_CROSS_EXECUTION_MIN_DATAPOINT_COUNT;  

        // For some training data , we don't average the measures, but use total accumulated time for the current execution
        // This is used for some experiment when a single execution with a given input data will only explore one policy.
        // All executions of the same region should be added into a total execution time, instead of calculating their average values.
        static int APOLLO_USE_TOTAL_TIME; 
        // Debugging and tracing configurations
        static int APOLLO_STORE_MODELS;
        static int APOLLO_TRACE_MEASURES;
        static int APOLLO_TRACE_POLICY;
        static int APOLLO_TRACE_RETRAIN;
        static int APOLLO_TRACE_ALLGATHER;
        static int APOLLO_TRACE_BEST_POLICIES;
        static int APOLLO_TRACE_CROSS_EXECUTION;

        static int APOLLO_FLUSH_PERIOD;
        static int APOLLO_TRACE_CSV;
        static std::string APOLLO_INIT_MODEL; // initial policy selection modes: random, roundRobin, static,0 (always 1st one), etc
        static std::string APOLLO_TRACE_FOLDER_SUFFIX;  // more general trace folder, storing not just csv file

    private:
        Config();
};

#endif
