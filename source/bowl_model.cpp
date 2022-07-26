#include "bowl_model.h"
#include <cmath>
#include <opencv4/opencv2/core/types.hpp>
#define PI 3.1415926

void BowlModel::compute() 
{
    float factor = height/pow(extendLength,power);
    for(int i=0;i<=nopOfArc;i++)
    {
        float angle_ = (90-startAngle-i*angle/nopOfArc)*PI/180;
        float baseRadius_ = xBaseRadius*yBaseRadius/sqrt(pow(xBaseRadius*sin(angle_),2)+pow(yBaseRadius*cos(angle_),2));
        for(int j=1;j<=nopOfRadius;j++)
        {
            float radius_ = j*baseRadius_/nopOfRadius;
            Point3f position = Point3f{radius_*cos(angle_),radius_*sin(angle_),0};
            positions.push_back(position);
            if(i<nopOfFusion)
            {
                alphas.push_back(1.0*i/nopOfFusion);
            }
            else if(i>nopOfArc-nopOfFusion)
            {
                alphas.push_back(1.0*(nopOfArc-i)/nopOfFusion);
            }
            else
            {
                alphas.push_back(1);
            }
        }
        for(int j=1;j<=nopOfExtend;j++)
        {
            float radius_ = baseRadius_+j*extendLength/nopOfExtend;
            float height_ = factor*pow(j*extendLength/nopOfExtend,power);
            Point3f position = Point3f{radius_*cos(angle_),radius_*sin(angle_),height_};
            positions.push_back(position);
            if(i<nopOfFusion)
            {
                alphas.push_back(1.0*i/nopOfFusion);
            }
            else if(i>nopOfArc-nopOfFusion)
            {
                alphas.push_back(1.0*(nopOfArc-i)/nopOfFusion);
            }
            else
            {
                alphas.push_back(1);
            }
        }
    }
    for(int i=0;i<nopOfArc;i++)
    {
        for(int j=0;j<nopOfRadius+nopOfExtend-1;j++)
        {
            unsigned int index1 = i*(nopOfRadius+nopOfExtend)+j;
            unsigned int index2 = i*(nopOfRadius+nopOfExtend)+j+1;
            unsigned int index3 = (i+1)*(nopOfRadius+nopOfExtend)+j;
            unsigned int index4 = (i+1)*(nopOfRadius+nopOfExtend)+j+1;
            indexes.push_back(index1);
            indexes.push_back(index4);
            indexes.push_back(index2);

            indexes.push_back(index1);
            indexes.push_back(index3);
            indexes.push_back(index4);
        }
    }
}

