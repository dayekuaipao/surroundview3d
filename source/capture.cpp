#include "capture.h"
#include <fcntl.h>

#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <iostream>

#if MEMORY_TYPE == 2
struct cma_frame_allocator m_allocator(4);
#endif

Capture::Capture()
{
    mWidth = WIDTH;
    mHeight = HEIGHT;
    mBufCount = BUF_COUNT;
    format=VIDEO_FORMAT;
}
Capture::Capture(int width,int height,unsigned int format)
{
    mWidth = width;
    mHeight = height;
    mBufCount = BUF_COUNT;
    this->format=format;
}

Capture::~Capture()
{
    reset();
}

void Capture::reset()
{
    mCapture = false;
    if (mGrabThread.joinable())
    {
        mGrabThread.join();
    }
    enum v4l2_buf_type type;
    type = capture_type;
    if (mFileDesc != -1)
    {
        xioctl(mFileDesc, VIDIOC_STREAMOFF, &type);
    }
    if (mInitialized && mBuffers)
    {
        for (unsigned int i = 0; i < mBufCount; ++i)
        {
            munmap(mBuffers[i].start, mBuffers[i].length);
        }
        if (mBuffers)
        {
            free(mBuffers);
        }
        mBuffers = nullptr;
    }
    if (mFileDesc)
    {
        close(mFileDesc);
    }
    if (mLastFrame.data)
    {
        delete[] mLastFrame.data;
        mLastFrame.data = nullptr;
    }
    mInitialized = false;
}

bool Capture::initCapture(int devId)
{
//    reset();
    if(mInitialized){

        fprintf(stderr,"Capture alreadly init\r\n");
        return false;
    }
    mInitialized=true;

    bool opened = false;

    opened = openCamera(static_cast<uint8_t>(devId));

    if (!opened)
    {
        return false;
    }
    return startCapture();
}
bool Capture::initCapture(int devId ,unsigned int format)
{
//    reset();
    this->format=format;
    if(mInitialized){

        fprintf(stderr,"Capture alreadly init\r\n");
        return false;
    }
    mInitialized=true;

    bool opened = false;

    opened = openCamera(static_cast<uint8_t>(devId));

    if (!opened)
    {
        return false;
    }
    return startCapture();
}

const Frame &Capture::getLastFrame()
{
    const std::lock_guard<std::mutex> lock(mBufMutex);
    mNewFrame = false;
    return mLastFrame;
}


bool Capture::startCapture()
{

//gwj start video stream
      enum v4l2_buf_type type;
      type = capture_type;
      if (ioctl(mFileDesc,VIDIOC_STREAMON,&type) == -1) {
          perror("VIDIOC_STREAMON failed");
      }
    mGrabThread = std::thread(&Capture::grabThreadFunc, this);
//    sleep(1);
    return true;
}

void Capture::grabThreadFunc()
{
    prctl(PR_SET_NAME,"screen:video_capture");
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(v4l2_buffer));
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    memset(planes, 0, sizeof(planes));
    buf.type = capture_type;
    buf.memory = MEMORY_TYPE;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;
    mNewFrame = false;
    mCapture = true;

    uint64_t rel_ts = 0;

    mFirstFrame = true;

    struct timeval last={0,0},current={0,0};
      double fps = 0;
      int count=0;

      mCurrentIndex = 0;
      mLastFrame.data=(unsigned char *)mBuffers[mCurrentIndex].start;
      mLastFrame.last_data=(unsigned char *)mBuffers[mCurrentIndex].start;

    while (mCapture)
    {
        gettimeofday(&current,NULL);
        //current.tv_sec * 1000 + current.tv_usec / 1000;
        fps = (current.tv_sec - last.tv_sec) * 1000000
            + current.tv_usec - last.tv_usec;
        fps = fps ? 1000000.0 / fps : 0.0;

        int ret = ioctl(mFileDesc, VIDIOC_DQBUF, &buf);
        if(ret==-1){
            perror("GetFrame VIDIOC_DQBUF Failed");
            mSensorSta = false;
            sleep(1);
//            return -1;
        }
//        printf("%d mem[plane].length %d ,mem[plane].start :%x \r\n",buf.index,buf.m.planes[0].length,buf.m.planes[0].m.userptr);

        mLastFrame.last_data=mLastFrame.data;
        mBufMutex.lock();
        mCurrentIndex = buf.index;
        mLastFrame.data=(unsigned char *)mBuffers[mCurrentIndex].start;
        mBufMutex.unlock();

//        printf("tuser last data1 : %d  vc: %d \r\n",mLastFrame.last_data[mLastFrame.width *2*2879-3],mLastFrame.last_data[mLastFrame.width *2*2879-1]);
//        printf("tuser data1 : %d  vc: %d \r\n",mLastFrame.data[mLastFrame.width *2*2-3],mLastFrame.data[mLastFrame.width *2*2-1]);

        mSensorSta = true;

        if (ioctl(mFileDesc, VIDIOC_QBUF, &buf) < 0) {
            perror("GetFrame VIDIOC_QBUF Failed");
        }
        count++;
        // if(count%60==0){
        //    printf("video:%d :fps%f\n",mDevId,fps);
        //     fflush(stdout);
        // }
        last=current;
    }
}

