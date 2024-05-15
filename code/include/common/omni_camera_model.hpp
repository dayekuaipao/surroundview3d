#ifndef OMNI_CAMERA_MODEL_HPP
#define OMNI_CAMERA_MODEL_HPP
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv4/opencv2/ccalib/omnidir.hpp>

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
    void undistortPoints(InputArray src, OutputArray dst) const;
    void printRelativeRT() const;
    Point3f projectOnGround(Point2f imagePoint) const;
    Size imageSize;   /* The image size (width/height) */
    Mat rvec;                   /* Rotation vector */
    Mat tvec;                   /* Translation vector */
private:
    Mat cameraMatrix;           /* Camera matrix */
    Mat distCoeffs;             /* Distortion coefficients */
    Mat xi;
    Mat undistortMapX;
    Mat undistortMapY;
};


inline void OmniCameraModel::computeKD(InputArrayOfArrays images, Size patternSize)
{
    
    vector<Mat> object_points;
    vector<Mat> image_points;
    Mat obj;        // Chessboard corners in 3D spase
    obj.create(patternSize.width * patternSize.height, 1, CV_64FC3);
    Vec3d *ptr = obj.ptr<Vec3d>();
    for (int i = 0; i < patternSize.height; ++i)
    {
        for (int j = 0; j < patternSize.width; ++j)
        {
            ptr[i*patternSize.width + j] = Vec3d(double(j), double(i), 0.0);
        }
    }

    vector<Mat> chessboard_imgs;
    images.getMatVector(chessboard_imgs);
    
    for (auto&chessboard_img:chessboard_imgs)
    {
        //Find chessboard corners
        Mat corners;    // Chessboard corners in 2D camera frame
        imageSize = chessboard_img.size();
        bool found = findChessboardCorners(chessboard_img, patternSize, corners);
        if (found)
        {
            if(corners.type()!=CV_64FC2)
            {
                corners.convertTo(corners,CV_64FC2);
            }
            image_points.push_back(corners);
            object_points.push_back(obj);
        }
    }

    // Calculate intrinsic parameters
    if (object_points.size() > 3)
    {
        Mat _rvecs, _tvecs;
        int flags = 0;
        TermCriteria criteria(3, 200, 1e-8);
        Mat idx;
        double rms = omnidir::calibrate(object_points, image_points, imageSize, cameraMatrix,xi,distCoeffs,_rvecs,_tvecs,flags, criteria,idx);
        cout<<"rms "<<rms<<endl;
        cout<<"camera K  "<<endl<<cameraMatrix<<endl;
        cout<<"camera D  "<<endl<<distCoeffs<<endl;
        cout<<"xi  "<<endl<<xi<<endl;
    }
    else
    {
        cout<<"Can not Find Chessboard Corners!"<<endl;
    }
}

inline void OmniCameraModel::computeRT(InputArrayOfArrays imagePoints,InputArrayOfArrays worldPoints)
{
    Mat undistortedPoints;
    omnidir::undistortPoints(imagePoints,undistortedPoints,cameraMatrix,distCoeffs,xi,Matx33f::eye());
    solvePnP(worldPoints,undistortedPoints,Matx33f::eye(),Matx41f::zeros(),rvec,tvec);
}


inline void OmniCameraModel::readKD(string filename)
{
    FileStorage fs(filename,FileStorage::READ);
    fs["cameraMatrix"]>>cameraMatrix;
    fs["distCoeffs"]>>distCoeffs;
    fs["xi"]>>xi;
    fs["imageSize"]>>imageSize;
    fs.release();
}

inline void OmniCameraModel::writeKD(string filename) const
{
    FileStorage fs(filename,FileStorage::WRITE);
    fs<<"cameraMatrix"<<cameraMatrix;
    fs<<"distCoeffs"<<distCoeffs;
    fs<<"xi"<<xi;
    fs<<"imageSize"<<imageSize;
    fs.release();
}

inline void OmniCameraModel::project(InputArrayOfArrays p3ds,OutputArrayOfArrays p2ds) const
{
    double _xi = xi.at<double>(0);
    cout<<_xi<<endl;
    omnidir::projectPoints(p3ds, p2ds, rvec, tvec, cameraMatrix, _xi, distCoeffs);
}

inline void OmniCameraModel::initUndistortMaps()
{
    Matx33f p = Matx33d{imageSize.width*0.125f,0,imageSize.width*0.5f,0,imageSize.height*0.125f,imageSize.height*0.5f,0,0,1};
    omnidir::initUndistortRectifyMap(cameraMatrix, distCoeffs, xi, Matx33d::eye(), p, imageSize,CV_16SC2, undistortMapX, undistortMapY, omnidir::RECTIFY_PERSPECTIVE);
}

inline void OmniCameraModel::readRT(string filename)
{
    FileStorage fs(filename,FileStorage::READ);
    fs["rvec"]>>rvec;
    fs["tvec"]>>tvec;
    Mat R;
    Rodrigues(rvec, R);
    Mat cameraRotationVector;
    Rodrigues(R.t(), cameraRotationVector);
    Mat cameraTranslationVector = -R.t() * tvec;
    cout<<"Camera translation  "<<endl<<cameraTranslationVector<<endl;
    cout<<"Camera rotation  "<<endl<<cameraRotationVector<<endl;
    fs.release();
}

inline void OmniCameraModel::writeRT(string filename) const
{
    FileStorage fs(filename,FileStorage::WRITE);
    fs<<"rvec"<<rvec;
    fs<<"tvec"<<tvec;
    fs.release();
}

inline void OmniCameraModel::undistort(InputArray src, OutputArray dst) const
{
    
    remap(src, dst, undistortMapX, undistortMapY, INTER_LINEAR);
}
inline void OmniCameraModel::undistortPoints(InputArray src, OutputArray dst) const
{
    omnidir::undistortPoints(src,dst,cameraMatrix,distCoeffs,xi,Matx33f::eye());
}

inline void OmniCameraModel::printRelativeRT() const
{
    Mat R;
    Rodrigues(rvec, R);
    Mat cameraRotationVector;
    Rodrigues(R.t(), cameraRotationVector);
    Mat cameraTranslationVector = -R.t() * tvec;
    cout<<"Relative camera translation  "<<endl<<cameraTranslationVector<<endl;
    cout<<"Relative camera rotation  "<<endl<<cameraRotationVector<<endl;
}

inline Point3f OmniCameraModel::projectOnGround(Point2f inPoint) const
{
    Mat rotationMatrix;
    Rodrigues(rvec, rotationMatrix);
    vector<Point2f> vinPoint = {inPoint};
    undistortPoints(vinPoint,vinPoint);
    Mat imagePoint = Mat::ones(3, 1, DataType<double>::type); //u,v,1
	imagePoint.at<double>(0, 0) = vinPoint[0].x;
	imagePoint.at<double>(1, 0) = vinPoint[0].y;
 
	//计算比例参数S
	Mat tempMat, tempMat2;
	tempMat = rotationMatrix.t()  * imagePoint;
	tempMat2 = rotationMatrix.t() * tvec;
	double s = tempMat2.at<double>(2, 0)/ tempMat.at<double>(2, 0);
    //计算世界坐标
	Mat wcPoint = rotationMatrix.t() * (s  * imagePoint - tvec);
	Point3f worldPoint(wcPoint.at<double>(0, 0), wcPoint.at<double>(1, 0), wcPoint.at<double>(2, 0));
	return worldPoint;
}
#endif