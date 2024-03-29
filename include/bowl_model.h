#ifndef BOWL_MODEL_H
#define BOWL_MODEL_H


#include <iostream>
#include <opencv4/opencv2/core/mat.hpp>
#include <vector>
#include <memory>
#include <opencv4/opencv2/core.hpp>
#include "object_model.h"
using namespace::cv;
using namespace::std;

class BowlModel:public ObjectModel {
public:
    BowlModel(float xBaseRadius,float yBaseRadius, int nopOfRadius,
        float extendLength, float height,int nopOfExtend,
        float startAngle,float angle, int nopOfArc, 
        int nopOfFusion=1,float power=3):
    xBaseRadius(xBaseRadius),yBaseRadius(yBaseRadius),nopOfRadius(nopOfRadius),
    extendLength(extendLength),height(height),nopOfExtend(nopOfExtend),
    startAngle(startAngle),angle(angle),nopOfArc(nopOfArc),
    nopOfFusion(nopOfFusion),power(power){compute();}

private:
    float xBaseRadius,yBaseRadius,startAngle,angle,extendLength,height,power;
    int nopOfRadius,nopOfExtend,nopOfArc,nopOfFusion;
    void compute();
};


#endif 