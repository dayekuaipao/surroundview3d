#ifndef RGB_DRAWER_H
#define RGB_DRAWER_H
#include "camera_drawer.h"

class RGBDrawer:public CameraDrawer
{
public:
    RGBDrawer(const OmniCameraModel& cameraModel,const ObjectModel& objectModel,Shader shader):CameraDrawer(cameraModel,objectModel,shader){};
    void initTexture();
    void setTexture(const Buffer& frame) const;
private:
    GLuint texture;
    float borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
};
#endif