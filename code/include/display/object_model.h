#ifndef OBJECT_MODEL_H
#define OBJECT_MODEL_H
#include <opencv2/opencv.hpp>
#include <vector>
using namespace::cv;
using namespace::std;

class ObjectModel {
public:
    const vector<Point3f>& getPositions()const {return positions;};
    const vector<unsigned int>& getIndexes()const {return indexes;}
    const vector<float>& getAlphas()const {return alphas;}
protected:
    vector<Point3f> positions;
    vector<unsigned int> indexes; 
    vector<float> alphas;
private:
    virtual void compute()=0;

};


#endif 