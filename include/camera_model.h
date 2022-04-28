#ifndef CAMERA_MODEL_H
#define CAMERA_MODEL_H
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

using namespace std;
using namespace cv;

class CameraModel
{
public:
    int index;
    Size imageSize;   /* The image size (width/height) */
    Mat cameraMatrix;                      /* Camera matrix */
    Mat distCoeffs;             /* Distortion coefficients */
    Mat xi;
    
    Mat rvec;                   /* Rotation vector */
    Mat tvec;                   /* Translation vector */
    Mat map1,map2;

    CameraModel(Size imageSize):index(0),imageSize(imageSize){};
    CameraModel(int idx,Size imageSize):index(idx),imageSize(imageSize){};
    
    void computeKD(InputArrayOfArrays images, Size patternSize);
    void computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints);
    void readKD(string path);
    void readRT(string path);
    void writeKD(string path);
    void writeRT(string path);
    void project(InputArrayOfArrays p3d,OutputArrayOfArrays p2d);
    void undistort(InputArray src, OutputArray dst);
    
};
#endif