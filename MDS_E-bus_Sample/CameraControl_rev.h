#pragma once


#include "global.h"
#include "CameraManager.h"


class CameraManager; // CameraManager Ŭ������ ������ ���� ���� ����, ��ó�� �ϱ� ����
class CMDS_Ebus_SampleDlg;


// �ִ� �� �ּ� ���� ���� �����ϱ� ���� ����ü
struct MDSMeasureMaxSpotValue
{
	int x; // x ��ǥ
	int y; // y ��ǥ
	int pointIdx; // ����Ʈ �ε���
	ushort tempValue; // �µ� ��
};

struct MDSMeasureMinSpotValue
{
	int x; // x ��ǥ
	int y; // y ��ǥ
	int pointIdx; // ����Ʈ �ε���
	ushort tempValue; // �ӽ� ��
};

// ī�޶� ��Ʈ�� Ŭ������ CameraControl_rev
class CameraControl_rev : public CameraManager
{
public:
	
	CameraControl_rev(int nIndex); // ������
	~CameraControl_rev(); // �Ҹ���

private:

	CameraModelList m_Camlist; 
	ThreadStatus m_TStatus; // ������ ����

	double m_dCam_FPS;
	int m_nCamIndex;

	CameraManager* Manager; 
	bool m_bRunning; 
	bool m_bReStart; // ��Ŀ���� ������ ����
	bool m_PaletteInitialized = false; // �ȷ�Ʈ ���� ����

	Mat OriMat; // �̹��� ���� ����� �����ϱ����� ����
	BITMAPINFO* m_BitmapInfo;

	ColormapTypes m_colormapType; // �÷��� ����
public:

	cv::Rect m_Select_rect; // ������ ������ �簢��
	PvDevice* m_Device;
	PvStream* m_Stream;
	PvPipeline* m_Pipeline;

	CWinThread* pThread[CAMERA_COUNT];
	Camera_Parameter* m_Cam_Params;

	bool m_bThreadFlag;
	bool m_bStart; // ī�޶� ���� ���� �÷���

	MDSMeasureMaxSpotValue m_MaxSpot; // �ִ� ���� ��
	MDSMeasureMinSpotValue m_MinSpot; // �ּ� ���� ��

	PvPixelType m_pixeltype;

private:

	void SetImageSize(Mat Img); // �̹��� ũ�� ����
	void SetCameraFPS(double dValue); // ī�޶� FPS ����
	DWORD ConvertHexValue(const CString& hexString); // CString to Hex
	
	/*Utill&Init*/
	void Initvariable();
	

	/*Camera Func*/
	PvDevice* ConnectToDevice(int nIndex); // PvDevice ���� �Լ�
	PvStream* OpenStream(int nIndex); // PvStream Open �Լ�
	void ConfigureStream(PvDevice* aDevice, PvStream* aStream, int nIndex); // Stream �Ķ���� ����
	PvPipeline* CreatePipeline(PvDevice* aDevice, PvStream* aStream, int nIndex); // ���������� ������ �Լ�
	int SetStreamingCameraParameters(PvGenParameterArray* lDeviceParams, int nIndex, CameraModelList Camlist); // ��Ʈ���� �� �Ķ���� �����ϴ� �Լ�
	bool FindCameraModelName(int nCamIndex, CString strModel); // ī�޶� �� �̸� ã��
	bool CameraParamSetting(int nIndex, PvDevice* aDevice); // ī�޶� �Ķ���� ����
	 /*-----------------------------------------*/
	
	 /*Image Func*/
	template <typename T>
	cv::Mat NormalizeAndProcessImage(const T* data, int height, int width, int cvType); // �̹��� ����ȭ
	cv::Mat ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg); // �̹��� �б�
	cv::Mat MapColorsToPalette(const cv::Mat& inputImage, cv::ColormapTypes colormap); // �ķ�Ʈ ����
	PvImage* GetImageFromBuffer(PvBuffer* aBuffer);
	uint16_t* extractROI(const uint8_t* byteArr, int byteArrLength, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight); // ROI ������ ó��
	uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
	void ProcessImageData(const uint16_t* data, int size); // �̹��� �����͸� ó���ϰ� �ּ�/�ִ밪 ������Ʈ
	void ImageProcessing(PvBuffer* aBuffer, int nIndex); // �̹��� �İ��� ������
	void DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo); // �̹��� �׸���
	void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels);
	void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex);
	void SetupImagePixelType(PvImage* lImage);
	int UpdateHeightForA50Camera(int& nHeight, int nWidth);
	void DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex);
	void CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex, uint16_t* roiArr);
	void Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length);
	void WriteCSV(string filename, Mat m); // CSV ���Ͽ� ����
	BITMAPINFO* CreateBitmapInfo(int w, int h, int bpp, int nIndex); // ��Ʈ�� ���� �� �̹��� ���� �Լ�
	unsigned char* GetImageDataPointer(PvImage* image);
	bool IsValidBuffer(PvBuffer* aBuffer);
	bool IsInvalidState(int nIndex, PvBuffer* buffer);
	bool IsRoiValid(const cv::Rect& roi);

	 /*-----------------------------------------*/
public:

	/*Thread Func*/
	static UINT ThreadCam(LPVOID _mothod); // ī�޶� ������
	bool DestroyThread(void); // ������ �Ҹ�

	/*Main Dialog */
	CMDS_Ebus_SampleDlg* GetMainDialog();

	/*Camera Func*/
	void CameraManagerLink(CameraManager* _link); // CameraManager�� ����
	void CameraSequence(int nIndex); // ī�޶� ������
	bool CameraStart(int nIndex); // ī�޶� ����
	bool CameraStop(int nIndex); // ī�޶� ����
	void CameraDisconnect(); // ī�޶� ���� ����
	double GetCameraFPS(); // ī�޶� FPS ���
	bool AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex); // �Ķ���� ����
	int ReStartSequence(int nIndex); // �ٽ� ���� ������
	bool Camera_destroy(); // ī�޶� ����� ���� �Ҵ�� �޸� ����
	void LoadCaminiFile(int nIndex); // ī�޶� �Ķ���� �������� 

	/*Status*/
	void SetRunningFlag(bool bFlag); // ī�޶� ���� �÷��� 
	bool GetRunningFlag();
	void SetReStartFlag(bool bFlag); // ��Ŀ���� ������ �÷���
	bool GetReStartFlag();
	void SetThreadFlag(bool bFlag);
	bool GetThreadFlag();
	void SetStartFlag(bool bFlag);
	bool GetStartFlag();
	/*-----------------------------------------*/

	/*Image Func*/
	cv::Size GetImageSize(); // ī�޶� �̹��� ũ�� ���

	void SetColormapType(ColormapTypes type);
	ColormapTypes GetColormapType();
	/*-----------------------------------------*/
};