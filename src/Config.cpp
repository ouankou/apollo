#include <string>

#include "apollo/Config.h"

int Config::APOLLO_COLLECTIVE_TRAINING;
int Config::APOLLO_LOCAL_TRAINING;
int Config::APOLLO_SINGLE_MODEL;
int Config::APOLLO_REGION_MODEL;
int Config::APOLLO_TRACE_MEASURES;
int Config::APOLLO_NUM_POLICIES;
int Config::APOLLO_TRACE_POLICY;
int Config::APOLLO_RETRAIN_ENABLE;
float Config::APOLLO_RETRAIN_TIME_THRESHOLD;
float Config::APOLLO_RETRAIN_REGION_THRESHOLD;
int Config::APOLLO_STORE_MODELS;
int Config::APOLLO_TRACE_RETRAIN;
int Config::APOLLO_TRACE_ALLGATHER;
int Config::APOLLO_TRACE_BEST_POLICIES;
int Config::APOLLO_FLUSH_PERIOD;
int Config::APOLLO_TRACE_CSV;

int Config::APOLLO_CROSS_EXECUTION;
int Config::APOLLO_CROSS_EXECUTION_MIN_DATAPOINT_COUNT;
int Config::APOLLO_TRACE_CROSS_EXECUTION;

int Config::APOLLO_USE_TOTAL_TIME;

std::string Config::APOLLO_INIT_MODEL;
std::string Config::APOLLO_TRACE_FOLDER_SUFFIX;
