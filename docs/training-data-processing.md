This document follows the training datasets to explain the pipeline of data generation, aggregation, postprocessing, and model building in Apollo. 

# Key Concepts

All training data is associated with a code region (Apollo:Region), often a parallelized OpenMP loop.

Machine learning is used to use a list of features to predict 
* either the best policy (code variant) choice: a classification model 
* or execution time: a regression modeling

# Code Region Instrumentation

To enable adaptation of an OpenMP loop, the code needs to be transformed to have several variants and controlled by Apollo APIs. 

One example is https://github.com/chunhualiao/apollo/blob/develop/test/daxpy-v3.cpp

```
// step 1: add headers 

#include <apollo/Apollo.h>
#include <apollo/Region.h>


  // skeleton of some iterative algorithm
  for(int j=0; j<ITERS; j++) {
  
  
    size_t test_size = size;

    //step 2. Create Apollo region if needed
    Apollo::Region *region = Apollo::instance()->getRegion(
        /* id */ "daxpy",
        /* NumFeatures */ 1,
        /* NumPolicies */ 2);

    //step 3. Begin region and set feature vector's value
    // Feature vector has only 1 element for this example
    region->begin( /* feature vector */ { (float)test_size } );

    // Get the policy to execute from Apollo
    int policy = region->getPolicyIndex();

    switch(policy) {
      case 0: daxpy_cpu(A, x, y, r, test_size); break;
      case 1: daxpy_gpu(A, x, y, r, test_size); break;
      default: assert("invalid policy\n");
    }

    //step 4.  End region execution
    region->end();

  }
  
```


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

A measure= execution_count plus time_total // aggregate variable

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

# Checking if Sufficient Data is collected

This is done within 


```
void Apollo::Region::checkAndFlushMeasurements(int step)
{
  int rank = apollo->mpiRank; 

  if (!hasEnoughTrainingData()) return; 
  //..
  reduceBestPolicies(step);
}


/*
We use a simple threshold for now.

Assuming the following parameters
1. Feature vector size: F_size
2. Each feature: we collect least point_count
3. Policy count: policy_count

Total record count in measures would be:
Record_count = power (point_count, F_size) * policy_count

Record_threshhold= power(Minimum_point_count, F_size)* policy_count

Example: 
 * Min_Point_Count is set to 25
 * num_features: 1
 * num_region_policies: 3
 * min_record_count = 25^1 *3 = 75 
 
TODO: This is exponential complexity. 
 * */

void Apollo::Region::setDataCollectionThreshold()
{
  // A threshold to check if enough data is collected for a region
  const int Min_Point_Count=Config::APOLLO_CROSS_EXECUTION_MIN_DATAPOINT_COUNT;
  min_record_count = min(50000, (int)pow(Min_Point_Count,num_features )* num_region_policies);
  if (Config::APOLLO_TRACE_CROSS_EXECUTION)
    cout<<"min_record_count="<<min_record_count<<endl;
}


bool Apollo::Region::hasEnoughTrainingData()
{
  return measures.size()>=min_record_count;
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

A new option has been introduced :  APOLLO_USE_TOTAL_TIME 

This is used for some experiment when a single execution with a given input data will only explore one policy.  All executions of the same region should be added into a total execution time, instead of calculating their average values.


```
diff --git a/src/Region.cpp b/src/Region.cpp
index 3bb6f21..d0285dc 100644
--- a/src/Region.cpp
+++ b/src/Region.cpp
@@ -660,16 +660,29 @@ Apollo::Region::reduceBestPolicies(int step)
                 << " , total: " << time_set->time_total
                 << " , time_avg: " <<  ( time_set->time_total / time_set->exec_count ) << std::endl;
         }
-        double time_avg = ( time_set->time_total / time_set->exec_count );
+        
 
+        // we now support two kinds of time features: total accumulated time vs. average time (default)
+        double final_time ; 
+        
+        if (Config::APOLLO_USE_TOTAL_TIME) 
+        {
+            final_time = time_set->time_total; 
+        }
+        else
+        {
+           double time_avg = ( time_set->time_total / time_set->exec_count );
+           final_time = time_avg; 
+        }
+        
         auto iter =  best_policies.find( feature_vector );
         if( iter ==  best_policies.end() ) {
-            best_policies.insert( { feature_vector, { policy_index, time_avg } } );
+            best_policies.insert( { feature_vector, { policy_index, final_time } } );
         }
         else {
             // Key exists, update only if we find better choices
-            if(  best_policies[ feature_vector ].second > time_avg ) {
-                best_policies[ feature_vector ] = { policy_index, time_avg };
+            if(  best_policies[ feature_vector ].second > final_time ) {
+                best_policies[ feature_vector ] = { policy_index, final_time };
             }
         }
     }
```
# Feed Labeled Training Data to Machine Model Builders

For each region, just obtain the labelled data and call ModelFactory interface functions

```
void Apollo::Region::checkAndFlushMeasurements(int step)
{
...
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
//...                    
}
```
