#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "omni_camera_model.h"
#include "bowl_model.h"
#include "shader.h"
#include "capture.h"
#include "glcamera.h"
#include "yuyv_drawer.h"
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
#include <chrono>

#define CAPTURE_NUM 4
#define CAPTURE_FORMAT V4L2_PIX_FMT_NV12

using namespace::std;
using namespace::cv;

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
int init_glad();
int init_glfw();
void gl_set();
void gl_clear();

// camera
glCamera camera(glm::vec3(0.0f, 0.0f, 100.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;
GLFWwindow* window;

int main()
{
    vector<OmniCameraModel> cameraModels;
    vector<Capture*> captures;

    for(int c=0;c<CAPTURE_NUM;c++)
    {
        OmniCameraModel cameraModel;
        // vector<Mat> images;
        // for(int i=0;i<20;i++)
        // {
        //     Mat image = imread("../resource/images/"+to_string(c)+"/image"+to_string(i)+".jpg");
        //     images.push_back(image);
        // }
        // cameraModel.computeKD(images,cv::Size(11,8));
        string path = "../resource/parameters/camera_"+to_string(c)+".yaml";
        cameraModel.readKD(path);
        cameraModels.push_back(cameraModel);
        Capture* capture = new Capture(cameraModel.imageSize.width,cameraModel.imageSize.height,CAPTURE_FORMAT);
        captures.push_back(capture);
    }

    if(init_glfw()!=0)
    {
        return 1;
    }
    if(init_glad()!=0)
    {
        return 2;
    }

    Shader shader("../resource/shaders/vertex.glsl", "../resource/shaders/yuv_fragment.glsl");
    GLuint VAOs[CAPTURE_NUM],VBOs[CAPTURE_NUM],EBOs[CAPTURE_NUM];
    glGenVertexArrays(CAPTURE_NUM, VAOs);
    glGenBuffers(CAPTURE_NUM, VBOs);
    glGenBuffers(CAPTURE_NUM, EBOs);

    float startAngles[CAPTURE_NUM] = {-25,15,155,195};
    float angles[CAPTURE_NUM] = {50,150,50,150};
    int nopOfArcs[CAPTURE_NUM] = {50,150,50,150};
    int nopOfFusions[CAPTURE_NUM] = {10,10,10,10};

    vector<CameraDrawer*> drawers;
    for(int c=0;c<CAPTURE_NUM;c++)
    {
        Capture* capture = captures[c];
        BowlModel model{380,800,200,300,500,200,startAngles[c],angles[c],nopOfArcs[c],nopOfFusions[c]};
        stringstream ss;
        ss<<"/dev/video"<<2*c+2;
        capture->initCapture(ss.str().c_str());
        capture->startCapture();

        OmniCameraModel cameraModel = cameraModels[c];
        // Mat worldPoints;
        // string worldPointsString = "WorldPoints"+to_string(c);
        // parameter[worldPointsString]>>worldPoints;

        // Mat imagePoints;
        // string imagePointsString = "ImagePoints"+to_string(c);
        // parameter[imagePointsString]>>imagePoints;

        //cameraModel->computeRT(imagePoints,worldPoints);
        cameraModel.readRT("../resource/parameters/camera_"+to_string(c)+".yaml");

        YUYVDrawer* drawer = new YUYVDrawer(cameraModel,model,shader);
        drawer->initVertex(VAOs[c],VBOs[c],EBOs[c]);
        drawer->initTexture();
        drawers.push_back(drawer);
    }

    std::cout<<"render start"<<std::endl;
    while (!glfwWindowShouldClose(window))
    {

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);
        gl_clear();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 3000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        for(int i=0;i<CAPTURE_NUM;i++)
        {
            glm::mat4 model = glm::identity<glm::mat4>();
            Capture* capture = captures[i];
            Buffer frame = capture->getLastFrame();
            drawers[i]->draw(frame,projection,view,model);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for(int i=0;i<CAPTURE_NUM;i++)
    {
        captures[i]->stopCapture();
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


int init_glfw()
{
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
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Surround3D", NULL, NULL);
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
    return 0;
}

int init_glad()
{
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }
    else
    {
        return 0;
    }
}

void gl_set()
{
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);  // Enable blending
    glEnable(GL_CULL_FACE);  // Cull back face
}

void gl_clear()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}