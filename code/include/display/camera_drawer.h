#ifndef DRAWER_H
#define DRAWER_H
#include "object_model.h"
#include "common/omni_camera_model.hpp"
#include "shader.h"
#include "capture.h"
#include <vector>
class CameraDrawer
{
public:
    CameraDrawer(const OmniCameraModel& cameraModel,const ObjectModel& objectModel,Shader shader);
    void initVertex(GLuint& VAO,GLuint& VBO,GLuint& EBO);
    virtual void initTexture()=0;
    virtual void setTexture(const Buffer& frame) const =0;

    void draw(const Buffer& frame,const glm::mat4& projection,const glm::mat4& view,const glm::mat4& model) const;

protected:
    size_t positionSize;
    size_t indexSize;
    size_t height;
    size_t width;
    float* p_vertexes;
    unsigned int* p_indexes;
    GLuint mVAO;
    Shader shader;
};

#endif