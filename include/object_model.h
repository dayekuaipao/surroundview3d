#ifndef OBJECT_MODEL_H
#define OBJECT_MODEL_H
#include <opencv4/opencv2/core/mat.hpp>
#include <vector>
using namespace::cv;
using namespace::std;

class ObjectModel {
public:
    vector<Point3f>& getPositions(){return positions;};
    vector<unsigned int>& getIndexes(){return indexes;}
    virtual ~ObjectModel()=default;
protected:
    vector<Point3f> positions;
    vector<unsigned int> indexes; 
private:
    virtual void compute()=0;

};


#endif 