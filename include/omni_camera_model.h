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

using namespace std;
using namespace cv;

class OmniCameraModel
{
public:
    void computeKD(InputArrayOfArrays images, Size patternSize);
    void writeKD(string filename) const;
    void computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints);
    void readKD(string filename);
    void project(InputArrayOfArrays p3d,OutputArrayOfArrays p2d) const;
    void initUndistortMaps();
    void readRT(string filename);
    void writeRT(string filename) const;
    void undistort(InputArray src, OutputArray dst) const;
    void printRT() const;
    Size imageSize;   /* The image size (width/height) */
private:
    Mat cameraMatrix;           /* Camera matrix */
    Mat distCoeffs;             /* Distortion coefficients */
    Mat xi;
    Mat rvec;                   /* Rotation vector */
    Mat tvec;                   /* Translation vector */
    Mat undistortMapX;
    Mat undistortMapY;
};
#endif