#define ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))

static struct {
    enum v4l2_buf_type type;
    bool supported;
    const char *name;
    const char *string;
} buf_types[] = {
    { V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, 1, "Video capture mplanes", "capture-mplane", },
    { V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, 1, "Video output", "output-mplane", },
    { V4L2_BUF_TYPE_VIDEO_CAPTURE, 1, "Video capture", "capture", },
    { V4L2_BUF_TYPE_VIDEO_OUTPUT, 1, "Video output mplanes", "output", },
    { V4L2_BUF_TYPE_VIDEO_OVERLAY, 0, "Video overlay", "overlay" },
    { V4L2_BUF_TYPE_META_CAPTURE, 1, "Meta-data capture", "meta-capture", },
//    { V4L2_BUF_TYPE_META_OUTPUT, 1, "Meta-data output", "meta-output", },
};
static struct v4l2_format_info {
    const char *name;
    unsigned int fourcc;
    unsigned char n_planes;
} pixel_formats[] = {
    { "RGB332", V4L2_PIX_FMT_RGB332, 1 },
    { "RGB444", V4L2_PIX_FMT_RGB444, 1 },
    { "ARGB444", V4L2_PIX_FMT_ARGB444, 1 },
    { "XRGB444", V4L2_PIX_FMT_XRGB444, 1 },
    { "RGB555", V4L2_PIX_FMT_RGB555, 1 },
    { "ARGB555", V4L2_PIX_FMT_ARGB555, 1 },
    { "XRGB555", V4L2_PIX_FMT_XRGB555, 1 },
    { "RGB565", V4L2_PIX_FMT_RGB565, 1 },
    { "RGB555X", V4L2_PIX_FMT_RGB555X, 1 },
    { "RGB565X", V4L2_PIX_FMT_RGB565X, 1 },
    { "BGR666", V4L2_PIX_FMT_BGR666, 1 },
    { "BGR24", V4L2_PIX_FMT_BGR24, 1 },
    { "RGB24", V4L2_PIX_FMT_RGB24, 1 },
    { "BGR32", V4L2_PIX_FMT_BGR32, 1 },
    { "ABGR32", V4L2_PIX_FMT_ABGR32, 1 },
    { "XBGR32", V4L2_PIX_FMT_XBGR32, 1 },
    { "RGB32", V4L2_PIX_FMT_RGB32, 1 },
    { "ARGB32", V4L2_PIX_FMT_ARGB32, 1 },
    { "XRGB32", V4L2_PIX_FMT_XRGB32, 1 },
    { "HSV24", V4L2_PIX_FMT_HSV24, 1 },
    { "HSV32", V4L2_PIX_FMT_HSV32, 1 },
    { "Y8", V4L2_PIX_FMT_GREY, 1 },
    { "Y10", V4L2_PIX_FMT_Y10, 1 },
    { "Y12", V4L2_PIX_FMT_Y12, 1 },
    { "Y16", V4L2_PIX_FMT_Y16, 1 },
    { "UYVY", V4L2_PIX_FMT_UYVY, 1 },
    { "VYUY", V4L2_PIX_FMT_VYUY, 1 },
    { "YUYV", V4L2_PIX_FMT_YUYV, 1 },
    { "YVYU", V4L2_PIX_FMT_YVYU, 1 },
    { "NV12", V4L2_PIX_FMT_NV12, 1 },
    { "NV12M", V4L2_PIX_FMT_NV12M, 2 },
    { "NV21", V4L2_PIX_FMT_NV21, 1 },
    { "NV21M", V4L2_PIX_FMT_NV21M, 2 },
    { "NV16", V4L2_PIX_FMT_NV16, 1 },
    { "NV16M", V4L2_PIX_FMT_NV16M, 2 },
    { "NV61", V4L2_PIX_FMT_NV61, 1 },
    { "NV61M", V4L2_PIX_FMT_NV61M, 2 },
    { "NV24", V4L2_PIX_FMT_NV24, 1 },
    { "NV42", V4L2_PIX_FMT_NV42, 1 },
    { "YUV420M", V4L2_PIX_FMT_YUV420M, 3 },
    { "YUV422M", V4L2_PIX_FMT_YUV422M, 3 },
    { "YUV444M", V4L2_PIX_FMT_YUV444M, 3 },
    { "YVU420M", V4L2_PIX_FMT_YVU420M, 3 },
    { "YVU422M", V4L2_PIX_FMT_YVU422M, 3 },
    { "YVU444M", V4L2_PIX_FMT_YVU444M, 3 },
    { "SBGGR8", V4L2_PIX_FMT_SBGGR8, 1 },
    { "SGBRG8", V4L2_PIX_FMT_SGBRG8, 1 },
    { "SGRBG8", V4L2_PIX_FMT_SGRBG8, 1 },
    { "SRGGB8", V4L2_PIX_FMT_SRGGB8, 1 },
    { "SBGGR10_DPCM8", V4L2_PIX_FMT_SBGGR10DPCM8, 1 },
    { "SGBRG10_DPCM8", V4L2_PIX_FMT_SGBRG10DPCM8, 1 },
    { "SGRBG10_DPCM8", V4L2_PIX_FMT_SGRBG10DPCM8, 1 },
    { "SRGGB10_DPCM8", V4L2_PIX_FMT_SRGGB10DPCM8, 1 },
    { "SBGGR10", V4L2_PIX_FMT_SBGGR10, 1 },
    { "SGBRG10", V4L2_PIX_FMT_SGBRG10, 1 },
    { "SGRBG10", V4L2_PIX_FMT_SGRBG10, 1 },
    { "SRGGB10", V4L2_PIX_FMT_SRGGB10, 1 },
    { "SBGGR10P", V4L2_PIX_FMT_SBGGR10P, 1 },
    { "SGBRG10P", V4L2_PIX_FMT_SGBRG10P, 1 },
    { "SGRBG10P", V4L2_PIX_FMT_SGRBG10P, 1 },
    { "SRGGB10P", V4L2_PIX_FMT_SRGGB10P, 1 },
    { "SBGGR12", V4L2_PIX_FMT_SBGGR12, 1 },
    { "SGBRG12", V4L2_PIX_FMT_SGBRG12, 1 },
    { "SGRBG12", V4L2_PIX_FMT_SGRBG12, 1 },
    { "SRGGB12", V4L2_PIX_FMT_SRGGB12, 1 },
//    { "IPU3_SBGGR10", V4L2_PIX_FMT_IPU3_SBGGR10, 1 },
//    { "IPU3_SGBRG10", V4L2_PIX_FMT_IPU3_SGBRG10, 1 },
//    { "IPU3_SGRBG10", V4L2_PIX_FMT_IPU3_SGRBG10, 1 },
//    { "IPU3_SRGGB10", V4L2_PIX_FMT_IPU3_SRGGB10, 1 },
    { "DV", V4L2_PIX_FMT_DV, 1 },
    { "MJPEG", V4L2_PIX_FMT_MJPEG, 1 },
    { "MPEG", V4L2_PIX_FMT_MPEG, 1 },
};

