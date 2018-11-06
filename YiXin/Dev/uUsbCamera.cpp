//------------------------------------------------------------------------//
#include "uUsbCamera.h"
#include <QFile>
#include <iostream>
#include <assert.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
//#include <asm/types.h>            // for videodev2.h
#include <QColor>
//------------------------------------------------------------------------//
#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
	IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,
} io_method;

struct buffer {
	unsigned char * start;
	size_t length;
};
//------------------------------------------------------------------------//
int fb_fd;
char *fbmem = NULL;
unsigned int fb_length = 0;

typedef struct fb_var_screeninfo F_VINFO;

char dev_name[100];
static io_method io = IO_METHOD_MMAP;
struct buffer * buffers = NULL;
static unsigned int n_buffers = 0;
struct v4l2_format fmt;
using namespace std;

bool g_bFdPreview=false;//是否预览到fb
//------------------------------------------------------------------------//
static unsigned int fb_grab(int fd, char **fbmem)
{
	F_VINFO modeinfo;
	unsigned int length;

	if (ioctl(fd, FBIOGET_VSCREENINFO, &modeinfo) < 0)
	{
		perror("FBIOGET_VSCREENINFO");
		exit ( EXIT_FAILURE);
	}
	length = modeinfo.xres * modeinfo.yres * (modeinfo.bits_per_pixel >> 3);

    printf("fb memory info=xres (%d) x yres (%d), %d bpp\n", modeinfo.xres,modeinfo.yres, modeinfo.bits_per_pixel);
    *fbmem= (char *) mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
	if (*fbmem < 0)
	{
        printf("mmap()");
		length = 0;
	}

	return length;
}

static void errno_exit(const char * s) {
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	//exit ( EXIT_FAILURE);
}

#define CLEAR(x) memset (&(x), 0, sizeof (x))


