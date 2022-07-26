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
#include "camera_model.h"

using namespace std;
using namespace cv;

class OcamCameraModel:public CameraModel
{
public:    
    void initUndistortMaps(float sf);
    void computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints);
    void loadModel(string filename);
    void project(InputArrayOfArrays p3d,OutputArrayOfArrays p2d) const;

private:
    vector<double>  pol;       /* The polynomial coefficients of radial camera model */
    vector<double>  invpol;    /* The coefficients of the inverse polynomial */
    Point2d         center;    /* cx and cy coordinates of the center in pixels */
    Matx22d         affine;    /* | sx  shy | sx, sy - scale factors along the x/y axis
                                  | shx sy  | shx, shy -  shear factors along the x/y axis */
    Point3f cam2world(const Point2f& imagePoint) const;//OpenCV style behavior! Different from origin Matlab style!
    Point2f world2cam(const Point3f& worldPoint) const;//OpenCV style behavior! Different from origin Matlab style!
};
#endif