static void list_formats(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pixel_formats); i++)
        printf("%s (\"%c%c%c%c\", %u planes)\n",
               pixel_formats[i].name,
               pixel_formats[i].fourcc & 0xff,
               (pixel_formats[i].fourcc >> 8) & 0xff,
               (pixel_formats[i].fourcc >> 16) & 0xff,
               (pixel_formats[i].fourcc >> 24) & 0xff,
               pixel_formats[i].n_planes);
}

static const struct v4l2_format_info *v4l2_format_by_fourcc(unsigned int fourcc)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pixel_formats); ++i) {
        if (pixel_formats[i].fourcc == fourcc)
            return &pixel_formats[i];
    }

    return NULL;
}
static const char *v4l2_format_name(unsigned int fourcc)
{
    const struct v4l2_format_info *info;
    static char name[5];
    unsigned int i;

    info = v4l2_format_by_fourcc(fourcc);
    if (info)
        return info->name;

    for (i = 0; i < 4; ++i) {
        name[i] = fourcc & 0xff;
        fourcc >>= 8;
    }

    name[4] = '\0';
    return name;
}
static const char *v4l2_buf_type_name(enum v4l2_buf_type type)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(buf_types); ++i) {
        if (buf_types[i].type == type)
            return buf_types[i].name;
    }

    if (type & V4L2_BUF_TYPE_PRIVATE)
        return "Private";
    else
        return "Unknown";
}
static void video_enum_frame_intervals(int fd, __u32 pixelformat,
    unsigned int width, unsigned int height)
{
    struct v4l2_frmivalenum ival;
    unsigned int i;
    int ret;

    for (i = 0; ; ++i) {
        memset(&ival, 0, sizeof ival);
        ival.index = i;
        ival.pixel_format = pixelformat;
        ival.width = width;
        ival.height = height;
        ret = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival);
        if (ret < 0)
            break;

        if (i != ival.index)
            printf("Warning: driver returned wrong ival index "
                "%u.\n", ival.index);
        if (pixelformat != ival.pixel_format)
            printf("Warning: driver returned wrong ival pixel "
                "format %08x.\n", ival.pixel_format);
        if (width != ival.width)
            printf("Warning: driver returned wrong ival width "
                "%u.\n", ival.width);
        if (height != ival.height)
            printf("Warning: driver returned wrong ival height "
                "%u.\n", ival.height);

        if (i != 0)
            printf(", ");

        switch (ival.type) {
        case V4L2_FRMIVAL_TYPE_DISCRETE:
            printf("%u/%u",
                ival.discrete.numerator,
                ival.discrete.denominator);
            break;

        case V4L2_FRMIVAL_TYPE_CONTINUOUS:
            printf("%u/%u - %u/%u",
                ival.stepwise.min.numerator,
                ival.stepwise.min.denominator,
                ival.stepwise.max.numerator,
                ival.stepwise.max.denominator);
            return;

        case V4L2_FRMIVAL_TYPE_STEPWISE:
            printf("%u/%u - %u/%u (by %u/%u)",
                ival.stepwise.min.numerator,
                ival.stepwise.min.denominator,
                ival.stepwise.max.numerator,
                ival.stepwise.max.denominator,
                ival.stepwise.step.numerator,
                ival.stepwise.step.denominator);
            return;

        default:
            break;
        }
    }
}
static void video_enum_frame_sizes(int fd, __u32 pixelformat)
{
    struct v4l2_frmsizeenum frame;
    unsigned int i;
    int ret;

    for (i = 0; ; ++i) {
        memset(&frame, 0, sizeof frame);
        frame.index = i;
        frame.pixel_format = pixelformat;
        ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frame);
        if (ret < 0)
            break;

        if (i != frame.index)
            printf("Warning: driver returned wrong frame index "
                "%u.\n", frame.index);
        if (pixelformat != frame.pixel_format)
            printf("Warning: driver returned wrong frame pixel "
                "format %08x.\n", frame.pixel_format);

        switch (frame.type) {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
            printf("\tFrame size: %ux%u (", frame.discrete.width,
                frame.discrete.height);
            video_enum_frame_intervals(fd, frame.pixel_format,
                frame.discrete.width, frame.discrete.height);
            printf(")\n");
            break;

        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
            printf("\tFrame size: %ux%u - %ux%u (",
                frame.stepwise.min_width,
                frame.stepwise.min_height,
                frame.stepwise.max_width,
                frame.stepwise.max_height);
            video_enum_frame_intervals(fd, frame.pixel_format,
                frame.stepwise.max_width,
                frame.stepwise.max_height);
            printf(")\n");
            break;

        case V4L2_FRMSIZE_TYPE_STEPWISE:
            printf("\tFrame size: %ux%u - %ux%u (by %ux%u) (\n",
                frame.stepwise.min_width,
                frame.stepwise.min_height,
                frame.stepwise.max_width,
                frame.stepwise.max_height,
                frame.stepwise.step_width,
                frame.stepwise.step_height);
            video_enum_frame_intervals(fd, frame.pixel_format,
                frame.stepwise.max_width,
                frame.stepwise.max_height);
            printf(")\n");
            break;

        default:
            break;
        }
    }
}
static int video_querycap(int fd)
{
    struct v4l2_capability cap;
    unsigned int caps;
    bool has_video;
    bool has_meta;
    bool has_capture;
    bool has_output;
    bool has_mplane;
    int ret;

    memset(&cap, 0, sizeof cap);
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0)
        return 0;

    caps = cap.capabilities & V4L2_CAP_DEVICE_CAPS
         ? cap.device_caps : cap.capabilities;

    has_video = caps & (V4L2_CAP_VIDEO_CAPTURE_MPLANE |
                V4L2_CAP_VIDEO_CAPTURE |
                V4L2_CAP_VIDEO_OUTPUT_MPLANE |
                V4L2_CAP_VIDEO_OUTPUT);
    has_meta = caps & (V4L2_CAP_META_CAPTURE );
    has_capture = caps & (V4L2_CAP_VIDEO_CAPTURE_MPLANE |
                  V4L2_CAP_VIDEO_CAPTURE |
                  V4L2_CAP_META_CAPTURE);
    has_output = caps & (V4L2_CAP_VIDEO_OUTPUT_MPLANE |
                 V4L2_CAP_VIDEO_OUTPUT );
    has_mplane = caps & (V4L2_CAP_VIDEO_CAPTURE_MPLANE |
                 V4L2_CAP_VIDEO_OUTPUT_MPLANE);

    printf("Device `%s' on `%s' (driver '%s') supports%s%s%s%s %s mplanes.\n",
        cap.card, cap.bus_info, cap.driver,
        has_video ? " video," : "",
        has_meta ? " meta-data," : "",
        has_capture ? " capture," : "",
        has_output ? " output," : "",
        has_mplane ? "with" : "without");

    return 0;
}
static void video_enum_formats_onece(int fd, enum v4l2_buf_type type)
{
    struct v4l2_fmtdesc fmt;
    unsigned int i;
    int ret;

    for (i = 0; ; ++i) {
        memset(&fmt, 0, sizeof fmt);
        fmt.index = i;
        fmt.type = type;
        ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt);
        if (ret < 0)
            break;

        if (i != fmt.index)
            printf("Warning: driver returned wrong format index "
                "%u.\n", fmt.index);
        if (type != fmt.type)
            printf("Warning: driver returned wrong format type "
                "%u.\n", fmt.type);

        printf("\tFormat %u: %s (%08x)\n", i,
            v4l2_format_name(fmt.pixelformat), fmt.pixelformat);
        printf("\tType: %s (%u)\n", v4l2_buf_type_name((v4l2_buf_type)fmt.type),
            fmt.type);
        printf("\tName: %.32s\n", fmt.description);
        video_enum_frame_sizes(fd, fmt.pixelformat);
        printf("\n");
    }
}

