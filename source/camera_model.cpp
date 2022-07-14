#include "camera_model.h"
#include <opencv4/opencv2/calib3d.hpp>
#include <opencv4/opencv2/core/hal/interface.h>
#include <opencv4/opencv2/core/matx.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/ccalib/omnidir.hpp>
#include <strings.h>

void CameraModel::readRT(string path)
{
    FileStorage fs(path,FileStorage::READ);
    fs["rvec"]>>rvec;
    fs["tvec"]>>tvec;
    Mat R;
    Rodrigues(rvec, R);
    Mat cameraRotationVector;
    Rodrigues(R.t(), cameraRotationVector);
    Mat cameraTranslationVector = -R.t() * tvec;
    cout << "Camera translation  "<<endl <<  cameraTranslationVector << endl;
    cout << "Camera rotation  " <<endl<<  cameraRotationVector << endl;\
    fs.release();
}

void CameraModel::writeRT(string path)
{
    FileStorage fs(path,FileStorage::WRITE);
    fs<<"rvec"<<rvec;
    fs<<"tvec"<<tvec;
    fs.release();
}

void CameraModel::undistort(InputArray src, OutputArray dst)
{
    
    remap(src, dst, undistortMapX, undistortMapY, INTER_LINEAR);
}