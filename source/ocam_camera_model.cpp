#include "ocam_camera_model.h"
#include <opencv4/opencv2/calib3d.hpp>
#include <opencv4/opencv2/core/matx.hpp>
#include <opencv4/opencv2/core/types.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <strings.h>


void OcamCameraModel::project(InputArrayOfArrays worldPoints,OutputArrayOfArrays imagePoints) const
{
    Mat R;
    cv::Rodrigues(rvec, R);
    Mat RT = Mat::eye(4,4,CV_32F);
    Mat rROI(RT,Rect(0,0,3,3));
    R.copyTo(rROI);
    Mat tROI(RT,Rect(3,0,1,3));
    tvec.copyTo(tROI);

    Mat _worldPoints = worldPoints.getMat();
    imagePoints.create(_worldPoints.size(),CV_32FC2);
    Mat _imagePoints = imagePoints.getMat();
    
    for(int i=0;i<_worldPoints.rows;i++)
    {
        for(int j=0;j<_worldPoints.cols;i++)
        {
            Point3d worldPoint = _worldPoints.at<Point3f>(i,j);
            Mat worldPointMat(4, 1, CV_32F);
            worldPointMat.at<float>(0,0) = worldPoint.x;
            worldPointMat.at<float>(1,0) = worldPoint.y;
            worldPointMat.at<float>(2,0) = worldPoint.z;
            worldPointMat.at<float>(3,0) = 1;
            Mat cameraPointMat = RT*worldPointMat;
            cameraPointMat /= cameraPointMat.at<float>(3,0);
            Point3d cameraPoint(cameraPointMat.at<float>(0,0),cameraPointMat.at<float>(1,0),cameraPointMat.at<float>(2,0));
            Point2d imagePoint;
            world2cam(cameraPoint,imagePoint);
            _imagePoints.at<Point2d>(i,j) = imagePoint;
        }
    }
}

void OcamCameraModel::loadModel(string filename)
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

void OcamCameraModel::cam2world(const Point2f& imagePoint,Point3f& worldPoint) const
{
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
}

void OcamCameraModel::world2cam(const Point3f& worldPoint,Point2f& imagePoint) const
{
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

}

void OcamCameraModel::initUndistortMaps(float sf)
{
    int width = imageSize.width;
    int height = imageSize.height;
    double Nxc = height / 2.0;
    double Nyc = width / 2.0;
    double Nz = -width / sf;
    Point3d worldPoint;
    Point2d imagePoint;
    undistortMapX.create(height, width, CV_32FC1);
    undistortMapY.create(height, width, CV_32FC1);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            worldPoint.x = (i - Nxc);
            worldPoint.y = (j - Nyc);
            worldPoint.z = Nz;
            world2cam(worldPoint, imagePoint);
            undistortMapX.at<float>(i, j) = (float)imagePoint.x;
            undistortMapY.at<float>(i, j) = (float)imagePoint.y;
        }
    }
}

void OcamCameraModel::computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints)
{
    vector<Point3d> cameraPoints;
    Mat _imagePoints = imagePoints.getMat();
    Mat _worldPoints = worldPoints.getMat();
    for(int i=0;i<_worldPoints.rows;i++)
    {
        for(int j=0;j<_worldPoints.cols;i++)
        {
            Point3d cameraPoint;
            Point2d imagePoint = _imagePoints.at<Point2d>(i,j);
            cam2world(imagePoint,cameraPoint);
            cameraPoints.push_back(cameraPoint);
        }
    }
    Mat homo = cv::findHomography(worldPoints,cameraPoints);
    Vec3d r1 = homo.col(0);
    Vec3d r2 = homo.col(1);
    Vec3d r3 = r1.cross(r2);
    Vec3d tvec = homo.col(2);
    Matx33d R = cv::Matx33d{r1[0],r2[0],r3[0],
                r1[1],r2[1],r3[1],
                r1[2],r2[2],r3[2]};
    cv::Rodrigues(R, rvec);
}