#pragma once


#include "global.h"
#include "CameraManager.h"


class CameraManager; // CameraManager Ŭ������ ������ ���� ���� ����, ��ó�� �ϱ� ����
class CMDS_Ebus_SampleDlg;
extern std::mutex drawmtx;

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


	std::mutex drawmtx;
	std::mutex filemtx;
	std::mutex videomtx;
	std::mutex writermtx;

	cv::VideoWriter videoWriter;
	// Box ���� ���� �ִ�, �ּ� �µ���
	ushort m_Max = 0;
	ushort m_Min = 65535;

	int m_Max_X;
	int m_Max_Y;
	int m_Min_X;
	int m_Min_Y;

	double m_dCam_FPS;
	int m_nCamIndex;

	CameraManager* Manager; 
	bool m_bRunning; 
	bool m_bReStart; // ��Ŀ���� ������ ����
	bool m_PaletteInitialized = false; // �ȷ�Ʈ ���� ����
	bool m_bYUVYFlag;  // YUVY ���� �̹��� ���� ������ ���� ����
	bool m_bGrayFlag; // Gray Scale �̹��� ���� ������ ���� ����
	bool m_bColorFlag; // Color Scale �̹��� ���� ������ ���� ����
	bool m_b16BitFlag; // 8/16Bit �ƹ��� ���� ������ ���� ����
	bool m_isRecording; // ������ ��ȭ �� ���º���
	bool m_bStartRecording; // ������ ��ȭ �� ���º���
	cv::Vec3b m_Markcolor;
	cv::Vec3b m_findClosestColor;

	Mat OriMat; // �̹��� ���� ����� �����ϱ����� ����
	Mat m_TempData; // �µ������͸� �����ϱ����� ����
	Mat m_videoMat; // ��ȭ �̹��� ���縦 ���� ����

	BITMAPINFO* m_BitmapInfo;

	PaletteTypes m_colormapType; // �÷��� ����

	int m_nCsvFileCount;
	int m_nSaveInterval;
	std::string m_strRawdataPath;
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
	cv::Mat MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette); // �ķ�Ʈ ����
	cv::Mat ConvertUYVYToBGR8(const cv::Mat& input);
	cv::Mat Convert16UC1ToBGR8(const cv::Mat& input);
	cv::Mat ConvertToCV_8UC4(const cv::Mat& input);
	PvImage* GetImageFromBuffer(PvBuffer* aBuffer);
	std::unique_ptr<uint16_t[]> extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight);
	std::unique_ptr<uint16_t[]> extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight);
	uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
	void ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi); // �̹��� �����͸� ó���ϰ� �ּ�/�ִ밪 ������Ʈ
	double GetScaleFactor();
	bool InitializeTemperatureThresholds();
	void ProcessROIData(std::unique_ptr<uint16_t[]>& data, int size, int nROIWidth, int nROIHeight, int adjustedStartX, int adjustedStartY, double dScale);
	void ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx);
	void ImageProcessing(PvBuffer* aBuffer, int nIndex); // �̹��� �İ��� ������
	void DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo); // �̹��� �׸���
	void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels);
	void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex);
	void SetupImagePixelType(PvImage* lImage);
	void putTextOnCamModel(cv::Mat& imageMat);
	int UpdateHeightForA50Camera(int& nHeight, int nWidth);
	cv::Mat DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex);
	void CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex);
	std::unique_ptr<uint16_t[]> Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length);
	bool WriteCSV(string filename, Mat m, char* strtime); // CSV ���Ͽ� ����
	BITMAPINFO* CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channels); // ��Ʈ�� ���� �� �̹��� ���� �Լ�
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
	std::chrono::system_clock::time_point lastCallTime;
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
	void SetPixelFormatParameter(); // PixelFormat �Ķ���� ���� 

	/*Status*/
	void SetRunningFlag(bool bFlag); // ī�޶� ���� �÷��� 
	bool GetRunningFlag();
	void SetReStartFlag(bool bFlag); // ��Ŀ���� ������ �÷���
	bool GetReStartFlag();
	void SetThreadFlag(bool bFlag);
	bool GetThreadFlag();
	void SetStartFlag(bool bFlag);
	bool GetStartFlag();
	void SetYUVYType(bool bFlag);
	bool GetYUVYType();

	void SetGrayType(bool bFlag);
	bool GetGrayType();

	void Set16BitType(bool bFlag);
	bool Get16BitType();

	void SetColorPaletteType(bool bFlag);
	bool GetColorPaletteType();

	void SetPaletteType(PaletteTypes type);
	PaletteTypes GetPaletteType();

	 void SetRawdataPath(std::string path);
	 std::string GetRawdataPath();

	 std::string GetImageSavePath();
	 std::string GetRawSavePath();
	 std::string GetRecordingPath();

	bool bbtnDisconnectFlag;
	/*-----------------------------------------*/

	/*Image Func*/
	cv::Size GetImageSize(); // ī�޶� �̹��� ũ�� ���
	cv::Vec3b findBrightestColor(const std::vector<cv::Vec3b>& colors); // �ķ�Ʈ���� ���� ���� �÷� ã��
	cv::Vec3b findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor); // �ķ�Ʈ���� ������ ���̶� ����� �� ã��
	/*-----------------------------------------*/
	BITMAPINFO* ConvertImageTo32BitWithBitmapInfo(const cv::Mat& imageMat, int w, int h);
	cv::Mat temperaturePalette(const cv::Mat& temperatureData);
	
	Mat applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB);
	std::vector<cv::Vec3b> adjustPaletteScale(const std::vector<cv::Vec3b>& originalPalette, double scaleR, double scaleG, double scaleB);
	std::vector<cv::Vec3b> convertPaletteToBGR(const std::vector<std::string>& hexPalette);
	cv::Vec3b hexStringToBGR(const std::string& hexColor);
	std::vector<std::string> CreateRainbowPalette();
	std::vector<std::string> GetPalette(PaletteTypes paletteType);
	void SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata);
	bool CreateDirectoryRecursively(const std::string& path);
	bool StartRecording(int frameWidth, int frameHeight, double frameRate);
	void StopRecording();
	void SetStartRecordingFlag(bool bFlag);
	bool GetStartRecordingFlag();
	void RecordThreadFunction(double frameRate);
	void UpdateFrame(Mat newFrame);
	void ProcessAndRecordFrame(const Mat &processedImageMat, int nWidth, int nHeight);
};