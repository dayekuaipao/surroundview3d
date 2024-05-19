#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "common/omni_camera_model.hpp"
#include "bowl_model.h"
#include "shader.h"
#include "capture.h"
#include "glcamera.h"
#if USE_YUV
#include "yuyv_drawer.h"
#else
#include "rgb_drawer.h"
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstring>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <chrono>


using namespace::std;
using namespace::cv;

// settings
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
    #if USE_CAPTURE
    vector<Capture*> captures;
    #endif
    for(int c=0;c<NUM_CAMERA;c++)
    {
        OmniCameraModel cameraModel;
        string path = string(DATAPATH)+string("/params/intrinsic/intrinsic_")+to_string(c)+".yaml";
        cameraModel.readKD(path);
        cameraModels.push_back(cameraModel);
        #if USE_CAPTURE
        Capture* capture = new Capture(cameraModel.imageSize.width,cameraModel.imageSize.height,CAPTURE_FORMAT);
        captures.push_back(capture);
        #endif
    }

    if(init_glfw()!=0)
    {
        return 1;
    }
    if(init_glad()!=0)
    {
        return 2;
    }

    #if USE_YUV
    string fragment_shader_path = string(SHADERPATH)+string("/yuv_fragment.glsl");
    #else
    string fragment_shader_path = string(SHADERPATH)+string("/rgb_fragment.glsl");
    #endif
    string vertex_shader_path = string(SHADERPATH)+string("/vertex.glsl");
    Shader shader(vertex_shader_path.c_str(), fragment_shader_path.c_str());
    GLuint VAOs[NUM_CAMERA],VBOs[NUM_CAMERA],EBOs[NUM_CAMERA];
    glGenVertexArrays(NUM_CAMERA, VAOs);
    glGenBuffers(NUM_CAMERA, VBOs);
    glGenBuffers(NUM_CAMERA, EBOs);

    float startAngles[NUM_CAMERA] = {-17.5,7,84,162,187,264};
    float angles[NUM_CAMERA] = {35,87,87,35,87,87};
    int nopOfArcs[NUM_CAMERA] = {35,87,87,35,87,87};
    int nopOfFusions[NUM_CAMERA] = {10,10,10,10,10,10};

    vector<CameraDrawer*> drawers;
    for(int c=0;c<NUM_CAMERA;c++)
    {
        BowlModel model{300,1360,200,500,200,100,startAngles[c],angles[c],nopOfArcs[c],nopOfFusions[c]};

        #if USE_CAPTURE
        Capture* capture = captures[c];
        stringstream ss;
        ss<<"/dev/video"<<2*c+2;
        capture->initCapture(ss.str().c_str());
        capture->startCapture();
        #endif
        OmniCameraModel cameraModel = cameraModels[c];
        cameraModel.readRT(string(DATAPATH)+string("/params/extrinsic/extrinsic_")+to_string(c)+".yaml");

        #if USE_YUYV
        YUYVDrawer* drawer = new YUYVDrawer(cameraModel,model,shader);
        #else
        RGBDrawer* drawer = new RGBDrawer(cameraModel,model,shader);
        #endif

        drawer->initVertex(VAOs[c],VBOs[c],EBOs[c]);
        drawer->initTexture();
        drawers.push_back(drawer);
    }
    gl_set();
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

        for(int i=0;i<NUM_CAMERA;i++)
        {
            glm::mat4 model = glm::identity<glm::mat4>();
            Buffer frame;
            #if USE_CAPTURE
            Capture* capture = captures[i];
            frame = capture->getLastFrame();
            #else
            Mat image = imread(string(DATAPATH)+string("/images/calib_extrinsic/image_")+to_string(i)+".jpg");
            frame.length = image.total()*image.channels();
            frame.data = image.data;
            #endif
            drawers[i]->draw(frame,projection,view,model);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    #if USE_CAPTURE
    for(int i=0;i<NUM_CAMERA;i++)
    {
        captures[i]->stopCapture();
        delete captures[i];
    }
     #endif
    glDeleteVertexArrays(NUM_CAMERA, VAOs);
    glDeleteBuffers(NUM_CAMERA, VBOs);
    glDeleteBuffers(NUM_CAMERA, EBOs);
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// Enable blending
    glEnable(GL_CULL_FACE);  // Cull back face
}

void gl_clear()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}