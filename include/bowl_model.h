#ifndef BOWL_MODEL_H
#define BOWL_MODEL_H


#include <iostream>
#include <opencv4/opencv2/core/mat.hpp>
#include <vector>
#include <memory>
#include <opencv4/opencv2/core.hpp>
#include "model.h"
using namespace::cv;
using namespace::std;

class BowlModel:public Model {
public:
    BowlModel(float xBaseRadius,float yBaseRadius, int nopOfRadius,
        float extendLength, float height,int nopOfExtend,
        float startAngle,float angle, int nopOfArc, 
        int nopOfFusion=1,float power=3):
    xBaseRadius(xBaseRadius),yBaseRadius(yBaseRadius),nopOfRadius(nopOfRadius),
    extendLength(extendLength),height(height),nopOfExtend(nopOfExtend),
    startAngle(startAngle),angle(angle),nopOfArc(nopOfArc),
    nopOfFusion(nopOfFusion),power(power){compute();}
    vector<float>& getAlphas(){return alphas;}
    ~BowlModel()=default;

private:
    float xBaseRadius,yBaseRadius,startAngle,angle,extendLength,height,power;
    int nopOfRadius,nopOfExtend,nopOfArc,nopOfFusion;
    vector<float> alphas;
    void compute();
};


#endif 