#ifndef APOLLO_MODELS_REGRESSIONTREE_H
#define APOLLO_MODELS_REGRESSIONTREE_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include <string>
#include <vector>

#include <opencv2/ml.hpp>
using namespace cv;
using namespace cv::ml;

#include "apollo/TimingModel.h"

namespace Apollo {

class RegressionTree : public TimingModel {

    public:
        RegressionTree(std::vector< std::vector<float> > &features, std::vector<float> &responses);

        ~RegressionTree();

        double getTimePrediction(std::vector<float> &features);
        void store(const std::string &filename);

    private:
        // Ptr<DTrees> dtree;
        Ptr<RTrees> dtree;
        //Ptr<KNearest> dtree;
        //Ptr<Boost> dtree;
        //Ptr<ANN_MLP> dtree;
        //Ptr<LogisticRegression> dtree;
}; //end: RegressionTree (class)


}; //end: Apollo (namespace)

#endif
