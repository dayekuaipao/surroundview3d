#include "capture.h"
#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <opencv4/opencv2/imgproc.hpp>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static int xioctl(int fd, int request, void *arg)
{
        int r;
 
        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);
 
        return r;
}

Capture::~Capture()
{
    reset();
}

int Capture::initCapture(const char* deviceName)
{
    mFileDesc = open(deviceName,O_RDWR);
    if (mFileDesc == -1)
    {
        perror("Opening video device");
        return 1;
    }
    int res;
    res = print_caps();
    if (res!=0)
    {
        return res;
    }
    res = init_mmap();
    if (res!=0)
    {
        return res;
    }
    return startCapture();
}

int Capture::startCapture()
{ 
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(mFileDesc, VIDIOC_STREAMON, &type))
    {
        perror("Start Capture");
        return 1;
    }
    return 0;
}

int Capture::stopCapture()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(mFileDesc, VIDIOC_STREAMOFF, &type))
    {
        perror("Stop Capture");
        return 1;
    }
    return 0;
}

const Buffer& Capture::getLastFrame()
{
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(mFileDesc, VIDIOC_QBUF, &buf))
    {
        perror("Query Buffer");
    }
    if(-1 == xioctl(mFileDesc, VIDIOC_DQBUF, &buf))
    {
        perror("Retrieving Frame");
    }
    return mBuffer;
}

int Capture::print_caps()
{
    struct v4l2_capability caps = {};
    if (-1 == xioctl(mFileDesc, VIDIOC_QUERYCAP, &caps))
    {
            perror("Querying Capabilities");
            return 1;
    }
 
    printf( "Driver Caps:\n"
            "  Driver: \"%s\"\n"
            "  Card: \"%s\"\n"
            "  Bus: \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n",
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&0xff,
            (caps.version>>24)&0xff,
            caps.capabilities);
 
 
    struct v4l2_cropcap cropcap = {0};
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl (mFileDesc, VIDIOC_CROPCAP, &cropcap))
    {
            perror("Querying Cropping Capabilities");
            return 1;
    }
 
    printf( "Camera Cropping:\n"
            "  Bounds: %dx%d+%d+%d\n"
            "  Default: %dx%d+%d+%d\n"
            "  Aspect: %d/%d\n",
            cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
            cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
            cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);
 
    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char fourcc[5] = {0};
    char c, e;
    printf("  FMT : CE Desc\n--------------------\n");
    while (0 == xioctl(mFileDesc, VIDIOC_ENUM_FMT, &fmtdesc))
    {
            strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
            c = fmtdesc.flags & 1? 'C' : ' ';
            e = fmtdesc.flags & 2? 'E' : ' ';
            printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
            fmtdesc.index++;
    }
 
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = mWidth;
    fmt.fmt.pix.height = mHeight;
    fmt.fmt.pix.pixelformat = mFormat;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (-1 == xioctl(mFileDesc, VIDIOC_S_FMT, &fmt))
    {
        perror("Setting Pixel Format");
        return 1;
    }
 
    strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
    printf( "Selected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
           fourcc,
            fmt.fmt.pix.field);
    return 0;
}

int Capture::init_mmap()
{
    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
 
    if (-1 == xioctl(mFileDesc, VIDIOC_REQBUFS, &req))
    {
        perror("Requesting Buffer");
        return 1;
    }
 
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(mFileDesc, VIDIOC_QUERYBUF, &buf))
    {
        perror("Querying Buffer");
        return 1;
    }
 
    mBuffer.data = (uint8_t *)mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, mFileDesc, buf.m.offset);
    mBuffer.length = buf.length;
    printf("Length: %d\nAddress: %p\n", buf.length, mBuffer.data);
    printf("Image Length: %d\n", buf.bytesused);
 
    return 0;
}

void Capture::reset()
{
    stopCapture();
    munmap(mBuffer.data, mBuffer.length);
    if(mFileDesc)
    {
        close(mFileDesc);
        mFileDesc = -1;
    }
}
