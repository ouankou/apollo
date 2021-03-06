# Key Concepts

All training data is associated with a code region (Apollo:Region)

A machine learning model is trained for a list of features to predict either the best policy or code variant choice (classification model) or execution time (regression modeling).

# Context of a measured executime time

A code region may contain several code variants (one policy id for each)

Each measured execution time is tied to 
* the activateed policy choice, or code variant choice

It is also associated with a feature vector values. 

A same code region+ same policy + same feature vector value  may repeat multiple times during execution

example
* code region 1, choosing serial execution policy 1,  with a feature vector of size 1 storing iteration size. 

As a result, an aggregate variable {execution_count, time_total} is used to store all appearance of the unique code region+policy+feature vector. 

```

A measure= execution_count + time_total // aggregate variable

map < pair <vector<float>, int> , unique_ptr<Apollo::Region::Measure> > measures

```
# Actual measure of start and end time

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

# Postprocessing of raw measurements

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
