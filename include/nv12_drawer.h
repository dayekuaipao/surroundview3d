#ifndef NV12_DRAWER_H
#define NV12_DRAWER_H
#include "camera_drawer.h"

class NV12Drawer:public CameraDrawer
{
public:
    NV12Drawer(const OmniCameraModel& cameraModel,const ObjectModel& objectModel,Shader shader):CameraDrawer(cameraModel,objectModel,shader){};
    void initTexture();
    void setTexture(const Buffer& frame) const;
private:
    GLuint yTexture;
    GLuint uvTexture;
    float borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
};
#endif