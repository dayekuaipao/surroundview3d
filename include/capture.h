#ifndef CAPTURE_H
#define CAPTURE_H

#include <mutex>
#include <atomic>
#include <stdint.h>
#include <thread>
#include <vector>
#include <linux/videodev2.h>

struct Buffer
{
    uint8_t *data;
    size_t length;
};

class Capture
{
public:
    Capture(int width,int height,unsigned int format):
            mWidth(width),mHeight(height),mFormat(format){};
    ~Capture();

    int initCapture(const char* deviceName);
    int startCapture();
    int stopCapture();
    const Buffer& getLastFrame();

private:
    unsigned int mFormat;
    int mWidth;
    int mHeight;
    int mFileDesc = -1;
    Buffer mBuffer;
    int print_caps();
    int init_mmap();
    void reset();
    int cnt=0;
};

#endif
