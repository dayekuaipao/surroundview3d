#ifndef OCAM_CAMERA_MODEL_H
#define OCAM_CAMERA_MODEL_H
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

class OcamCameraModel
{
public:
    int index;
    Size imageSize;   /* The image size (width/height) */
    Matx44d RT;                      /* Camera matrix */

    Mat undistortMapX;
    Mat undistortMapY;
    vector<double>  pol;       /* The polynomial coefficients of radial camera model */
    vector<double>  invpol;    /* The coefficients of the inverse polynomial */
    Point2d         center;    /* cx and cy coordinates of the center in pixels */
    Matx22d         affine;    /* | sx  shy | sx, sy - scale factors along the x/y axis
                                  | shx sy  | shx, shy -  shear factors along the x/y axis */
    

    OcamCameraModel(Size imageSize):index(0),imageSize(imageSize){};
    OcamCameraModel(int idx,Size imageSize):index(idx),imageSize(imageSize){};
                 
    void undistort(InputArray src, OutputArray dst);
    void project(vector<Point3d>& worldPoints,vector<Point2d>& imagePoints);

    void computeRT(vector<Point2d>& imagePoints,vector<Point3d>& worldPoints);
    void readRT(string path);
    void writeRT(string path);
    void loadModel(string filename);

    void initUndistortMaps(float sf);

private:
    void cam2world(Point2d& imagePoint,Point3d& worldPoint);//OpenCV style behavior! Different from origin Matlab style!
    void world2cam(Point3d& worldPoint,Point2d& imagePoint);////OpenCV style behavior! Different from origin Matlab style!
};
#endif