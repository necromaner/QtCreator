#ifndef UUSBCAMERA_H_
#define UUSBCAMERA_H_
//------------------------------------------------------------------------//
#include <QString>
#include <linux/videodev2.h>
#include <linux/fb.h>
//------------------------------------------------------------------------//
typedef enum
{
	SUCCESS = 0,
	OPENFAIL,
	OVERSIZE,
	READFAIL,
	UNKNOWN
} CameraErrorCode;

typedef enum
{
	CAMERA_USB = 0,
	CAMERA_CMOS
} CameraType;

//------------------------------------------------------------------------//
extern "C" int write_JPEG_file(int w, int h, char * filename, unsigned char *image_buffer,
		int quality);
extern "C" int JPEGToRGB(unsigned char *src, unsigned char *dst, int width, int height,
		int len);
//int write_JPEG_file(int w, int h, char * filename, unsigned char *image_buffer,int quality)
//{
//    return 0;
//}

//int JPEGToRGB(unsigned char *src, unsigned char *dst, int width, int height,int len)
//{
//    return 0;
//}
//------------------------------------------------------------------------//
typedef unsigned char BYTE;
#define V4L2_CID_ROTATION		(V4L2_CID_PRIVATE_BASE + 0)
#define QUALITY 60
//------------------------------------------------------------------------//
class QUsbCamera
{
public:
	QUsbCamera(CameraType type, QString devname,int channel);
	virtual ~QUsbCamera();
    int MjpgSize;
    bool OpenDevice();
    bool GetBuffer(unsigned char *buffer);
    bool SaveImage(char *buf, char *name, int quality);
    //bool write_JPEG_file(unsigned char *image_buffer, char * filename,int quality);
	bool GetDeviceStatus();
	bool GetImage();
    void CloseDevice();
	void SetImageFormate(int formate);//0rgb,1yuyv
    void printVersion();
	bool getMaxSize(int &width,int &height);
	bool setRotation(int value);
    void SetImageSize(int iWidth, int iHeight);
    CameraErrorCode  getLastErrorCode();

    void InitCamera();
    int SetBrightness(int );
    int SetContrast(int);
    int SetSaturation (int);
    int SetHue(int);
    int SetSharpness(int);
    int SetGain(int);
    int SetGamma(int);
    int SetExposureAuto(bool);
    int SetExposureMenu(bool);
    int SetExposure(int);
    int SetWhiteBalanceAuto(bool);
    int SetWhiteBalanceMenu(bool);
    int SetWhiteBalance(int);
    int SetFocusingAuto(bool);
    int SetFocusingMenu(bool);
    int SetFocusing(int);
    int GetFocusing(int);
private:
	int m_iFd;
	int m_iWidth;
	int m_iHeight;
    int m_iChannel;
	int pixelformat;
	CameraType m_eType;
    CameraErrorCode m_eErrorCode;
    QString m_strDevName;
	bool m_bOk;
    int m_iQuality;
    QString m_strImageName;
    bool m_bDoCapture;
	bool init_device(void);
	void init_read(unsigned int buffer_size);
	bool init_mmap(void);
	void init_userp(unsigned int buffer_size);
	void uninit_device(void);
	bool start_capturing(void);
	void stop_capturing(void);
	void mainloop(unsigned char *image);
	int read_frame(unsigned char *image);
	void close_device(void);
	bool open_device(void);
	bool process_image(v4l2_buffer buf,unsigned char *image);
};
//------------------------------------------------------------------------//
extern bool g_bFdPreview;
//------------------------------------------------------------------------//
#endif /*UUSBCAMERA_H_*/
