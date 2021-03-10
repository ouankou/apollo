This document follows the training datasets to explain the pipeline of data generation, aggregation, postprocessing, and model building in Apollo. 

# Key Concepts

All training data is associated with a code region (Apollo:Region), often a parallelized OpenMP loop.

Machine learning is used to use a list of features to predict 
* either the best policy (code variant) choice: a classification model 
* or execution time: a regression modeling

# Context of a measured executime time

A code region may contain several code variants (one policy id for each)

Each measured execution time is tied to 
* the activated policy choice for this execution, or the code variant choice

It is also associated with a feature vector's values. A simplest feature vector can have a length of 1 and store the count of loop iteration. 

# Measuring the start and end time of a region

Timing happens within Region::begin() and Region::end(). 

For example
```
void
Apollo::Region::end(Apollo::RegionContext *context)
{
    if(context->isDoneCallback)
        pending_contexts.push_back(context);
    else {
      context->exec_time_end = std::chrono::steady_clock::now();   // get the time here
      double duration = std::chrono::duration<double>(context->exec_time_end -
                                                      context->exec_time_begin)
                            .count();
      collectContext(context, duration);
    }

    collectPendingContexts();
}
```

# Raw Metrics and Aggregated Measures

A same code region+ same policy + same feature vector value  may repeat multiple times during an execution. 
* the primary key for execution timing records = (feature-vector, policy)

Example
* Code region 1: choosing serial execution policy 1,  with a feature vector of size 1 (storing iteration size of the code region). 

As a result, an aggregate result {execution_count, time_total} is used to store all appearance of the unique code region+policy+feature vector. 

A measure= execution_count + time_total // aggregate variable

```

typedef struct Measure {
            int       exec_count;
            double    time_total;
            Measure(int e, double t) : exec_count(e), time_total(t) {}
} Measure;
        
map < pair <vector<float>, int> , unique_ptr<Apollo::Region::Measure> > measures;

```

TODO: std::map<> has O(logN) complexity for insertion operations.  We may need to use unordered_map<> with O(1) instead. 


## Cross-Exection Data Collection and Merging

This is a new feature controlled by Config::APOLLO_CROSS_EXECUTION. Aggregated measures of each code region can be saved into a text file when a monitored program terminates.  The file will also be loaded next time the program starts. 

Saving happens in Apollo::Region::~Region()
```
    // Save region information into a file
    if (Config::APOLLO_CROSS_EXECUTION)
      serialize();

```

Saved data has the following content:

```
Rank 0 Region daxpy MEASURES
features: [ 10000000,  ]: policy: 0 , count: 10 , total: 0.0603572 , time_avg: 0.00603572
features: [ 10000000,  ]: policy: 1 , count: 10 , total: 1.27882 , time_avg: 0.127882
```

Loading happens in Apollo::Region* Apollo::getRegion(). This is the new recommended interface to create all regions also. 

```
Apollo::Region* Apollo::getRegion (const std::string& region_name, int feature_count, int policy_count)
{
  if (regions.count(region_name))
    return regions[region_name];

  Apollo::Region* region = new Apollo::Region (feature_count, region_name.c_str(), policy_count);
  regions[region_name] = region;

  // Load previous execution's data
  if (Config::APOLLO_CROSS_EXECUTION) 
    region->loadPreviousMeasures();

  return regions[region_name];
}

```

# Postprocessing and Labeling of Aggregated measurements

Raw timing information need to be calculated for average values , and labeled with best policies for each feature vectors. 

This happens in Apollo:Region::reduceBestPolicies()

```
 for (auto iter_measure = measures.begin();
            iter_measure != measures.end();   iter_measure++) {

        const std::vector<float>& feature_vector = iter_measure->first.first;
        const int policy_index                   = iter_measure->first.second;
        auto                           &time_set = iter_measure->second;

// calculating average execution time across multiple invoking of the context

        double time_avg = ( time_set->time_total / time_set->exec_count );
        
// code updating best policies. 

        auto iter =  best_policies.find( feature_vector );
        if( iter ==  best_policies.end() ) {
            best_policies.insert( { feature_vector, { policy_index, time_avg } } );
        }
        else {
            // Key exists
            if(  best_policies[ feature_vector ].second > time_avg ) {
                best_policies[ feature_vector ] = { policy_index, time_avg };
            }
        }        
```
# Feed Labeled Training Data to Machine Model Builders

For each region, just obtain the labelled data and call ModelFactory interface functions

```
// for classification model: two vectors
    std::vector< std::vector<float> > train_features;  // feature vectors
    std::vector< int > train_responses;          // best policy choice for the vector


// for regression model: another two vectors
    std::vector< std::vector< float > > train_time_features;
    std::vector< float > train_time_responses;


       if( reg->model->training && reg->best_policies.size() > 0 ) {
            if( Config::APOLLO_REGION_MODEL ) {
                //std::cout << "TRAIN MODEL PER REGION" << std::endl;
                // Reset training vectors
                train_features.clear();
                train_responses.clear();
                train_time_features.clear();
                train_time_responses.clear();

                // Prepare training data
                for(auto &it2 : reg->best_policies) {
                    train_features.push_back( it2.first );
                    train_responses.push_back( it2.second.first );

                    std::vector< float > feature_vector = it2.first;
                    feature_vector.push_back( it2.second.first );
                    train_time_features.push_back( feature_vector );
                    train_time_responses.push_back( it2.second.second );
                }
            }

...
//feed training data sets into the model builder 

            // TODO(cdw): Load prior decisiontree...
            reg->model = ModelFactory::createDecisionTree(
                    num_policies,
                    train_features,
                    train_responses );

            reg->time_model = ModelFactory::createRegressionTree(
                    train_time_features,
                    train_time_responses );
```
