
// Copyright (c) 2020, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <memory>
#include <utility>
#include <algorithm>
#include <cmath>

#include <sys/types.h>
#include <sys/stat.h>

#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/Region.h"
#include "apollo/Logging.h"
#include "apollo/ModelFactory.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif //ENABLE_MPI

using namespace std;
// get a timestamp for now, "2021-02-19-16:04:40-PST"
static string getTimeStamp ()
{
  time_t now = time(0);
  struct tm * timeinfo = localtime (&now);
  char buffer [30];
  strftime (buffer, 30, "%F-%T-%Z",timeinfo);
  return string(buffer);
}

int
Apollo::Region::getPolicyIndex(Apollo::RegionContext *context)
{
    int choice = model->getIndex( context->features );

    if (Config::APOLLO_TRACE_CROSS_EXECUTION)
    {
      if (model->training)
        cout<<"Region::getPolicyIndex() uses a training model"<<endl;
      else
        cout<<"Region::getPolicyIndex() uses a production model"<<endl;
    }

    if( Config::APOLLO_TRACE_POLICY ) {
        std::stringstream trace_out;
        int rank;
        rank = apollo->mpiRank;
        trace_out << "Rank " << rank \
            << " region " << name \
            << " model " << model->name \
            << " features [ ";
        for(auto &f: context->features)
            trace_out << (int)f << ", ";
        trace_out << "] policy " << choice << std::endl;
        std::cout << trace_out.str();
        std::ofstream fout( "rank-" + std::to_string(rank) + "-policies.txt", std::ofstream::app );
        fout << trace_out.str();
        fout.close();
    }

#if 0
    if (choice != context->policy) {
        std::cout << "Change policy " << context->policy\
            << " -> " << choice << " region " << name \
            << " training " << model->training \
            << " features: "; \
            for(auto &f : apollo->features) \
                std::cout << (int)f << ", "; \
            std::cout << std::endl; //ggout
    } else {
        //std::cout << "No policy change for region " << name << ", policy " << current_policy << std::endl; //gout
    }
#endif
    context->policy = choice;
    //log("getPolicyIndex took ", evaluation_time_total, " seconds.\n");
    return choice;
}

Apollo::Region::Region(
        const int num_features,
        const char  *regionName,
        int          numAvailablePolicies,
        Apollo::CallbackDataPool *callbackPool,
        const std::string &modelYamlFile)
    :
        num_features(num_features), current_context(nullptr), idx(0), callback_pool(callbackPool)
{

    apollo = Apollo::instance();
    if( Config::APOLLO_NUM_POLICIES ) {
        apollo->num_policies = Config::APOLLO_NUM_POLICIES;
    }
    else {
        apollo->num_policies = numAvailablePolicies;
    }
    
  // We need per region policy count
   num_region_policies = numAvailablePolicies; 

    strncpy(name, regionName, sizeof(name)-1 );
    name[ sizeof(name)-1 ] = '\0';

    // if cross-execution training is turned on, we need to set a threshold
    if (Config::APOLLO_CROSS_EXECUTION)
      setDataCollectionThreshold();

    // This first time the region is created, load an available model vs. set an initial model
    setPolicyModel (num_region_policies, modelYamlFile); // refactor the model setting logic into a separated function.

    // Set the current model used to select a policy 
    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    const auto ret = apollo->regions.insert( { name, this } );

    return;
}

