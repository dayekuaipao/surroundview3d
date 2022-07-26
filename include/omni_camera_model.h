#ifndef OMNI_CAMERA_MODEL_H
#define OMNI_CAMERA_MODEL_H
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/core/cvstd.hpp>
#include <opencv4/opencv2/core/mat.hpp>
#include <opencv4/opencv2/core/operations.hpp>
#include <opencv4/opencv2/core/types.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "camera_model.h"

using namespace std;
using namespace cv;

class OmniCameraModel:public CameraModel
{
public:
    void computeKD(InputArrayOfArrays images, Size patternSize);
    void saveModel(string path) const;
    void computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints);
    void loadModel(string filename);
    void project(InputArrayOfArrays p3d,OutputArrayOfArrays p2d) const;
    
private:
    Mat distCoeffs;             /* Distortion coefficients */
    Mat xi;
};
#endif