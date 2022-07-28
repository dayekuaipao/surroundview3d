#include "ocam_camera_model.h"
#include <opencv4/opencv2/calib3d.hpp>
#include <opencv4/opencv2/core/matx.hpp>
#include <opencv4/opencv2/core/types.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#include <ostream>
#include <strings.h>


void OcamCameraModel::project(InputArrayOfArrays worldPoints,OutputArrayOfArrays imagePoints) const
{
    imagePoints.create(worldPoints.size(),CV_32FC2);
    Mat _imagePoints = imagePoints.getMat();
    vector<Point2f> cameraPoints;
    vector<Point3f> worldPoints_ = worldPoints.getMat();
    for(auto& worldPoint:worldPoints_)
    {
        worldPoint.z = -worldPoint.z;
    }
    projectPoints(worldPoints_, rvec, tvec, Mat::eye(Size(3,3),CV_32FC1),Mat::zeros(Size(5,1),CV_32FC1), cameraPoints);
    for(int i=0;i<cameraPoints.size();i++)
    {
            Point2f cameraPoint = cameraPoints[i];
            Point3f cameraPoint_ = Point3f{cameraPoint.x,cameraPoint.y,1};
            cameraPoint_ /= -sqrt(cameraPoint.x*cameraPoint.x+cameraPoint.y*cameraPoint.y+1);
            Point2f imagePoint;
            imagePoint = world2cam(cameraPoint_);
            _imagePoints.at<Point2f>(i) = imagePoint;
    }
}

void OcamCameraModel::readKD(string filename)
{
    string line;
    ifstream ifs_ref(filename.c_str());
    int line_num = 0;
    while (getline(ifs_ref, line))
    {
        if ((line.length() > 1) && (line.at(0) != '#')) 
        {
            stringstream str_stream(line);
            double dbl_val;
            
            switch (line_num)
            {
            case 0:     // polynomial coefficients
                str_stream >> dbl_val;
                while (str_stream >> dbl_val) 
                    pol.push_back(dbl_val);
                break;
            case 1:     // polynomial coefficients for the inverse mapping function
                str_stream >> dbl_val;
                while (str_stream >> dbl_val) 
                    invpol.push_back(dbl_val);
                break;
            case 2:     // center
                str_stream >> center.x;
                str_stream >> center.y;
                break;
            case 3:     // affine parameters
                str_stream >> affine(0, 0);
                str_stream >> affine(0, 1);
                str_stream >> affine(1, 0);
                affine(1, 1) = 1;
                break;
            case 4:     // image size
                str_stream >> imageSize.height;
                str_stream >> imageSize.width;
                break;
            default:
                break;                  
            }
            line_num++;
        }
    }
    
    ifs_ref.close(); 
}

Point3f OcamCameraModel::cam2world(const Point2f& imagePoint) const
{
    Point3f worldPoint;
    double invdet  = 1 / (affine(0, 0) - affine(0, 1) * affine(1, 0)); // 1/det(A), where A = [c,d;e,1] as in the Matlab file

    double xp = invdet*(affine(1, 1)*(imagePoint.y - center.x) - affine(0, 1) * (imagePoint.x - center.y));
    double yp = invdet*(-affine(1, 0) * (imagePoint.y - center.x) + affine(0, 0) * (imagePoint.x - center.y));
  
    double r   = sqrt(xp*xp + yp*yp); //distance [pixels] of  the point from the image center
    double zp  = pol[0];
    double r_i = 1;
     
    for (uint i = 1; i < pol.size(); i++)
    {
        r_i *= r;
        zp += r_i * pol[i];
    }
 
    //normalize to unit norm
    double invnorm = 1 / sqrt(xp*xp + yp*yp + zp*zp);
 
    worldPoint.x = (float)(invnorm*xp);
    worldPoint.y = (float)(invnorm*yp); 
    worldPoint.z = (float)(invnorm*zp);
    return worldPoint;
}

Point2f OcamCameraModel::world2cam(const Point3f& worldPoint) const
{
    Point2f imagePoint;
    double xc_norm = imageSize.width / 2.0;
    double yc_norm = imageSize.height / 2.0;
    
    // norm = sqrt(X^2 + Y^2)
    double norm = sqrt(worldPoint.x * worldPoint.x + worldPoint.y * worldPoint.y);
    if (norm == 0)
    {
        imagePoint.y = center.x;
        imagePoint.x = center.y;
    }
    else
    {
        // t = atan(Z/sqrt(X^2 + Y^2)) 
        double t = atan(worldPoint.z / norm);
        
        // r = a0 + a1 * t + a2 * t^2 + a3 * t^3 + ...
        double t_pow = t;
        double r = invpol[0];
        
        for (uint i = 1; i < invpol.size(); i++)
        {               
            r += t_pow * invpol[i];
            t_pow *= t;
        }
        
        /* | u | = r * | X | / sqrt(X^2 + Y^2);
           | v |       | Y |                     */
        double u = r * worldPoint.x / norm;
        double v = r * worldPoint.y / norm;

        imagePoint.y = (float)(affine(0, 0) * u + affine(0, 1) * v + center.x);
        imagePoint.x = (float)(affine(1, 0) * u + affine(1, 1) * v + center.y);
    }           
    return imagePoint;
}

void OcamCameraModel::initUndistortMaps(float sf)
{
    int width = imageSize.width;
    int height = imageSize.height;
    double Nxc = height / 2.0;
    double Nyc = width / 2.0;
    double Nz = -width / sf;
    Point3f worldPoint;
    Point2f cameraPoint;
    undistortMapX.create(height, width, CV_32FC1);
    undistortMapY.create(height, width, CV_32FC1);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            worldPoint.x = (i - Nxc);
            worldPoint.y = (j - Nyc);
            worldPoint.z = Nz;
            cameraPoint = world2cam(worldPoint);
            undistortMapX.at<float>(i, j) = (float)cameraPoint.x;
            undistortMapY.at<float>(i, j) = (float)cameraPoint.y;
        }
    }
}

void OcamCameraModel::computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints)
{
    vector<Point2f> cameraPoints;
    Mat _imagePoints = imagePoints.getMat();
    for(int i=0;i<_imagePoints.rows;i++)
    {
        for(int j=0;j<_imagePoints.cols;j++)
        {
            Point3f cameraPoint;
            Point2f imagePoint = _imagePoints.at<Point2f>(i,j);
            cameraPoint = cam2world(imagePoint);
            Point2f cameraPoint_ = Point2f(cameraPoint.x/cameraPoint.z,cameraPoint.y/cameraPoint.z);
            cameraPoints.push_back(cameraPoint_);
        }
    }
    cv::solvePnP(worldPoints,cameraPoints,Mat::eye(Size(3,3),CV_32FC1),Mat::zeros(Size(5,1),CV_32FC1),rvec,tvec);
}