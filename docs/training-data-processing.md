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

For example
* Code region 1: choosing serial execution policy 1,  with a feature vector of size 1 (storing a loop iteration size of the code region, with value of 100). 
* This region may be executed multiple times , with policy 1 and loop iteration size 100. 
 
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

Raw metrics of a run of a program can be saved into a csv log file, with the following columns 
* rankid: MPI rank id, always 0 if MPI is not enabled
* training: the Apollo's method of going through different code variants, including Random, RoundRobin, or Static. 
* region: the unique Apollo region id
* idx: a serial number starting from 0 for all records. Each measure will generate one record
* f0,f1, ...: list of features in the feature vector. 
* policy: the code variant ID or policy ID of this region being measured
* xtime: execution time of this region with this policy ID, in seconds. 

For example trace-RoundRobin-region-mm-rank-02021-04-27-10:47:50-PDT-0.csv has content of 
```
rankid,training,region,idx,f0,policy,xtime
0,RoundRobin,mm,0,5376,0,47.322892876
0,RoundRobin,mm,1,5376,1,48.207636252
0,RoundRobin,mm,2,5376,0,49.565180368
0,RoundRobin,mm,3,5376,1,48.260190565
```
The file stores a list of execution time for MPI rank id 0, using Apollo's RoundRobin method to go through all variables/policies, for a region named mm (matrix multiplication kernel), feature vector has value of <5376>, policies 0 or 1. 


The aggregated measures for each region can be saved into a log file. One example file is 
Region-Data-rank-0-smith-waterman-measures2021-07-09-12:28:12-PDT.txt , with the following content (partial only).

The file shows that the feature vector has only one element, there are three policies.  For each value of the vector, there are three entries (line 3-5 for feature vector <63>).  Each policy of this feature value runs multiple times (65 times in this example). The total field indicate the accumulated total time, time_avg is the average execution time. 

```
  1 ========Apollo::Region::serialize()2021-07-09-12:28:12-PDT================
  2 Rank 0 Region smith-waterman MEASURES
  3 features: [ 63,  ]: policy: 0 , count: 65 , total: 0.00018966 , time_avg: 2.91785e-06
  4 features: [ 63,  ]: policy: 1 , count: 65 , total: 0.0124016 , time_avg: 0.000190794
  5 features: [ 63,  ]: policy: 2 , count: 65 , total: 0.00450337 , time_avg: 6.92826e-05
  6 features: [ 1087,  ]: policy: 0 , count: 1089 , total: 0.0101418 , time_avg: 9.31295e-06
  7 features: [ 1087,  ]: policy: 1 , count: 1089 , total: 0.257569 , time_avg: 0.000236519
  8 features: [ 1087,  ]: policy: 2 , count: 1089 , total: 0.028062 , time_avg: 2.57686e-05
  9 features: [ 2111,  ]: policy: 0 , count: 2113 , total: 0.0709318 , time_avg: 3.35692e-05
 10 features: [ 2111,  ]: policy: 1 , count: 2113 , total: 0.257291 , time_avg: 0.000121766
 11 features: [ 2111,  ]: policy: 2 , count: 2113 , total: 0.0548461 , time_avg: 2.59565e-05
 12 features: [ 3135,  ]: policy: 0 , count: 3137 , total: 0.0978315 , time_avg: 3.11863e-05
 13 features: [ 3135,  ]: policy: 1 , count: 3137 , total: 0.327007 , time_avg: 0.000104242
 14 features: [ 3135,  ]: policy: 2 , count: 3137 , total: 0.0896911 , time_avg: 2.85914e-05
 15 features: [ 4159,  ]: policy: 0 , count: 4161 , total: 0.118588 , time_avg: 2.84999e-05
 16 features: [ 4159,  ]: policy: 1 , count: 4161 , total: 0.393155 , time_avg: 9.44857e-05
 17 features: [ 4159,  ]: policy: 2 , count: 4161 , total: 0.173163 , time_avg: 4.16157e-05

```


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
