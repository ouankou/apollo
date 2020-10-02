#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#ifndef APOLLO_MACROS_H
// Apollo header files should always begin with this, to provide
// universal override capability regardless of which header files
// a user includes, and which order they include them.
#include "apollo/Macros.h"
#endif

#include <string>
#include <vector>

#include "apollo/PolicyModel.h"
#include <opencv2/ml.hpp>

using namespace cv;
using namespace cv::ml;

namespace Apollo {

class DecisionTree : public PolicyModel {

    public:
        DecisionTree(int num_policies, std::vector< std::vector<float> > &features, std::vector<int> &responses);
        DecisionTree(int num_policies, std::string path);

        ~DecisionTree();

        int  getIndex(void);
        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename);
        void load(const std::string &filename);

    private:
        //Ptr<DTrees> dtree;
        Ptr<RTrees> dtree;
        //Ptr<SVM> dtree;
        //Ptr<NormalBayesClassifier> dtree;
        //Ptr<KNearest> dtree;
        //Ptr<Boost> dtree;
        //Ptr<ANN_MLP> dtree;
        //Ptr<LogisticRegression> dtree;
}; //end: DecisionTree (class)

}; //end: Apollo (namespace)

#endif