static int xioctl(int fd, int request, void * arg)
{
	int r;
	do
		r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

using namespace std;

QUsbCamera::QUsbCamera(CameraType type, QString devname, int channel) {
	m_eType = type;
	m_strDevName = devname;
	m_iFd = -1;
	m_iChannel = channel;
    pixelformat = V4L2_PIX_FMT_YUYV;//wy-del
    //pixelformat = V4L2_PIX_FMT_MJPEG;//wy-add
	m_bDoCapture = false;
	m_bOk = false;
	memset(dev_name, 0x00, sizeof(dev_name));
	memcpy(dev_name, devname.toStdString().c_str(), devname.length());
}

QUsbCamera::~QUsbCamera() {
}
void QUsbCamera::SetImageSize(int iWidth, int iHeight) {
	m_iWidth = iWidth;
	m_iHeight = iHeight;
}
CameraErrorCode QUsbCamera::getLastErrorCode() {
    return m_eErrorCode;
}

void QUsbCamera::InitCamera()
{
    SetExposureMenu(true);//手动曝光
    sleep(1);
    SetWhiteBalanceMenu(true);//手动白平衡
    sleep(1);
    SetFocusingMenu(true);//手动对焦
    sleep(1);
    SetExposure(88);//设置曝光值
    SetWhiteBalance(4600);//设置白平衡值    
    SetFocusing(100);//设置设置焦点值
    SetBrightness(0);//亮度
    SetContrast(32);//对比度
    SetSaturation (64);//饱和度
    SetHue(0);//色调
    SetSharpness(3);//清晰度
    SetGain(0);//增益
    SetGamma(100);//伽玛
}


int QUsbCamera::SetBrightness(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_BRIGHTNESS;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetContrast(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_CONTRAST;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetSaturation(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_SATURATION;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetHue(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_HUE;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetSharpness(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_SHARPNESS;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetGain(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_GAIN;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetGamma(int value)
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_GAMMA;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetExposureAuto(bool value)//自动曝光
{
    struct v4l2_control ctrl;
    if(value)
    {
        ctrl.id    = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value     = V4L2_EXPOSURE_APERTURE_PRIORITY;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
    else
    {
        ctrl.id    = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value     = V4L2_EXPOSURE_MANUAL;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
}
int QUsbCamera::SetExposureMenu(bool value)//手动曝光
{
    struct v4l2_control ctrl;
    if(value)
    {
        ctrl.id    = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value     = V4L2_EXPOSURE_MANUAL;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
    else
    {
        ctrl.id    = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value     = V4L2_EXPOSURE_APERTURE_PRIORITY;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
}
int QUsbCamera::SetExposure(int value)//设置曝光值
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_EXPOSURE_ABSOLUTE;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetWhiteBalanceAuto(bool value)//自动白平衡
{
    struct v4l2_control ctrl;
    if(value)
    {
        ctrl.id    = V4L2_CID_AUTO_WHITE_BALANCE;
        ctrl.value     = 1;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
    else
    {
        ctrl.id    = V4L2_CID_AUTO_WHITE_BALANCE;
        ctrl.value     = 0;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
}
int QUsbCamera::SetWhiteBalanceMenu(bool value)//手动白平衡
{
    struct v4l2_control ctrl;
    if(value)
    {
        ctrl.id    = V4L2_CID_AUTO_WHITE_BALANCE;
        ctrl.value     = 0;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
    else
    {
        ctrl.id    = V4L2_CID_AUTO_WHITE_BALANCE;
        ctrl.value     = 1;
        return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    }
}
int QUsbCamera::SetWhiteBalance(int value)//设置白平衡值
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetFocusingAuto(bool value)//自动对焦
{
    struct v4l2_control ctrl;
    if(value)
    {
        ctrl.id    = V4L2_CID_FOCUS_AUTO;
        ctrl.value     = 1;
    }
    else
    {
        ctrl.id    = V4L2_CID_FOCUS_AUTO;
        ctrl.value     = 0;
    }
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetFocusingMenu(bool value)//手动对焦
{
    struct v4l2_control ctrl;
    if(value)
    {
        ctrl.id    = V4L2_CID_FOCUS_AUTO;
        ctrl.value     = 0;
    }
    else
    {
        ctrl.id    = V4L2_CID_FOCUS_AUTO;
        ctrl.value     = 1;
    }
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::SetFocusing(int value)//设置设置焦点值
{
    struct v4l2_control ctrl;
    ctrl.id    = V4L2_CID_FOCUS_ABSOLUTE;
    ctrl.value     = value;
    return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
}
int QUsbCamera::GetFocusing(int value)//得到当前焦点值
{
    value=0;
    struct v4l2_control ctrl;
    //手动对焦
    ctrl.id    = V4L2_CID_FOCUS_AUTO;
    ctrl.value     = 0;
    ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl);
    //得到当前焦点值
    return ioctl(m_iFd, VIDIOC_G_CTRL, &ctrl);
}

bool QUsbCamera::OpenDevice()//打开USB摄像头
{
    if (open_device())
    {
        //cout << "设备打开成功" << endl;
        if (init_device())
        {
			if (start_capturing())
            {
				return true;
            }
		}
    }
    else
    {
        //cout << "设备打开失败" << endl;
		return false;
    }
    return true;
}

void QUsbCamera::SetImageFormate(int formate)//0rgb,1yuyv
{
    formate=0;
    return ;
}
bool QUsbCamera::GetBuffer(unsigned char *image)
{
    //mainloop(image);
    read_frame(image);//wy-add
	return true;
}
bool QUsbCamera::SaveImage(char *image, char *name, int quality) {
    char *aa=name;
    aa++;
    int bb=quality;
    bb++;
	unsigned char image_buffer[m_iWidth * m_iHeight * 3];
	for (int y = 0; y < m_iHeight; y++)
		for (int x = 0; x < m_iWidth; x++) {
			image_buffer[(y * m_iWidth + x) * 3] = (((__u16 *) image)[y
					* m_iWidth + x] & 0xf800) >> 8;
			image_buffer[(y * m_iWidth + x) * 3 + 1]
					= (((__u16 *) image)[y * m_iWidth + x] & 0x07e0) >> 3;
			image_buffer[(y * m_iWidth + x) * 3 + 2]
					= (((__u16 *) image)[y * m_iWidth + x] & 0x001f) << 3;
		}
    /*//wy-del
	if (write_JPEG_file(m_iWidth, m_iHeight, name, image_buffer, quality) == 0)
	{
		//		printf("保存完成\n");
		return true;
	}
    */
    return true;//wy-add
    //return false;
}
bool QUsbCamera::GetDeviceStatus() {
	return m_iFd > 0 ? true : false;
}
void QUsbCamera::CloseDevice() {
	stop_capturing();
	uninit_device();
	close_device();
}
bool QUsbCamera::getMaxSize(int &width, int &height) {
	width = 1024;
	height = 768;
	return true;
}
bool QUsbCamera::open_device(void)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno,
				strerror(errno));
		return false;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		return false;
	}

	m_iFd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);

	if (-1 == m_iFd) {
		m_eErrorCode = OPENFAIL;
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
				strerror(errno));
		return false;
	}
	return true;
}
bool QUsbCamera::init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	unsigned int min;

    if (-1 == xioctl(m_iFd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			return false;
        } else
        {
			errno_exit("VIDIOC_QUERYCAP");
			return false;
		}
	}

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		//		exit ( EXIT_FAILURE);
		return false;
	}

    switch (io)
    {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n", dev_name);
			return false;
		}
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
			return false;
		}
		break;
	}

	v4l2_input input;
	memset(&input, 0, sizeof(struct v4l2_input));
	input.index = 0;
	int rtn = ioctl(m_iFd, VIDIOC_S_INPUT, &input);
    if (rtn < 0)
    {
		printf("VIDIOC_S_INPUT:rtn(%d)\n", rtn);
		return false;
	}


	/* Select video input, video standard and tune here. */CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(m_iFd, VIDIOC_CROPCAP, &cropcap))
    {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(m_iFd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	struct v4l2_fmtdesc fmtdesc; //获取摄像头
	memset(&fmtdesc, 0, sizeof(fmtdesc));
	struct v4l2_format fmt; //设置获取视频的格式
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //视频数据流类型，永远都是V4L2_BUF_TYPE_VIDEO_CAPTURE	
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;			//视频源的格式为JPEG或YUN4:2:2或RGB
    //fmt.fmt.pix.pixelformat = pixelformat;//wy-add
	fmt.fmt.pix.width = m_iWidth; //设置视频宽度
	fmt.fmt.pix.height = m_iHeight; //设置视频高度
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(m_iFd, VIDIOC_ENUM_FMT, &fmtdesc))
		return false;
	//printf("VIDIOC_ENUM_FMT(%s, VIDEO_CAPTURE)\n", fmtdesc.description);

	if (strcmp((const char*) fmtdesc.description, "RGB565") == 0) {
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;//V4L2_PIX_FMT_YUV420;//;
		pixelformat = V4L2_PIX_FMT_RGB565;
	} else if (strcmp((const char*) fmtdesc.description, "MJPEG") == 0) //视频源的格式为JPEG或YUN4:2:2或RGB
	{
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //视频源的格式为MJPEG
		pixelformat = V4L2_PIX_FMT_MJPEG;
	} else if (strcmp((const char*) fmtdesc.description, "JPEG") == 0) {
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //视频源的格式为MJPEG
		pixelformat = V4L2_PIX_FMT_MJPEG;
	} else if (strcmp((const char*) fmtdesc.description, "YUV 4:2:2 (YUYV)")
			== 0) {
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //视频源的格式为YUYV
		pixelformat = V4L2_PIX_FMT_YUYV;
	} else {
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420; //视频源的格式为JPEG或YUN4:2:2或RGB
		pixelformat = V4L2_PIX_FMT_YUV420;
	}
	if (ioctl(m_iFd, VIDIOC_S_FMT, &fmt) < 0) //使配置生效
	{
		printf("set format failed\n");
		return false;
	}

	setRotation(180);

	/*	CLEAR(fmt);

	 fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	 fmt.fmt.pix.width = 640;
	 fmt.fmt.pix.height = 480;
	 fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	 fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	 if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
	 errno_exit("VIDIOC_S_FMT");*/

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {
	case IO_METHOD_READ:
		init_read(fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap();
		break;

	case IO_METHOD_USERPTR:
		init_userp(fmt.fmt.pix.sizeimage);
		break;
	}

	if (g_bFdPreview)
	{
		fb_fd = open("/dev/fb0", O_RDWR);
		if (fb_fd < 0)
		{
			return false;
		}

		if ((fb_length = fb_grab(fb_fd, &fbmem)) == 0)
			cout << "fd mmap fail" << endl;

		memset(fbmem, 0, fb_length);
	}

	return true;
}
void QUsbCamera::init_read(unsigned int buffer_size) {
	buffers = (buffer *) calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit ( EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = (unsigned char *) malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit ( EXIT_FAILURE);
	}
}

bool QUsbCamera::init_mmap(void) {
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(m_iFd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"memory mapping\n", dev_name);
			return false;
		} else {
			errno_exit("VIDIOC_REQBUFS");
			return false;
		}
	}

	/*if (req.count < 2)
	 {
	 fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
	 return false;
	 }*/

	buffers = (buffer *) calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return false;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl(m_iFd, VIDIOC_QUERYBUF, &buf)) {
			errno_exit("VIDIOC_QUERYBUF");
			return false;
		}

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = (unsigned char *) mmap(
				NULL /* start anywhere */, buf.length,
				PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */, m_iFd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			return false;
	}
	return true;
}

void QUsbCamera::init_userp(unsigned int buffer_size) {
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(m_iFd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"user pointer i/o\n", dev_name);
			exit ( EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = (buffer *) calloc(4, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit ( EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = (unsigned char *) memalign(
		/* boundary */page_size, buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit ( EXIT_FAILURE);
		}
	}
}

int QUsbCamera::read_frame(unsigned char *image)
{
	struct v4l2_buffer buf;
	unsigned int i;
    switch (io)
    {
	case IO_METHOD_READ:
        if (-1 == read(m_iFd, buffers[0].start, buffers[0].length))
        {
            switch (errno)
            {
                case EAGAIN:
                    return 0;

                case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */

                default:
                    errno_exit("read");
			}
		}

		//process_image(buffers[0].start);

		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(m_iFd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < n_buffers);
		process_image(buf, image);
		if (-1 == xioctl(m_iFd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(m_iFd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start && buf.length
					== buffers[i].length)
				break;

		assert(i < n_buffers);

		//process_image((void *) buf.m.userptr);

		if (-1 == xioctl(m_iFd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;
	}

	return 1;
}

void QUsbCamera::mainloop(unsigned char *image)
{
    //cout << "QUsbCamera -mainloop ok1" << endl;
	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(m_iFd, &fds);

	//	Timeout.
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(m_iFd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r)
    {
		//if (EINTR == errno)
		//continue;

		errno_exit("select");
		//return false;
	}

    if (0 == r)
    {
		fprintf(stderr, "select timeout\n");
		exit ( EXIT_FAILURE);
	}
    //cout << "QUsbCamera -mainloop ok2" << endl;
    read_frame(image);
    //cout << "QUsbCamera -mainloop ok3" << endl;
}

void QUsbCamera::stop_capturing(void)
{
	enum v4l2_buf_type type;

    switch (io)
    {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(m_iFd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");

		break;
	}
}

bool QUsbCamera::start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

    switch (io)
    {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
        {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

            if (-1 == xioctl(m_iFd, VIDIOC_QBUF, &buf))
            {
				errno_exit("VIDIOC_QBUF");
				return false;
			}
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(m_iFd, VIDIOC_STREAMON, &type))
        {
			errno_exit("VIDIOC_STREAMON");
			return false;
		}
		break;

	case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
        {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) buffers[i].start;
			buf.length = buffers[i].length;

            if (-1 == xioctl(m_iFd, VIDIOC_QBUF, &buf))
            {
				errno_exit("VIDIOC_QBUF");
				return false;
			}
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(m_iFd, VIDIOC_STREAMON, &type))
        {
			errno_exit("VIDIOC_STREAMON");
			return false;
		}
		break;
	}
	return true;
}

void QUsbCamera::uninit_device(void)
{
	unsigned int i;

    switch (io)
    {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free(buffers[i].start);
		break;
	}

	free(buffers);
}
void QUsbCamera::close_device(void) {
	if (-1 == close(m_iFd))
		errno_exit("close");
	m_iFd = -1;
}


short redAdjust[] = {
-161,-160,-159,-158,-157,-156,-155,-153,
-152,-151,-150,-149,-148,-147,-145,-144,
-143,-142,-141,-140,-139,-137,-136,-135,
-134,-133,-132,-131,-129,-128,-127,-126,
-125,-124,-123,-122,-120,-119,-118,-117,
-116,-115,-114,-112,-111,-110,-109,-108,
-107,-106,-104,-103,-102,-101,-100, -99,
 -98, -96, -95, -94, -93, -92, -91, -90,
 -88, -87, -86, -85, -84, -83, -82, -80,
 -79, -78, -77, -76, -75, -74, -72, -71,
 -70, -69, -68, -67, -66, -65, -63, -62,
 -61, -60, -59, -58, -57, -55, -54, -53,
 -52, -51, -50, -49, -47, -46, -45, -44,
 -43, -42, -41, -39, -38, -37, -36, -35,
 -34, -33, -31, -30, -29, -28, -27, -26,
 -25, -23, -22, -21, -20, -19, -18, -17,
 -16, -14, -13, -12, -11, -10,  -9,  -8,
  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,
   2,   3,   4,   5,   6,   7,   9,  10,
  11,  12,  13,  14,  15,  17,  18,  19,
  20,  21,  22,  23,  25,  26,  27,  28,
  29,  30,  31,  33,  34,  35,  36,  37,
  38,  39,  40,  42,  43,  44,  45,  46,
  47,  48,  50,  51,  52,  53,  54,  55,
  56,  58,  59,  60,  61,  62,  63,  64,
  66,  67,  68,  69,  70,  71,  72,  74,
  75,  76,  77,  78,  79,  80,  82,  83,
  84,  85,  86,  87,  88,  90,  91,  92,
  93,  94,  95,  96,  97,  99, 100, 101,
 102, 103, 104, 105, 107, 108, 109, 110,
 111, 112, 113, 115, 116, 117, 118, 119,
 120, 121, 123, 124, 125, 126, 127, 128,
};

short greenAdjust1[] = {
  34,  34,  33,  33,  32,  32,  32,  31,
  31,  30,  30,  30,  29,  29,  28,  28,
  28,  27,  27,  27,  26,  26,  25,  25,
  25,  24,  24,  23,  23,  23,  22,  22,
  21,  21,  21,  20,  20,  19,  19,  19,
  18,  18,  17,  17,  17,  16,  16,  15,
  15,  15,  14,  14,  13,  13,  13,  12,
  12,  12,  11,  11,  10,  10,  10,   9,
   9,   8,   8,   8,   7,   7,   6,   6,
   6,   5,   5,   4,   4,   4,   3,   3,
   2,   2,   2,   1,   1,   0,   0,   0,
   0,   0,  -1,  -1,  -1,  -2,  -2,  -2,
  -3,  -3,  -4,  -4,  -4,  -5,  -5,  -6,
  -6,  -6,  -7,  -7,  -8,  -8,  -8,  -9,
  -9, -10, -10, -10, -11, -11, -12, -12,
 -12, -13, -13, -14, -14, -14, -15, -15,
 -16, -16, -16, -17, -17, -17, -18, -18,
 -19, -19, -19, -20, -20, -21, -21, -21,
 -22, -22, -23, -23, -23, -24, -24, -25,
 -25, -25, -26, -26, -27, -27, -27, -28,
 -28, -29, -29, -29, -30, -30, -30, -31,
 -31, -32, -32, -32, -33, -33, -34, -34,
 -34, -35, -35, -36, -36, -36, -37, -37,
 -38, -38, -38, -39, -39, -40, -40, -40,
 -41, -41, -42, -42, -42, -43, -43, -44,
 -44, -44, -45, -45, -45, -46, -46, -47,
 -47, -47, -48, -48, -49, -49, -49, -50,
 -50, -51, -51, -51, -52, -52, -53, -53,
 -53, -54, -54, -55, -55, -55, -56, -56,
 -57, -57, -57, -58, -58, -59, -59, -59,
 -60, -60, -60, -61, -61, -62, -62, -62,
 -63, -63, -64, -64, -64, -65, -65, -66,
};

short greenAdjust2[] = {
  74,  73,  73,  72,  71,  71,  70,  70,
  69,  69,  68,  67,  67,  66,  66,  65,
  65,  64,  63,  63,  62,  62,  61,  60,
  60,  59,  59,  58,  58,  57,  56,  56,
  55,  55,  54,  53,  53,  52,  52,  51,
  51,  50,  49,  49,  48,  48,  47,  47,
  46,  45,  45,  44,  44,  43,  42,  42,
  41,  41,  40,  40,  39,  38,  38,  37,
  37,  36,  35,  35,  34,  34,  33,  33,
  32,  31,  31,  30,  30,  29,  29,  28,
  27,  27,  26,  26,  25,  24,  24,  23,
  23,  22,  22,  21,  20,  20,  19,  19,
  18,  17,  17,  16,  16,  15,  15,  14,
  13,  13,  12,  12,  11,  11,  10,   9,
   9,   8,   8,   7,   6,   6,   5,   5,
   4,   4,   3,   2,   2,   1,   1,   0,
   0,   0,  -1,  -1,  -2,  -2,  -3,  -4,
  -4,  -5,  -5,  -6,  -6,  -7,  -8,  -8,
  -9,  -9, -10, -11, -11, -12, -12, -13,
 -13, -14, -15, -15, -16, -16, -17, -17,
 -18, -19, -19, -20, -20, -21, -22, -22,
 -23, -23, -24, -24, -25, -26, -26, -27,
 -27, -28, -29, -29, -30, -30, -31, -31,
 -32, -33, -33, -34, -34, -35, -35, -36,
 -37, -37, -38, -38, -39, -40, -40, -41,
 -41, -42, -42, -43, -44, -44, -45, -45,
 -46, -47, -47, -48, -48, -49, -49, -50,
 -51, -51, -52, -52, -53, -53, -54, -55,
 -55, -56, -56, -57, -58, -58, -59, -59,
 -60, -60, -61, -62, -62, -63, -63, -64,
 -65, -65, -66, -66, -67, -67, -68, -69,
 -69, -70, -70, -71, -71, -72, -73, -73,
};

short blueAdjust[] = {
-276,-274,-272,-270,-267,-265,-263,-261,
-259,-257,-255,-253,-251,-249,-247,-245,
-243,-241,-239,-237,-235,-233,-231,-229,
-227,-225,-223,-221,-219,-217,-215,-213,
-211,-209,-207,-204,-202,-200,-198,-196,
-194,-192,-190,-188,-186,-184,-182,-180,
-178,-176,-174,-172,-170,-168,-166,-164,
-162,-160,-158,-156,-154,-152,-150,-148,
-146,-144,-141,-139,-137,-135,-133,-131,
-129,-127,-125,-123,-121,-119,-117,-115,
-113,-111,-109,-107,-105,-103,-101, -99,
 -97, -95, -93, -91, -89, -87, -85, -83,
 -81, -78, -76, -74, -72, -70, -68, -66,
 -64, -62, -60, -58, -56, -54, -52, -50,
 -48, -46, -44, -42, -40, -38, -36, -34,
 -32, -30, -28, -26, -24, -22, -20, -18,
 -16, -13, -11,  -9,  -7,  -5,  -3,  -1,
   0,   2,   4,   6,   8,  10,  12,  14,
  16,  18,  20,  22,  24,  26,  28,  30,
  32,  34,  36,  38,  40,  42,  44,  46,
  49,  51,  53,  55,  57,  59,  61,  63,
  65,  67,  69,  71,  73,  75,  77,  79,
  81,  83,  85,  87,  89,  91,  93,  95,
  97,  99, 101, 103, 105, 107, 109, 112,
 114, 116, 118, 120, 122, 124, 126, 128,
 130, 132, 134, 136, 138, 140, 142, 144,
 146, 148, 150, 152, 154, 156, 158, 160,
 162, 164, 166, 168, 170, 172, 175, 177,
 179, 181, 183, 185, 187, 189, 191, 193,
 195, 197, 199, 201, 203, 205, 207, 209,
 211, 213, 215, 217, 219, 221, 223, 225,
 227, 229, 231, 233, 235, 238, 240, 242,
};
//判断范围
unsigned char clip(int val)
{
    if(val > 255)
    {
        return 255;
    }
    else if(val > 0)
    {
        return val;
    }
    else
    {
        return 0;
    }
}
//查表法YUV TO RGB
int YUYVToRGB_table(unsigned char *yuv, unsigned char *rgb, unsigned int width,unsigned int height)
{
    short y1=0, y2=0, u=0, v=0;
    unsigned char *pYUV = yuv;
    unsigned char *pGRB = rgb;
    int i=0;
    //int y=0,x=0,in=0,y0,out=0;
    int count =width * height /2;//width * height *2/4;

   for(i = 0; i < count; i++)
    {
        y1 = *pYUV++ ;
        u  = *pYUV++ ;
        y2 = *pYUV++ ;
        v  = *pYUV++ ;

        *pGRB++ = clip(y1 + redAdjust[v]);
        *pGRB++ = clip(y1 + greenAdjust1[u] + greenAdjust2[v]);
        *pGRB++ = clip(y1 + blueAdjust[u]);
        *pGRB++ = clip(y2 + redAdjust[v]);
        *pGRB++ = clip(y2 + greenAdjust1[u] + greenAdjust2[v]);
        *pGRB++ = clip(y2 + blueAdjust[u]);
    }
  return 0;
}
bool QUsbCamera::process_image(v4l2_buffer buf, unsigned char *image)
{
    //cout << "QUsbCamera -process_image ok" << endl;
	unsigned char * ptcur = (unsigned char *) buffers[0].start; //开始霍夫曼解码

	int iImageSize = 0;
    if (pixelformat == V4L2_PIX_FMT_RGB565)
    {
		memcpy(image, ptcur, m_iWidth * m_iHeight * 2);
		return true;
    } else if (pixelformat == V4L2_PIX_FMT_YUYV)
    {
		//		YUYVToRGB(m_iWidth, m_iHeight, ptcur, image);
        //cout<<"==============="<<endl;
        //YUYVToRGB(ptcur, image, m_iWidth, m_iHeight);//WY-del
        //Pyuv422torgb24(ptcur, image, m_iWidth, m_iHeight);//WY-add
        YUYVToRGB_table(ptcur, image, m_iWidth, m_iHeight);//WY-add
        if (m_bDoCapture)
        {
            //write_JPEG_file(m_iWidth, m_iHeight,(char *) m_strImageName.toStdString().c_str(), image,m_iQuality);//wy-del
		}
		return true;
    } else if (pixelformat == V4L2_PIX_FMT_JPEG || pixelformat== V4L2_PIX_FMT_MJPEG)
    {
		//解码成rgb
		unsigned int k;
        for (k = 0; k < buf.bytesused; k++)
        {
			if ((buffers[0].start[k] == 0x000000FF) && (buffers[0].start[k + 1]
					== 0x000000C4)) {
				break;
			}
		}
        if (k == buf.bytesused)
        {
			printf("huffman table don't exist! \n");
			return false;
		}
		unsigned int i;
        for (i = 0; i < buf.bytesused; i++)
        {
            if ((buffers[0].start[i] == 0x000000FF) && (buffers[0].start[i + 1]== 0x000000D8))
				break;
			ptcur++;
		}
		iImageSize = buf.bytesused - i;
        //----------------------//wy-add
        //JPEGToRGB(ptcur, image, m_iWidth, m_iHeight, iImageSize);
        memcpy(image, ptcur, iImageSize);
        MjpgSize=iImageSize;
        //----------------------
	}
	return true;
}
void QUsbCamera::printVersion()//版本信息
{
        cout << "，版本WY-V1.0" << endl;
}
bool QUsbCamera::setRotation(int value) {
	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_ROTATION;
	ctrl.value = value;
	return ioctl(m_iFd, VIDIOC_S_CTRL, &ctrl) == 0 ? true : false;
}
