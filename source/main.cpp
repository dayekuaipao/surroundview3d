#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "omni_camera_model.h"
#include "bowl_model.h"
#include "shader.h"
#include "capture.h"
#include "glcamera.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <bits/stdint-uintn.h>
#include <cstring>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv4/opencv2/core/persistence.hpp>
#include <opencv4/opencv2/core/types.hpp>
#include <string>
#include <strings.h>
#include <vector>

#define CAPTURE_WIDTH 1280
#define CAPTURE_HEIGHT 720
#define CAPTURE_NUM 4
#define CAPTURE_FORMAT V4L2_PIX_FMT_NV12
#define CAPTURE_CHANNEL 3
#define CAPTURE_SPLIT 2

using namespace::std;
using namespace::cv;

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

float borderColor[] = {1.0f,1.0f,1.0f,0.0f};

// camera
glCamera camera(glm::vec3(0.0f, 0.0f, 100.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

int main()
{

    unsigned int indexSize[CAPTURE_NUM];

    uint8_t* frameData[CAPTURE_NUM];
    vector<OmniCameraModel*> cameraModels;
    vector<Capture> captures;
    for(int c=0;c<CAPTURE_NUM;c++)
    {
        Capture capture(CAPTURE_WIDTH,CAPTURE_HEIGHT,CAPTURE_FORMAT);
        captures.push_back(capture);
        OmniCameraModel* cameraModel = new OmniCameraModel();
        // vector<Mat> images;
        // for(int i=0;i<20;i++)
        // {
        //     Mat image = imread("../resource/images/"+to_string(c)+"/image"+to_string(i)+".jpg");
        //     images.push_back(image);
        // }
        // cameraModel->computeKD(images,cv::Size(11,8));
        string path = "../resource/parameters/camera_"+to_string(c)+".yaml";
        cameraModel->readKD(path);
        cameraModels.push_back(cameraModel);
    }

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Surround3D", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);  // Enable blending
    glEnable(GL_CULL_FACE);  // Cull back face
    
    Shader shader("../resource/shaders/vertex.glsl", "../resource/shaders/fragment.glsl");
    GLuint VBOs[CAPTURE_NUM], VAOs[CAPTURE_NUM], EBOs[CAPTURE_NUM];
    glGenVertexArrays(CAPTURE_NUM, VAOs);
    glGenBuffers(CAPTURE_NUM, VBOs);
    glGenBuffers(CAPTURE_NUM, EBOs);


    float startAngles[CAPTURE_NUM] = {-25,15,155,195};
    float angles[CAPTURE_NUM] = {50,150,50,150};
    int nopOfArcs[CAPTURE_NUM] = {50,150,50,150};
    int nopOfFusions[CAPTURE_NUM] = {10,10,10,10};


    for(int c=0;c<CAPTURE_NUM;c++)
    {
        Capture& capture = captures[c];
        BowlModel model{380,800,200,300,500,200,startAngles[c],angles[c],nopOfArcs[c],nopOfFusions[c]};
        stringstream ss;
        ss<<"/dev/video"<<2+2*c;
        capture.initCapture(ss.str().c_str());
        capture.startCapture();

        OmniCameraModel* cameraModel = cameraModels[c];
        
        // Mat worldPoints;
        // string worldPointsString = "WorldPoints"+to_string(c);
        // parameter[worldPointsString]>>worldPoints;

        // Mat imagePoints;
        // string imagePointsString = "ImagePoints"+to_string(c);
        // parameter[imagePointsString]>>imagePoints;

        //cameraModel->computeRT(imagePoints,worldPoints);
        cameraModel->readRT("../resource/parameters/camera_"+to_string(c)+".yaml");

        vector<Point3f> positions = model.getPositions();
        vector<Point2f> texCoords;
        vector<unsigned int> indexes = model.getIndexes();

        cameraModel->project(positions,texCoords);

        vector<Point2f> texCoords_;
        for(auto& texCoord:texCoords)
        {
            Point2f p = Point2f{texCoord.x/cameraModel->imageSize.width,texCoord.y/cameraModel->imageSize.height};
            texCoords_.push_back(p);
        }

        vector<float> alphas = model.getAlphas();

        float* p_vertexes = new float[positions.size()*6];

        for(int i=0;i<positions.size();i++)
        {
            p_vertexes[6*i] = positions[i].x;
            p_vertexes[6*i+1] = positions[i].y;
            p_vertexes[6*i+2] = positions[i].z;
            p_vertexes[6*i+3] = texCoords_[i].x;
            p_vertexes[6*i+4] = texCoords_[i].y;
            p_vertexes[6*i+5] = alphas[i];
        }
        indexSize[c] = indexes.size();
        unsigned int* p_indexes = new unsigned int[indexes.size()];
        for(int i=0;i<indexes.size();i++)
        {
            p_indexes[i]=indexes[i];
        }

        glBindVertexArray(VAOs[c]);

        glBindBuffer(GL_ARRAY_BUFFER, VBOs[c]);
        glBufferData(GL_ARRAY_BUFFER, positions.size()*6*sizeof(float), p_vertexes, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(sizeof(float)*3));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(sizeof(float)*5));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[c]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), p_indexes, GL_STATIC_DRAW);

        frameData[c] = new uint8_t[CAPTURE_WIDTH*CAPTURE_HEIGHT*CAPTURE_CHANNEL/CAPTURE_SPLIT];
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    shader.use();

    GLuint yTexture;
    glGenTextures(1, &yTexture);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTextureParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, CAPTURE_WIDTH, CAPTURE_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
    shader.setInt("yTexture",0);

    GLuint uvTexture;
    glGenTextures(1, &uvTexture);
    glBindTexture(GL_TEXTURE_2D, uvTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTextureParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAPTURE_WIDTH/2, CAPTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    shader.setInt("uvTexture",1);

    std::cout<<"render start"<<std::endl;
    while (!glfwWindowShouldClose(window))
    {

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 3000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(int i=0;i<CAPTURE_NUM;i++)
        {
            Capture& capture = captures[i];
            Buffer frame = capture.getLastFrame();
            memcpy(frameData[i],frame.data,CAPTURE_WIDTH*CAPTURE_HEIGHT*CAPTURE_CHANNEL/CAPTURE_SPLIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, yTexture);
            glTexSubImage2D(GL_TEXTURE_2D,0,0,0,CAPTURE_WIDTH,CAPTURE_HEIGHT,GL_RG,GL_UNSIGNED_BYTE,frameData[i]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, uvTexture);
            glTexSubImage2D(GL_TEXTURE_2D,0,0,0,CAPTURE_WIDTH/2,CAPTURE_HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,frameData[i]);

            glm::mat4 model = glm::identity<glm::mat4>();

            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);
            shader.setMat4("model", model);
            glBindVertexArray(VAOs[i]);
            glDrawElements(GL_TRIANGLES, indexSize[i], GL_UNSIGNED_INT, 0);
            
            //glBindVertexArray(0);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for(int i=0;i<CAPTURE_NUM;i++)
    {
        delete cameraModels[i];
        delete [] frameData[i];
        captures[i].stopCapture();
    }
    
    glDeleteVertexArrays(CAPTURE_NUM, VAOs);
    glDeleteBuffers(CAPTURE_NUM, VBOs);
    glDeleteBuffers(CAPTURE_NUM, EBOs);
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
