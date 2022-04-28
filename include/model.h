#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <opencv4/opencv2/core/mat.hpp>
#include <vector>
#include <memory>
#include <opencv4/opencv2/core.hpp>
using namespace::cv;
using namespace::std;

class Model {
public:
    Model(){};
    Model(float xBaseRadius,float yBaseRadius, int nopOfRadius,float extendLength, float height,int nopOfExtend,float startAngle,float angle, int nopOfArc, int nopOfFusion=1,float power=3);
    ~Model();
    vector<Point3f> getPositions(){return positions;};
    vector<unsigned int> getIndexes(){return indexes;}
    vector<float> getAlphas(){return alphas;}

private:
    void compute();
    float xBaseRadius,yBaseRadius,startAngle,angle,extendLength,height,power;
    int nopOfRadius,nopOfExtend,nopOfArc,nopOfFusion;
    vector<Point3f> positions;
    vector<float> alphas;
    vector<unsigned int> indexes; 
};


#endif 