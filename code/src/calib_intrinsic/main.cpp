#include "common/omni_camera_model.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;
int main(){
    for(int i=0;i<NUM_CAMERA;i++){
        Size chessboardSize(CHESSBOARD_WIDTH,CHESSBOARD_HEIGHT);
        OmniCameraModel omniCameraModel;
        vector<Mat> images;
        for(int j=0;j<NUM_IMAGES;j++){
            Mat image = imread(string(DATAPATH)+string("/images/calib_intrinsic/camera_")+to_string(i)+"/image_"+to_string(j)+".jpg");
            images.push_back(image);
        }
        omniCameraModel.computeKD(images,chessboardSize);
        omniCameraModel.writeKD(string(DATAPATH)+string("/params/intrinsic/intrinsic_")+to_string(i)+".yaml");
    }
}