void video_enum_formats(char *video_name){
    int fd = open(video_name, O_RDWR, 0);
    video_querycap(fd);
    printf("- Available formats:\n");
    video_enum_formats_onece(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE);
    video_enum_formats_onece(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    video_enum_formats_onece(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
    video_enum_formats_onece(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    video_enum_formats_onece(fd, V4L2_BUF_TYPE_VIDEO_OVERLAY);
    video_enum_formats_onece(fd, V4L2_BUF_TYPE_META_CAPTURE);
//    video_enum_formats_onece(fd, V4L2_BUF_TYPE_META_OUTPUT);
    close(fd);
}
bool Capture::openCamera(uint8_t devId)
{
    mDevId = devId;
    mDevName = std::string("/dev/video") + std::to_string(mDevId);
    mFileDesc = open(mDevName.c_str(), O_RDWR, 0);


    if (mFileDesc == -1)
    {
        return false;
    }

    if (checkDevice() == false)
    {
        fprintf(stderr,"device erro");
        fflush(stdout);
        return false;
    }

//gwj::uppower first capture must set format, if not capture will erro,format will be 1920x0;
    if (setFormat() == -1)
    {
        fprintf(stderr,"if format erro ,maybe video format set erro\r\n");
        fprintf(stderr,"if device busy,maybe other using this video at now\r\n");
        return false;
    }
    if (checkResolution() == false)
    {
        fprintf(stderr,"video checkResolution erro");
        return false;
    }
    if((V4L2_PIX_FMT_NV12-format)==0){
        mChannels = 3;
        mSplit = 2;
    }
    else if((V4L2_PIX_FMT_UYVY-format==0)||(V4L2_PIX_FMT_VYUY-format==0)||(V4L2_PIX_FMT_YUYV-format==0)||(V4L2_PIX_FMT_YVYU-format==0)){
        mChannels = 2;
        mSplit = 1;
    }
    else{
        mChannels = 3;
        mSplit = 1;
    }

    mLastFrame.width = mWidth;
    mLastFrame.height = mHeight;
    mLastFrame.channels = mChannels;
    mLastFrame.split = mSplit;
    int bufSize = mLastFrame.width * mLastFrame.height * mLastFrame.channels / mLastFrame.split;
//    mLastFrame.data = new unsigned char[bufSize];

    if (prepareBuffers() == false)
    {
        fprintf(stderr,"prepareBuffers erro");
        return false;
    }
    return true;
}

int Capture::setFormat()
{
//gwj set format
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = capture_type;
    fmt.fmt.pix_mp.pixelformat = format;
    if (mWidth * mHeight != 0)
    {
        fmt.fmt.pix_mp.width = mWidth;
        fmt.fmt.pix_mp.height = mHeight;
    }
    int ret = xioctl(mFileDesc, VIDIOC_S_FMT, &fmt);
    return ret;
}

bool Capture::prepareBuffers()
{
#if MEMORY_TYPE == 1
    //gwj:: request buffer,buffer number is important
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(v4l2_requestbuffers));
    req.count = mBufCount;
    req.type = capture_type;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(mFileDesc, VIDIOC_REQBUFS, &req) == -1)
    {
        return false;
    }
    mBufCount = req.count;
    mBuffers = (Buffer *)calloc(req.count, sizeof(*mBuffers));
    if (mBuffers == nullptr)
    {
        return false;
    }
//gwj : map each buffer address form kernel to usersapce
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(v4l2_buffer));
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    memset(planes, 0, sizeof(planes));
    buf.type = capture_type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;
    for (int i = 0; i < req.count; i++)
    {
        buf.index = i;
        if (xioctl(mFileDesc, VIDIOC_QUERYBUF, &buf) == -1)
        {
            return false;
        }
//        std::cout <<"index i:::"<<i<<std::endl;
        unsigned int length;
        unsigned int offset;
        if(capture_type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            length = buf.m.planes[0].length;
            offset = buf.m.planes[0].m.mem_offset;
        }
        else{
            length = buf.length;
            offset = buf.m.offset;
        }
        mBuffers[i].length = length;
        mBuffers[i].start = (uint8_t *)mmap(nullptr, length, PROT_READ, MAP_SHARED, mFileDesc, offset);
        if (mBuffers[i].start == MAP_FAILED)
        {
            return false;
        }
    }

    //gwj: queue capture buffer
    memset(&buf, 0, sizeof(v4l2_buffer));
    buf.type = capture_type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;
    for (int i = 0; i < mBufCount; ++i)
    {
        buf.index = i;
        if (xioctl(mFileDesc, VIDIOC_QBUF, &buf) == -1)
        {
            perror("VIDIOC_QBUF::erro");
//            return false;
        }
    }

#elif MEMORY_TYPE == 2

    //gwj:: request buffer,buffer number is important
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(v4l2_requestbuffers));
    req.count = mBufCount;
    req.type = capture_type;
    req.memory = MEMORY_TYPE;
    if (xioctl(mFileDesc, VIDIOC_REQBUFS, &req) == -1)
    {
        printf("VIDIOC_REQBUFS erro \r\n");
        fflush(stdout);
        return false;
    }
    mBufCount = req.count;
    mBuffers = (Buffer *)calloc(req.count, sizeof(*mBuffers));
    if (mBuffers == nullptr)
    {
        return false;
    }
