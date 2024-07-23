#include <Eigen/Dense>
#include <Eigen/Core>
#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>
#include <ceres/ceres.h>
#include <ceres/rotation.h>
#include "common/omni_camera_model.hpp"
using namespace cv;
using namespace std;
using namespace ceres;

vector<vector<Point2f>> findContours(Mat image,double tolerance=0.1){
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Mat thresh;
    threshold(gray, thresh, 100, 255, THRESH_BINARY);
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(thresh, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    vector<vector<Point2f>> res;
    for(int i=0;i<contours.size();i++){
        vector<Point> approx;
        approxPolyDP(contours[i],approx, 0.1*arcLength(contours[i], true), true);
        double area = contourArea(approx);
        double arc = arcLength(approx, true);
        if(approx.size()==4&&area>3600){
            int idx = hierarchy[i][2];
            while(idx!=-1){
                vector<Point> approx_in;
                approxPolyDP(contours[idx],approx_in, 0.1*arcLength(contours[idx], true), true);
                if(approx_in.size()==4&&area>900){
                    double area_in = contourArea(approx_in);
                    double arc_in = arcLength(approx_in, true);
                    if(abs(area_in*9.0/area-1)<tolerance&&abs(arc_in*3.0/arc-1)<tolerance){
                        vector<Point2f> approx_in_float;
                        for(int j=0;j<approx_in.size();j++){
                            approx_in_float.push_back(Point2f{(float)approx_in[j].x,(float)approx_in[j].y});
                        }
                        res.push_back(approx_in_float);
                    }
                }
                idx = hierarchy[idx][0];
            }
        }
    }
    return res;
}

struct ProjectionError {
  ProjectionError(vector<vector<Point2f>> imagePoints,vector<Point3f> initWorldPoints)
      : imagePoints(imagePoints), initWorldPoints(initWorldPoints){}

  template <typename T>
  bool operator()(const T* const camera,T* residuals) const {
    T wp[NUM_CAMERA][NUM_POINT][2];

    for(int i=0;i<NUM_CAMERA-1;i++){
        T translation[3];
        T rotationVector[3]; 
        T rotation[9];
        for(int j=0;j<3;j++){
            rotationVector[j]=camera[i*6+j];
            translation[j]=camera[i*6+j+3];
        }
        AngleAxisToRotationMatrix(rotationVector,rotation);
        for(int j=0;j<NUM_POINT;j++){
            T s1,s2,s;
            T p[3] = {(T)imagePoints[i+1][j].x,(T)imagePoints[i+1][j].y,(T)1};
            s1 = rotation[6]*p[0]+rotation[7]*p[1]+rotation[8]*p[2];
            s2 = rotation[6]*translation[0]+rotation[7]*translation[1]+rotation[8]*translation[2];
            s = s2/s1;
            T p2[3];
            for(int k=0;k<3;k++){
                p2[k] = s*p[k]-translation[k];
            }
            wp[i][j][0] = rotation[0]*p2[0]+rotation[1]*p2[1]+rotation[2]*p2[2];
            wp[i][j][1] = rotation[3]*p2[0]+rotation[4]*p2[1]+rotation[5]*p2[2];
        }
    }
    for(int i=0;i<NUM_POINT;i++){
        wp[NUM_CAMERA-1][i][0] = (T)initWorldPoints[i].x;
        wp[NUM_CAMERA-1][i][1] = (T)initWorldPoints[i].y;
    }
    for(int i=0;i<NUM_CAMERA;i++){
        for(int j=0;j<NUM_POINT/2;j++){
            for(int k=0;k<2;k++){
                residuals[i*NUM_POINT+j*2+k] = wp[i][j+NUM_POINT/2][k]-wp[(i+1)%NUM_CAMERA][j][k];
            }
        }
    }
    return true;
  }

   static ceres::CostFunction* Create(const vector<vector<Point2f>> imagePoints,const vector<Point3f> initWorldPoints) {
     return new ceres::AutoDiffCostFunction<ProjectionError, NUM_CAMERA*NUM_POINT, (NUM_CAMERA-1)*6>
       (imagePoints,initWorldPoints);
   }
    vector<vector<Point2f>> imagePoints;
    vector<Point3f> initWorldPoints;
};
int main(){
    vector<vector<vector<Point2f>>> imagePoints;
    vector<OmniCameraModel> omniCameraModels;
    omniCameraModels.resize(NUM_CAMERA);
    double boardWidth = 75.0;
    for(int i=0;i<NUM_CAMERA;i++){
        Mat image = imread(string(DATAPATH)+string("/images/calib_extrinsic/image_")+to_string(i)+".jpg");
        // vector<vector<Point2f>> contours = findContours(image);
        // imagePoints.push_back(contours);
        omniCameraModels[i].readKD(string(DATAPATH)+string("/params/intrinsic/intrinsic_")+to_string(i)+".yaml");
    }
    /***************************/
    vector<vector<Point2f>> oriImgPoints;
    oriImgPoints.resize(6);
    for(int i=0;i<6;i++){
        oriImgPoints[i].resize(8);
    }

    oriImgPoints[0][0] = Point2f(258, 428);
    oriImgPoints[0][1] = Point2f(331, 439);
    oriImgPoints[0][2] = Point2f(196, 509);
    oriImgPoints[0][3] = Point2f(268, 535);
    oriImgPoints[0][4] = Point2f(909, 431);
    oriImgPoints[0][5] = Point2f(984, 421);
    oriImgPoints[0][6] = Point2f(972, 528);
    oriImgPoints[0][7] = Point2f(1045, 499);

    oriImgPoints[1][0] = Point2f(132, 476);
    oriImgPoints[1][1] = Point2f(172, 420);
    oriImgPoints[1][2] = Point2f(154, 492);
    oriImgPoints[1][3] = Point2f(198, 426);
    oriImgPoints[1][4] = Point2f(1147, 502);
    oriImgPoints[1][5] = Point2f(1095, 430);
    oriImgPoints[1][6] = Point2f(1177, 482);
    oriImgPoints[1][7] = Point2f(1130, 422);

    oriImgPoints[2][0] = Point2f(168, 483);
    oriImgPoints[2][1] = Point2f(222, 412);
    oriImgPoints[2][2] = Point2f(210, 510);
    oriImgPoints[2][3] = Point2f(272, 424);
    oriImgPoints[2][4] = Point2f(1172, 481);
    oriImgPoints[2][5] = Point2f(1123, 418);
    oriImgPoints[2][6] = Point2f(1194, 465);
    oriImgPoints[2][7] = Point2f(1153, 412);

    oriImgPoints[3][0] = Point2f(206, 488);
    oriImgPoints[3][1] = Point2f(144, 462);
    oriImgPoints[3][2] = Point2f(292, 394);
    oriImgPoints[3][3] = Point2f(222, 385);
    oriImgPoints[3][4] = Point2f(1085, 474);
    oriImgPoints[3][5] = Point2f(1020, 496);
    oriImgPoints[3][6] = Point2f(1011, 395);
    oriImgPoints[3][7] = Point2f(938, 399);

    oriImgPoints[4][0] = Point2f(163, 430);
    oriImgPoints[4][1] = Point2f(114, 494);
    oriImgPoints[4][2] = Point2f(134, 422);
    oriImgPoints[4][3] = Point2f(91, 477);
    oriImgPoints[4][4] = Point2f(1060, 436); 
    oriImgPoints[4][5] = Point2f(1114, 511); 
    oriImgPoints[4][6] = Point2f(1007, 448); 
    oriImgPoints[4][7] = Point2f(1067, 538); 

    oriImgPoints[5][0] = Point2f(162, 437); 
    oriImgPoints[5][1] = Point2f(110, 513); 
    oriImgPoints[5][2] = Point2f(126, 429); 
    oriImgPoints[5][3] = Point2f(81, 492);  
    oriImgPoints[5][4] = Point2f(1088, 400);
    oriImgPoints[5][5] = Point2f(1132, 451);
    oriImgPoints[5][6] = Point2f(1061, 409);
    oriImgPoints[5][7] = Point2f(1110, 469);
    for(int i=0;i<6;i++){
        vector<vector<Point2f>> contours;
        contours.resize(2);
        vector<Point2f> contour;
        for(int k=0;k<4;k++){
            contours[0].push_back(oriImgPoints[i][k]);
            contours[1].push_back(oriImgPoints[i][k+4]);
        }
        imagePoints.push_back(contours);
    }
    /***************************/

    vector<Point3f> worldPoints;
    worldPoints.resize(4);
    worldPoints[0] = Point3f(0, boardWidth, 0);
    worldPoints[1] = Point3f(boardWidth, boardWidth, 0);
    worldPoints[2] = Point3f(0, 0, 0);
    worldPoints[3] = Point3f(boardWidth, 0, 0);

    for(int i=0;i<NUM_CAMERA;i++){
        omniCameraModels[i].computeRT(imagePoints[i][0],worldPoints);
        for(int j=0;j<4;j++){
            worldPoints[j] = omniCameraModels[i].projectOnGround(imagePoints[i][1][j]);
        }
    }

    vector<Point3f> initWorldPoints;
    for(int i=0;i<2;i++){
        for(int j=0;j<NUM_POINT/2;j++){
            Point3f worldPointOut =  omniCameraModels[0].projectOnGround(imagePoints[0][i][j]);
            initWorldPoints.push_back(worldPointOut);
        }
    }

    vector<vector<Point2f>> undisortPoints;
    for(int i=0;i<imagePoints.size();i++){
        vector<Point2f> outUndisortPoints;
        for(int j=0;j<imagePoints[i].size();j++){
            vector<Point2f> outUndisortPoints_;
            omniCameraModels[i].undistortPoints(imagePoints[i][j],outUndisortPoints_);
            for(int i=0;i<outUndisortPoints_.size();i++){
                outUndisortPoints.push_back(outUndisortPoints_[i]);
            }
        }
        undisortPoints.push_back(outUndisortPoints);
    }

    ceres::CostFunction* cost_function = ProjectionError::Create(undisortPoints,initWorldPoints);
    double camera_params[(imagePoints.size()-1)*6];
    for(int i=0;i<imagePoints.size()-1;i++){
        for(int j=0;j<3;j++){
            camera_params[i*6+j] = omniCameraModels[i+1].rvec.at<double>(0,j);
            camera_params[i*6+j+3] = omniCameraModels[i+1].tvec.at<double>(0,j);
        }
    }
    ceres::Problem problem;
    problem.AddResidualBlock(cost_function,
                            nullptr /* squared loss */,
                            camera_params);
    ceres::Solver::Options options;
    options.minimizer_progress_to_stdout = true;
    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);
    std::cout << summary.FullReport() << endl;
    for(int i=0;i<NUM_CAMERA-1;i++){
        for(int j=0;j<3;j++){
            omniCameraModels[i+1].rvec.at<double>(0,j) = camera_params[i*6+j];
            omniCameraModels[i+1].tvec.at<double>(0,j) = camera_params[i*6+j+3];
        }   
    }

        vector<Mat> vRcam2wd;
        vector<Mat> vcamPoints;
        vRcam2wd.resize(NUM_CAMERA);
        vcamPoints.resize(NUM_CAMERA);
        for(uint i = 0; i< NUM_CAMERA; i++){
            Rodrigues(omniCameraModels[i].rvec, vRcam2wd[i]);
            vcamPoints[i] = -vRcam2wd[i].t() * omniCameraModels[i].tvec;
        }

        size_t frontCameraNum = 0;
        size_t backCameraNum = NUM_CAMERA/2;
        Point2f vehicleHead = Point2f(vcamPoints[frontCameraNum].at<double>(0,0), vcamPoints[frontCameraNum].at<double>(1,0));
        Point2f vehicleTail = Point2f(vcamPoints[backCameraNum].at<double>(0,0), vcamPoints[backCameraNum].at<double>(1,0));
        Point2f vehicleBodyL = Point2f(vcamPoints[backCameraNum+1].at<double>(0,0), vcamPoints[backCameraNum+1].at<double>(1,0));
        Point2f vehicleBodyR = Point2f(vcamPoints[backCameraNum-1].at<double>(0,0), vcamPoints[backCameraNum-1].at<double>(1,0));


        double vehicleLength = sqrt(pow((vehicleTail.x - vehicleHead.x),2) + pow((vehicleTail.y - vehicleHead.y),2));
        double vehicleWidth = sqrt(pow((vehicleBodyL.x - vehicleBodyR.x),2) + pow((vehicleBodyL.y - vehicleBodyR.y),2));

        Point2f vehicleCenter = Point2f((vehicleHead.x+vehicleTail.x)/2, (vehicleHead.y+vehicleTail.y)/2);

        Mat Rvehicle(3,3, CV_64FC1, Scalar(0));
        Rvehicle.at<double>(0,0) = ( abs(vehicleCenter.y - vehicleHead.y)/(vehicleLength/2));
        Rvehicle.at<double>(0,1) = (-abs(vehicleCenter.x - vehicleHead.x))/(vehicleLength/2);
        Rvehicle.at<double>(1,0) =   abs(vehicleCenter.x - vehicleHead.x)/(vehicleLength/2);
        Rvehicle.at<double>(1,1) =   abs(vehicleCenter.y - vehicleHead.y)/(vehicleLength/2);
        Rvehicle.at<double>(2,2) = 1;

        Mat Tvehicle(3,1, CV_64FC1, Scalar(0));
        Tvehicle.at<double>(0,0) = vehicleCenter.x;
        Tvehicle.at<double>(1,0) = vehicleCenter.y;
        Tvehicle.at<double>(2,0) = 0;

        for(uint i = 0; i < NUM_CAMERA; i++){
            Mat Rcam2vehicle = vRcam2wd[i]*Rvehicle;
            omniCameraModels[i].tvec = omniCameraModels[(i)].tvec + vRcam2wd[i]*Tvehicle;
            Rodrigues(Rcam2vehicle, omniCameraModels[(i)].rvec);
        }

    for(int i=0;i<NUM_CAMERA;i++){
        omniCameraModels[i].writeRT(string(DATAPATH)+string("/params/extrinsic/extrinsic_")+to_string(i)+".yaml");
    }
}