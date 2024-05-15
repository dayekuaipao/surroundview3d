#include "camera_drawer.h"

CameraDrawer::CameraDrawer(const OmniCameraModel& cameraModel,const ObjectModel& objectModel,Shader shader):shader(shader)
{
    width = cameraModel.imageSize.width;
    height = cameraModel.imageSize.height;
    vector<Point3f> positions = objectModel.getPositions();

    vector<Point2f> texCoords_;
    cameraModel.project(positions,texCoords_);

    vector<Point2f> texCoords;
    for(auto& texCoord_:texCoords_)
    {
        Point2f p = Point2f{texCoord_.x/width,texCoord_.y/height};
        texCoords.push_back(p);
    }

    vector<float> alphas = objectModel.getAlphas();

    p_vertexes = new float[positions.size()*6];
    positionSize = positions.size();
    for(int i=0;i<positions.size();i++)
    {
        p_vertexes[6*i] = positions[i].x;
        p_vertexes[6*i+1] = positions[i].y;
        p_vertexes[6*i+2] = positions[i].z;
        p_vertexes[6*i+3] = texCoords[i].x;
        p_vertexes[6*i+4] = texCoords[i].y;
        p_vertexes[6*i+5] = alphas[i];
    }

    vector<unsigned int> indexes = objectModel.getIndexes();
    p_indexes = new unsigned int[indexes.size()];
    for(int i=0;i<indexes.size();i++)
    {
        p_indexes[i]=indexes[i];
    }
    indexSize = indexes.size();
}

void CameraDrawer::initVertex(GLuint& VAO,GLuint& VBO,GLuint& EBO)
{
    mVAO = VAO;
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, positionSize*6*sizeof(float), p_vertexes, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(sizeof(float)*5));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize * sizeof(unsigned int), p_indexes, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CameraDrawer::draw(const Buffer& frame,const glm::mat4& projection,const glm::mat4& view,const glm::mat4& model) const
{
    setTexture(frame);
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setMat4("model", model);
    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, indexSize, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}