// Per region model building, after checking if we have enough data, if so. 
// Only used when cross-execution training is enabled.
void Apollo::Region::checkAndFlushMeasurements(int step)
{
  int rank = apollo->mpiRank; 

  if (!hasEnoughTrainingData()) return; 

  if (Config::APOLLO_TRACE_CROSS_EXECUTION)
    cout<<"Model Building: Apollo::Region::checkAndFlushMeasurements(): has enough training data, build the model and flush measures..."<<endl;
   // must save current measures into a file first. Or we will lose them
  reduceBestPolicies(step);

  // In cross execution mode: we don't want to flush the data accumulated so far. just keep accumulating
//  if (!Config::APOLLO_CROSS_EXECUTION)
    measures.clear();

  //TODO: support MPI?
  /*
    if( Config::APOLLO_COLLECTIVE_TRAINING ) {
        //std::cout << "DO COLLECTIVE TRAINING" << std::endl; //ggout
              gatherReduceCollectiveTrainingData(step);
        }
   */
    std::vector< std::vector<float> > train_features;
    std::vector< int > train_responses;

    std::vector< std::vector< float > > train_time_features;
    std::vector< float > train_time_responses;

   if (model->training && best_policies.size()>0)
   {
     if (Config::APOLLO_TRACE_CROSS_EXECUTION)
       cout<<"Model Building: entering model->training && best_policies.size()>0.."<<endl;
     if( Config::APOLLO_REGION_MODEL ) {
       if (Config::APOLLO_TRACE_CROSS_EXECUTION)
         std::cout << "Model Building: TRAIN MODEL PER REGION, update features using best policies" << std::endl;
       // Reset training vectors
       train_features.clear();
       train_responses.clear();
       train_time_features.clear();
       train_time_responses.clear();

       // Prepare training data
       for(auto &it2 : best_policies) {
         train_features.push_back( it2.first );
         train_responses.push_back( it2.second.first );

         std::vector< float > feature_vector = it2.first;
         feature_vector.push_back( it2.second.first );
         train_time_features.push_back( feature_vector );
         train_time_responses.push_back( it2.second.second );
       }
     }
     else
     {
       if (Config::APOLLO_TRACE_CROSS_EXECUTION)
         std::cout << "Model Building: False for TRAIN MODEL PER REGION" << std::endl;
     }

     if( Config::APOLLO_TRACE_BEST_POLICIES ) {
       std::stringstream trace_out;
       trace_out << "=== Rank " << rank \
         << " BEST POLICIES Region " << name << " ===" << std::endl;
       for( auto &b : best_policies ) {
         trace_out << "[ ";
         for(auto &f : b.first)
           trace_out << (int)f << ", ";
         trace_out << "] P:" \
           << b.second.first << " T: " << b.second.second << std::endl;
       }
       trace_out << ".-" << std::endl;
       std::cout << trace_out.str();
       std::ofstream fout("step-" + std::to_string(step) + \
           "-rank-" + std::to_string(rank) + "-" + name + "-best_policies.txt"); \
         fout << trace_out.str(); \
         fout.close();
     }

     // TODO(cdw): Load prior decisiontree...
     model = ModelFactory::createDecisionTree(
         num_region_policies,
         train_features,
         train_responses );

     time_model = ModelFactory::createRegressionTree(
         train_time_features,
         train_time_responses );

    // For cross-execution model, we always store the trained model for reuse later.
    // if( Config::APOLLO_STORE_MODELS ) 
//       model->store( "dtree-step-" + std::to_string( step ) \
//           + "-rank-" + std::to_string( rank ) \
//           + "-" + name + ".yaml" );

       model->store( getHistoryFilePath()+"/"+getPolicyModelFileName() ) ;

//       time_model->store("regtree-step-" + std::to_string( step ) \
//           + "-rank-" + std::to_string( rank ) \
//           + "-" + name + ".yaml");

       time_model->store(getHistoryFilePath()+"/"+getTimingModelFileName());
     
   }
   else
   {
     if (Config::APOLLO_TRACE_CROSS_EXECUTION)
       cout<<"Model Building: failed to entering if (model->training && best_policies.size()>0). No model is build.."<<endl;
   }
  // TODO: support retrain logic at region level
   best_policies.clear();
}

std::string Apollo::Region::getPolicyModelFileName(bool timeStamp/*=false*/)
{
  int rank = apollo->mpiRank;
  string res= "dtree-latest-rank-" + std::to_string( rank ) + "-" + name + ".yaml";
  return res; 
}

std::string Apollo::Region::getTimingModelFileName(bool timeStamp/*=false*/)
{
  int rank = apollo->mpiRank;
  string res= "regtree-latest-rank-" + std::to_string( rank ) + "-" + name + ".yaml";
  return res; 
}

// set the policy model of this region
/*
 If a yaml file is specified, we load the model from the file

 Otherwise we read the model from environment variable: APOLLO_INIT_MODEL, which can be
    static| Random| RoundRobin or
    Load another yaml file
 
 * */ 
