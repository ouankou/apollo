
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <chrono>
#include <memory>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

#include <mpi.h>

class Apollo::Region {
    public:
        Region(
                const int    num_features,
                const char   *regionName,
                int           numAvailablePolicies);
        ~Region();

        typedef struct Measure {
            int       exec_count;
            double    time_total;
            Measure(int e, double t) : exec_count(e), time_total(t) {}
            //~Measure() { std::cout << "Destruct measure" << std::endl; }
        } Measure;

        char    name[64];

        void     begin();
        void     end();

        int                   getPolicyIndex(void);

        // Key: < features, policy >, value: < time measurement >
        std::map< std::pair< std::vector<float>, int >, std::unique_ptr<Apollo::Region::Measure> > measures;

        int            current_policy;

        int reduceBestPolicies();
        void packMeasurements(char *buf, int size, MPI_Comm comm);
        std::map< std::vector< float >, std::pair< int, double > > best_policies;
        //
#if APOLLO_GLOBAL_MODEL
        std::shared_ptr<Model> model;
#else
        std::unique_ptr<Model> model;
#endif

    private:
        //
        Apollo        *apollo;
        bool           currently_inside_region;
        //
        std::chrono::steady_clock::time_point current_exec_time_begin;
        std::chrono::steady_clock::time_point current_exec_time_end;
        //
}; //end: Apollo::Region


#endif