//gwj : map each buffer address form kernel to usersapce
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(v4l2_buffer));
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    memset(planes, 0, sizeof(planes));
    buf.type = capture_type;
    buf.memory = MEMORY_TYPE;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;
    unsigned int length;
    unsigned int offset;
    for (int i = 0; i < req.count; i++)
    {
        buf.index = i;
        if (xioctl(mFileDesc, VIDIOC_QUERYBUF, &buf) == -1)
        {
            return false;
        }
        if(capture_type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            length = buf.m.planes[0].length;
            offset = buf.m.planes[0].m.userptr;
        }
        else{
            length = buf.length;
            offset = buf.m.userptr;
        }
        mBuffers[i].length = length;
//        printf("buf length %d  offset:%d \r\n",length,offset);
        fflush(stdout);
    }

    //gwj: queue capture buffer
    memset(&buf, 0, sizeof(v4l2_buffer));
    memset(planes, 0, sizeof(planes));
    buf.type = capture_type;
    buf.memory = MEMORY_TYPE;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;
    for (int i = 0; i < mBufCount; ++i)
    {

        mBuffers[i].length = length;
        mBuffers[i].start = m_allocator.aquire_buf(length);

        buf.index = i;
        if (capture_type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buf.m.planes = planes;
            for (size_t plane = 0; plane < 1; ++plane) {
                buf.m.planes[plane].length = mBuffers[i].length ;
                buf.m.planes[plane].m.userptr = (unsigned long)mBuffers[i].start;
//                printf("%d mem[plane].length %d ,mem[plane].start :%x \r\n",plane, mBuffers[i].length,mBuffers[i].start);
            }
        } else {
            buf.m.userptr = reinterpret_cast<unsigned long>(mBuffers[i].start);
        }
        if (xioctl(mFileDesc, VIDIOC_QBUF, &buf) == -1)
        {
            perror("VIDIOC_QBUF::erro");
//            return false;
        }
    }

