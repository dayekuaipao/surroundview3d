#ifndef YUYV_DRAWER_H
#define YUYV_DRAWER_H
#include "camera_drawer.h"

class YUYVDrawer:public CameraDrawer
{
public:
    YUYVDrawer(const OmniCameraModel& cameraModel,const ObjectModel& objectModel,Shader shader):CameraDrawer(cameraModel,objectModel,shader){};
    void initTexture();
    void setTexture(const Buffer& frame) const;
private:
    GLuint yTexture;
    GLuint uvTexture;
    float borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
};
#endif