void Apollo::Region::setPolicyModel (int numAvailablePolicies, const std::string &modelYamlFile)
{
  // timestamps are using 1 second as the smallest granularity, which is often not sufficient
  // we additionally add a sequence id to avoid conflicting names generated. 
   static int seq=0;

// TODO: why do we have global policy count??
    if (!modelYamlFile.empty()) {
        model = ModelFactory::loadDecisionTree(apollo->num_policies, modelYamlFile);
    }
    else {
        // TODO use best_policies to train a model for new region for which there's training data
        size_t pos = Config::APOLLO_INIT_MODEL.find(",");
        std::string model_str = Config::APOLLO_INIT_MODEL.substr(0, pos);
        if ("Static" == model_str)
        {
            int policy_choice = std::stoi(Config::APOLLO_INIT_MODEL.substr(pos + 1));
            if (policy_choice < 0 || policy_choice >= numAvailablePolicies)
            {
                std::cerr << "Invalid policy_choice " << policy_choice << std::endl;
                abort();
            }
            model = ModelFactory::createStatic(apollo->num_policies, policy_choice);
            //std::cout << "Model Static policy " << policy_choice << std::endl;
        }
        else if ("Load" == model_str)
        {
            std::string model_file;
            if (pos == std::string::npos)
            {
                // Load per region model using the region name for the model file.
                model_file = "dtree-latest-rank-" + std::to_string(apollo->mpiRank) + "-" + std::string(name) + ".yaml";
            }
            else
            {
                // Load the same model for all regions.
                model_file = Config::APOLLO_INIT_MODEL.substr(pos + 1);
            }
            //std::cout << "Model Load " << model_file << std::endl;
            model = ModelFactory::loadDecisionTree(apollo->num_policies, model_file);
        }
        else if ("Random" == model_str)
        {
            model = ModelFactory::createRandom(apollo->num_policies);
            //std::cout << "Model Random" << std::endl;
        }
        else if ("RoundRobin" == model_str)
        {
            model = ModelFactory::createRoundRobin(apollo->num_policies);
            //std::cout << "Model RoundRobin" << std::endl;
        }
        else
        {
            std::cerr << "Invalid model env var: " + Config::APOLLO_INIT_MODEL << std::endl;
            abort();
        }
    }

    if( Config::APOLLO_TRACE_CSV ) {
        // TODO: assumes model comes from env, fix to use model provided in the constructor
        // TODO: convert to filesystem C++17 API when Apollo moves to it
        int ret;
        ret = mkdir(
            std::string("./trace" + Config::APOLLO_TRACE_FOLDER_SUFFIX)
                .c_str(),
            S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (ret != 0 && errno != EEXIST) {
            perror("TRACE_CSV mkdir");
            abort();
        }
        std::string fname("./trace" + Config::APOLLO_TRACE_FOLDER_SUFFIX +
                          "/trace-" + Config::APOLLO_INIT_MODEL + "-region-" +
                          name + "-rank-" + std::to_string(apollo->mpiRank) + getTimeStamp() +"-"+ to_string(seq++)+
                          ".csv");
        std::cout << "TRACE_CSV fname " << fname << std::endl;
        trace_file.open(fname);
        if(trace_file.fail()) {
            std::cerr << "Error opening trace file " + fname << std::endl;
            abort();
        }
        // Write header.
        trace_file << "rankid,training,region,idx";
        //trace_file << "features";
        for(int i=0; i<num_features; i++)
            trace_file << ",f" << i;
        trace_file << ",policy,xtime\n";
    }
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
  if (Config::APOLLO_TRACE_CROSS_EXECUTION)
  {
      cout<< "Collected "<< measures.size() << " out of " << min_record_count << " required measure count." <<endl;
  }
  return measures.size()>=min_record_count;
}

// Store training datasets into a file
// The data include both  aggregated data (measures) and labeled data (with best policy information)
//
// Empty records may be found, if the execution already collects sufficient data and called model building already
void Apollo::Region::serialize(int training_steps=0)
{

  // do nothing if empty measures. 
  // This happens if the execution already collects sufficient data and called model building.
  if (measures.empty()) return; 

  std::stringstream trace_out;
  string t_stamp=getTimeStamp();
  int rank;
#ifdef ENABLE_MPI
  rank = apollo->mpiRank;
#else
  rank = 0;
#endif //ENABLE_MPI
  trace_out << "========Apollo::Region::serialize()"<< t_stamp<< "================" << std::endl \
    << "Rank " << rank << " Region " << name << " MEASURES "  << std::endl;

  for (auto iter_measure = measures.begin();
      iter_measure != measures.end();   iter_measure++) {

    const std::vector<float>& feature_vector = iter_measure->first.first;
    const int policy_index                   = iter_measure->first.second;
    auto  &time_set = iter_measure->second;

    trace_out << "features: [ ";
    for(auto &f : feature_vector ) { \
      trace_out << (int)f << ", ";
    }
    trace_out << " ]: "
      << "policy: " << policy_index
      << " , count: " << time_set->exec_count
      << " , total: " << time_set->time_total
      << " , time_avg: " <<  ( time_set->time_total / time_set->exec_count ) << std::endl;
    double time_avg = ( time_set->time_total / time_set->exec_count );
  }

  // separator for two portion of data
  trace_out << ".-" << std::endl;

  // serialize the reduced data
  //------------------------
  trace_out << "Rank " << rank << " Region " << name << " Reduce " << std::endl;
  for( auto &b : best_policies ) {
    trace_out << "features: [ ";
    for(auto &f : b.first )
      trace_out << (int)f << ", ";
    trace_out << "]: P:"
      << b.second.first << " T: " << b.second.second << std::endl;
  }
  trace_out << ".-" << std::endl;
  //  std::cout << trace_out.str();

  // save to a file
  string folder_name = getHistoryFilePath(); // "./trace" + Config::APOLLO_TRACE_FOLDER_SUFFIX;

  int ret;
  ret = mkdir(folder_name.c_str(),  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  if (ret != 0 && errno != EEXIST) {
    perror("Apollo::Region::serialize()  mkdir");
    abort();
  }

  // TODO: better file name for region data
  std::ofstream fout( folder_name +'/'+getHistoryFileName()); 
  fout << trace_out.str();
  fout.close();

  //In debugging mode: we also save into timestamped files as history
#if 0  //
  if (Config::APOLLO_CROSS_EXECUTION) 
  {
    std::ofstream fout2( folder_name +'/'+getHistoryFileName(true)); 
    fout2 << trace_out.str();
    fout2.close();
  }
#endif   
}

Apollo::Region::~Region()
{
    // Disable period based flushing.
    Config::APOLLO_FLUSH_PERIOD = 0;
    while(pending_contexts.size() > 0)
       collectPendingContexts();

    if(callback_pool)
        delete callback_pool;

    if( Config::APOLLO_TRACE_CSV )
    {
//        cout<<"Region::~Region() Config::APOLLO_TRACE_CSV==1, closing trace file.."<<endl;
        trace_file.close();
    }
#if 0 // We move this to the destructor of Apollo, calling serialize() only once
    // Save region information into a file
    if (Config::APOLLO_CROSS_EXECUTION)
    {
//      if (Config::APOLLO_TRACE_MEASURES) // we must unconditionally serialize current region's aggregated measures into a file, at least when the execution finishes
//      TODO: reduce overhead, only saving to file once per execution
        serialize(); 
    }
#endif
    return;
}

Apollo::RegionContext *
Apollo::Region::begin()
{
    Apollo::RegionContext *context = new Apollo::RegionContext();
    current_context = context;
    context->idx = this->idx;
    this->idx++;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    context->exec_time_begin = ts.tv_sec + ts.tv_nsec/1e9;

    context->isDoneCallback = nullptr;
    context->callback_arg = nullptr;
    return context;
}

//! this this the default begin() of a region
// It also tries to load models, if they are available
// No: model is set to region in memory, if it is built during this current run (by some previous iteration's region->end())
Apollo::RegionContext *
Apollo::Region::begin(const std::vector<float>& features)
{
    Apollo::RegionContext *context = begin();
    context->features = features;
    return context;
}

void
Apollo::Region::collectContext(Apollo::RegionContext *context, double metric)
{
  // std::cout << "COLLECT CONTEXT " << context->idx << " REGION " << name \
            << " metric " << metric << std::endl;
  auto iter = measures.find({context->features, context->policy});
  if (iter == measures.end()) {
    iter = measures
               .insert(std::make_pair(
                   std::make_pair(context->features, context->policy),
                   std::move(
                       std::make_unique<Apollo::Region::Measure>(1, metric))))
               .first;
    } else {
        iter->second->exec_count++;
        iter->second->time_total += metric;
    }

    if( Config::APOLLO_TRACE_CSV ) {
        trace_file << apollo->mpiRank << ",";
        trace_file << Config::APOLLO_INIT_MODEL << ",";
        trace_file << this->name << ",";
        trace_file << context->idx << ",";
        for(auto &f : context->features)
            trace_file << setprecision(15)<<f << ",";
        trace_file << context->policy << ",";
        trace_file << metric << "\n";
    }

    apollo->region_executions++;

    if( Config::APOLLO_FLUSH_PERIOD && ( apollo->region_executions%Config::APOLLO_FLUSH_PERIOD ) == 0 ) {
        //std::cout << "FLUSH PERIOD! region_executions " << apollo->region_executions<< std::endl; //ggout
        apollo->flushAllRegionMeasurements(apollo->region_executions);
    }

    delete context;
    current_context = nullptr;
}

void
Apollo::Region::end(Apollo::RegionContext *context, double metric)
{
    //std::cout << "END REGION " << name << " metric " << metric << std::endl;

    collectContext(context, metric);

    collectPendingContexts();

    return;
}

void Apollo::Region::collectPendingContexts() {
  auto isDone = [this](Apollo::RegionContext *context) {
    bool returnsMetric;
    double metric;
    if (context->isDoneCallback(context->callback_arg, &returnsMetric, &metric)) {
      if (returnsMetric)
        collectContext(context, metric);
      else {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        context->exec_time_end = ts.tv_sec + ts.tv_nsec/1e9;
        double duration = context->exec_time_end - context->exec_time_begin;
        collectContext(context, duration);
      }
      return true;
    }

    return false;
  };

  pending_contexts.erase(
      std::remove_if(pending_contexts.begin(), pending_contexts.end(), isDone),
      pending_contexts.end());
}

void
Apollo::Region::end(Apollo::RegionContext *context)
{
    if(context->isDoneCallback)
        pending_contexts.push_back(context);
    else {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      context->exec_time_end = ts.tv_sec + ts.tv_nsec/1e9;
      double duration = context->exec_time_end - context->exec_time_begin;
      collectContext(context, duration);
    }

    collectPendingContexts();
}


// DEPRECATED
int
Apollo::Region::getPolicyIndex(void)
{
    return getPolicyIndex(current_context);
}

// DEPRECATED
void
Apollo::Region::end(double metric)
{
    end(current_context, metric);
}

// DEPRECATED
void
Apollo::Region::end(void)
{
//TODO: test this in synchronous processing first, move to asynchronous handling later.
   if (Config::APOLLO_CROSS_EXECUTION)
   {
     if (model->training) // only do this during training session. Skip in production session
       if (!Config::APOLLO_USE_TOTAL_TIME) // only if not using total execution time, we can trigger model building in the middle of execution
         checkAndFlushMeasurements(idx);
   }
   end(current_context);
}

int
Apollo::Region::reduceBestPolicies(int step)
{
    std::stringstream trace_out;
    int rank;
    if( Config::APOLLO_TRACE_MEASURES) {
#ifdef ENABLE_MPI
        rank = apollo->mpiRank;
#else
        rank = 0;
#endif //ENABLE_MPI
        trace_out << "=================================" << std::endl \
            << "Rank " << rank << " Region " << name << " MEASURES "  << std::endl;
    }
    for (auto iter_measure = measures.begin();
            iter_measure != measures.end();   iter_measure++) {

        const std::vector<float>& feature_vector = iter_measure->first.first;
        const int policy_index                   = iter_measure->first.second;
        auto                           &time_set = iter_measure->second;

        if( Config::APOLLO_TRACE_MEASURES ) {
            trace_out << "features: [ ";
            for(auto &f : feature_vector ) { \
                trace_out << (int)f << ", ";
            }
            trace_out << " ]: "
                << "policy: " << policy_index
                << " , count: " << time_set->exec_count
                << " , total: " << time_set->time_total
                << " , time_avg: " <<  ( time_set->time_total / time_set->exec_count ) << std::endl;
        }
        

        // we now support two kinds of time features: total accumulated time vs. average time (default)
        double final_time ; 
        
        if (Config::APOLLO_USE_TOTAL_TIME) 
        {
            final_time = time_set->time_total; 
        }
        else
        {
           double time_avg = ( time_set->time_total / time_set->exec_count );
           final_time = time_avg; 
        }
        
        auto iter =  best_policies.find( feature_vector );
        if( iter ==  best_policies.end() ) {
            best_policies.insert( { feature_vector, { policy_index, final_time } } );
        }
        else {
            // Key exists, update only if we find better choices
            if(  best_policies[ feature_vector ].second > final_time ) {
                best_policies[ feature_vector ] = { policy_index, final_time };
            }
        }
    }

    if( Config::APOLLO_TRACE_MEASURES ) {
        trace_out << ".-" << std::endl;
        trace_out << "Rank " << rank << " Region " << name << " Reduce " << std::endl;
        for( auto &b : best_policies ) {
            trace_out << "features: [ ";
            for(auto &f : b.first )
                trace_out << (int)f << ", ";
            trace_out << "]: P:"
                << b.second.first << " T: " << b.second.second << std::endl;
        }
        trace_out << ".-" << std::endl;
        std::cout << trace_out.str();
        // store the file under the trace CSV folder
        string folder_name = getHistoryFilePath(); // "./trace" + Config::APOLLO_TRACE_FOLDER_SUFFIX;
        int ret;
        ret = mkdir(folder_name.c_str(),  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        if (ret != 0 && errno != EEXIST) {
          perror("Apollo::Region::reduceBestPolicies()  mkdir");
          abort();
        }

        string file_name_with_path = folder_name + '/' + "reduceBestPolicies-step-" + std::to_string(step) +
                          "-rank-" + std::to_string(rank) + "-" + name + "-measures.txt";
        cout<<"Apollo::Region::reduceBestPolicies() saving measures and labelled data into file:"<< file_name_with_path<<endl;
        std::ofstream fout(file_name_with_path);
        fout << trace_out.str();
        fout.close();
    }

    return best_policies.size();
}

void
Apollo::Region::setFeature(Apollo::RegionContext *context, float value)
{
    context->features.push_back(value);
    return;
}

// DEPRECATED
void
Apollo::Region::setFeature(float value)
{
    setFeature(current_context, value);
}

// A helper function to extract numbers from a line
// from pos, to next ',' extract a cell, return the field, and new pos
static string extractOneCell(string& line, size_t& pos)
{
  size_t end=pos; 
  while (end<line.size() && line[end]!=',')
    end++; 

  // now end is end or , 
  string res= line.substr(pos, end-pos);
  //  cout<<"start pos:"<<pos << "--end pos:"<< end <<endl;
  pos=end+1; 
  return res; 
}
std::string 
Apollo::Region::getHistoryFilePath()
{
  return "./trace" + Config::APOLLO_TRACE_FOLDER_SUFFIX;
}

// get previous exeuction's aggregated measures for a region specified by its name.
std::string 
Apollo::Region::getHistoryFileName(bool timestime)
{
   int rank = apollo->mpiRank;  //Automatically 0 if not an MPI environment.
   string t_str;
  if (timestime)
   t_str= getTimeStamp();
  return "Region-Data-rank-" + std::to_string(rank) + "-" + name + "-measures"+ t_str+ ".txt";
}
/*

Current data dump of measures has the following content
------begin of the file
=================================
Rank 0 Region daxpy MEASURES 
features: [ 9027,  ]: policy: 1 , count: 1 , total: 0.000241501 , time_avg: 0.000241501
features: [ 38863,  ]: policy: 0 , count: 1 , total: 0.00714597 , time_avg: 0.00714597
features: [ 61640,  ]: policy: 0 , count: 1 , total: 0.00875109 , time_avg: 0.00875109
features: [ 148309,  ]: policy: 0 , count: 1 , total: 0.00658767 , time_avg: 0.00658767
features: [ 240700,  ]: policy: 0 , count: 1 , total: 0.00978215 , time_avg: 0.00978215
features: [ 366001,  ]: policy: 1 , count: 1 , total: 0.00142485 , time_avg: 0.00142485
..
.-  
Rank 0 Region daxpy Reduce 
features: [ 9027, ]: P:1 T: 0.000241501
features: [ 38863, ]: P:0 T: 0.00714597
 
---------end of file
We only need to read in the first portion to populate the map of feature vector+policy -> measure pair<count, total>

std::map< std::pair< std::vector<float>, int >, std::unique_ptr<Apollo::Region::Measure> > measures;

 * */
bool
Apollo::Region::loadPreviousMeasures()
{
  string prev_data_file = getHistoryFilePath()+'/'+getHistoryFileName();

  ifstream datafile (prev_data_file.c_str(), ios::in);

  if (!datafile.is_open())
  {
    if (Config::APOLLO_TRACE_CROSS_EXECUTION)
      cout<<"Apollo::Region::loadPreviousMeasures() cannot find/open txt file "<< prev_data_file<<endl;
    return false;
  } 

  if (Config::APOLLO_TRACE_CROSS_EXECUTION)
    cout<<"loadPreviousMeasures() opens previous measure data file successfully: "<< prev_data_file <<endl;

  while (!datafile.eof())
  {
    string line;
    getline(datafile, line);
    if (line==".-")
    {
      // cout<<"Found end line, stopping.."<<endl;
      break;
    }

    // we have the line in a string anyway, just use it.
    size_t pos=0; 
    while (pos<line.size()) 
    {
      pos=line.find('[', pos);
      if (pos==string::npos) // a line without [ 
        break; 

      pos++; // skip [
      size_t end_bracket_pos=line.find(']', pos);
      if (end_bracket_pos==string::npos) // something is wrong
      {
        cerr<<"Error in Apollo::Region::loadPreviousMeasures(). cannot find ending ] in "<< line <<endl;
        return false; 
      }

      // extract one or more features, separated by ',' between pos and end_bracket_pos
      size_t end=pos;
      vector<float> cur_features; 
      while (pos<end_bracket_pos)
      {
        string cell=extractOneCell(line, pos); 
        // may reaching over ] , skip them
        // only keep one extracted before ']'
        if (pos < end_bracket_pos)
        {
          //  cout<<stoi(cell) << " " ; // convert to integer, they are mostly sizes of things right now
          cur_features.push_back(stof(cell));
        }
        else
          break; 
      }

      // extract policy
      int cur_policy;
      pos = line.find("policy:", end_bracket_pos);
      if (pos==string::npos)
        return false;
      pos+=7;
      string policy_id=extractOneCell(line, pos);
      //cout<<"policy:"<<stoi(policy_id) <<" "; // convert to integer, they are mostly sizes of things right now
      cur_policy = stoi(policy_id);

      // extract count
      int cur_count; 
      pos = line.find("count:", end_bracket_pos);
      if (pos==string::npos)
        return false;
      pos+=6;
      string count=extractOneCell(line, pos);
      //cout<<"count:"<<stoi(count) <<" "; // convert to integer, they are mostly sizes of things right now
      cur_count = stoi(count);

      //extract total execution time
      pos = line.find("total:", end_bracket_pos);
      if (pos==string::npos)
        return false;
      pos+=6;
      string total=extractOneCell(line, pos);
      // cout<<"total exe time:"<<stof(total) <<endl; // convert to integer, they are mostly sizes of things right now
      float cur_total_time = stof(total); 


      auto iter = measures.find({cur_features, cur_policy});
      if (iter == measures.end()) {
        iter = measures
          .insert(std::make_pair(
                std::make_pair(cur_features, cur_policy),
                std::move(
                  std::make_unique<Apollo::Region::Measure>(cur_count, cur_total_time))))
          .first;
      } else {
        // we dont' really expect anything going this branch
        // the previous training data should already be aggregated data. 
        assert (false);
        iter->second->exec_count+=cur_count;
        iter->second->time_total += cur_total_time;
      }
    } // hande a line 
  } // end while

  if (Config::APOLLO_TRACE_CROSS_EXECUTION)
      cout<<"After insertion: measures.size():"<<measures.size() <<endl;

  datafile.close();

  return true;
}
