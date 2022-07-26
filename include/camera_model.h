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
#include <opencv4/opencv2/viz/types.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;
using namespace cv;

class CameraModel
{
protected:
    Mat rvec;                   /* Rotation vector */
    Mat tvec;                   /* Translation vector */
    Mat undistortMapX;
    Mat undistortMapY;

public:
    Size imageSize;   /* The image size (width/height) */
    void readRT(string path);
    void writeRT(string path) const;
    void undistort(InputArray src, OutputArray dst) const;
    virtual void computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints)=0;
    virtual void loadModel(string filename)=0;
    virtual void project(InputArrayOfArrays p3d,OutputArrayOfArrays p2d) const=0;
    virtual ~CameraModel() = default;
};
#endif