//    while (true) {

//    }


#endif
    return true;
}

bool Capture::checkDevice()
{
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    if (stat(mDevName.c_str(), &st) == -1)
    {
        return false;
    }

    if (!S_ISCHR(st.st_mode))
    {
        return false;
    }

    return true;
}

bool Capture::checkResolution()
{
//gwj : get format set
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = capture_type;
    if (xioctl(mFileDesc, VIDIOC_G_FMT, &fmt) == -1)
    {
        std::cout <<"errosdgvs\r\n"<<std::endl;
        return false;
    }
    int width_tmp = fmt.fmt.pix_mp.width;
    int height_tmp = fmt.fmt.pix_mp.height;
//    std::cout <<"width"<<width_tmp<<"height"<<height_tmp<<std::endl;
    if (mWidth != width_tmp || mHeight != height_tmp)
    {

        std::cout <<"format erro"<<std::endl;
        return false;
    }
    return true;
}

int Capture::xioctl(int fd, uint64_t IOCTL_X, void *arg)
{
    int ret = 0;
    int tries = IOCTL_RETRY;
    do
    {
        ret = ioctl(fd, IOCTL_X, arg);
    } while (ret && tries-- && ((errno == EINTR) || (errno == EAGAIN) || (errno == ETIMEDOUT)));

    if (ret == -1)
    {
//        printf("xioctl:%d\r\n",IOCTL_X);
        perror("xioctl");
    }
    return ret;
}

