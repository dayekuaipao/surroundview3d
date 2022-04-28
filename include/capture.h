#ifndef CAPTURE_H
#define CAPTURE_H

#include <mutex>
#include <atomic>
#include <stdint.h>
#include <thread>
#include <vector>
#include <linux/videodev2.h>

#define WIDTH 1280
#define HEIGHT 720

#define IOCTL_RETRY 3
#define BUF_COUNT 4
#define MAX_CAMERA_COUNT 10
#define VIDEO_MAX_PLANES 8

#define READ_MODE 1
#define WRITE_MODE 2

//V4L2_MEMORY_MMAP             = 1,
//V4L2_MEMORY_USERPTR          = 2,
//V4L2_MEMORY_OVERLAY          = 3,
//V4L2_MEMORY_DMABUF           = 4,
#define MEMORY_TYPE 1

#define VIDEO_FORMAT V4L2_PIX_FMT_NV12
//#define VIDEO_FORMAT V4L2_PIX_FMT_BGR24
//#define VIDEO_FORMAT V4L2_PIX_FMT_RGB24

struct Frame
{
    uint64_t frame_id = 0;
    uint8_t *last_data = nullptr;
    uint8_t *data = nullptr;
    uint16_t width = 0;
    uint16_t height = 0;
    uint8_t channels = 0;
    uint8_t split = 1;
};

struct Buffer
{
    uint8_t *start;
    size_t length;
};

void video_enum_formats(char *video_name);

class Capture
{
public:
    Capture();
    Capture(int width,int height,unsigned int format=VIDEO_FORMAT);
    Capture(const Capture &capture);
    virtual ~Capture();

    bool initCapture(int devId );
    bool initCapture(int devId ,unsigned int format);
    const Frame &getLastFrame();
    void stopCapture() { mCapture = false; }

    unsigned int format;
//    unsigned int capture_type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enum v4l2_buf_type  capture_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    int mWidth = 0;
    int mHeight = 0;
    int mChannels = 0;
    int mSplit = 1;

    void setFlipmode(bool mode) //设置镜像模式
    {
        fip_mutex.lock();
        m_flipMode = mode;
        fip_mutex.unlock();
    }
    bool getFlipmode()
    {
        fip_mutex.lock();
        bool tmpMode =  m_flipMode;
        fip_mutex.unlock();

        return tmpMode;
    }
    bool getSensorSta()
    {
        return mSensorSta;
    }


private:
    bool mNewFrame = false;
    bool mInitialized = false;
    bool mCapture = false;
    bool mSensorSta = false;
    bool m_flipMode = false;

    int mDevId = 0;
    std::string mDevName;
    int mFileDesc = -1;

    std::mutex mBufMutex;
    std::mutex fip_mutex;

    Frame mLastFrame;
    uint8_t mBufCount = 9;
//    std::atomic_uchar mCurrentIndex=0;
    uint8_t mCurrentIndex = 0;
    Buffer *mBuffers = nullptr;

    std::thread mGrabThread;
    bool mFirstFrame = true;

    void grabThreadFunc();
    bool openCamera(uint8_t devId);
    bool startCapture();
    void reset();

    int setFormat();
    bool checkDevice();
    bool checkResolution();
    bool prepareBuffers();

    int xioctl(int fd, uint64_t IOCTL_X, void *arg);
};

#endif
