#pragma once

#include "global.h"
#include "CameraControl_rev.h"
#include "MDS_E-bus_SampleDlg.h"

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
	ushort tempValue; // �µ� ��
};

class ImageProcessor
{
public:

	ImageProcessor(int nIndex, CameraControl_rev* pCameraControl);
	virtual ~ImageProcessor();// �Ҹ���

private:

	int m_nIndex;
	CameraControl_rev *m_CameraControl;

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

protected:
	std::chrono::system_clock::time_point lastCallTime;
	std::string m_strRawdataPath;

	int m_nCsvFileCount;
	int m_nSaveInterval;
	bool m_PaletteInitialized = false; // �ȷ�Ʈ ���� ����
	bool m_bYUVYFlag;  // YUVY ���� �̹��� ���� ������ ���� ����
	bool m_bGrayFlag; // Gray Scale �̹��� ���� ������ ���� ����
	bool m_bRGBFlag; // RGB ��ȭ�� �̹��� ���� ������ ���� ����
	bool m_bColorFlag; // Color Scale �̹��� ���� ������ ���� ����
	bool m_b16BitFlag; // 8/16Bit �ƹ��� ���� ������ ���� ����
	bool m_isRecording; // ������ ��ȭ �� ���º���
	bool m_bStartRecording; // ������ ��ȭ �� ���º���
	bool m_bMouseImageSave; // ���콺 ��Ŭ������ �̹��� ���� ����

	cv::Vec3b m_Markcolor;
	cv::Vec3b m_findClosestColor;

	BITMAPINFO* m_BitmapInfo;
	PaletteTypes m_colormapType; // �÷��� ����
public:
	Mat OriMat; // �̹��� ���� ����� �����ϱ����� ����
	Mat m_TempData; // �µ������͸� �����ϱ����� ����
	Mat m_videoMat; // ��ȭ �̹��� ���縦 ���� ����
	cv::Rect m_Select_rect; // ������ ������ �簢��
	MDSMeasureMaxSpotValue m_MaxSpot; // �ִ� ���� ��
	MDSMeasureMinSpotValue m_MinSpot; // �ּ� ���� ��

    ///////////////////////////////////////////////////////////////////////////

private:
	template <typename T>
	cv::Mat NormalizeAndProcessImage(const T* data, int height, int width, int cvType); // �̹��� ����ȭ
	cv::Mat MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette); // �ķ�Ʈ ����
	cv::Mat ConvertUYVYToBGR8(const cv::Mat& input);
	cv::Mat Convert16UC1ToBGR8(const cv::Mat& input);
	void ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx);
	void Initvariable_ImageParams();
	void SetPixelFormatParametertoGUI(); // PixelFormat �Ķ���� ���� 

	std::unique_ptr<uint16_t[]> Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length);
	BITMAPINFO* CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channels); // ��Ʈ�� ���� �� �̹��� ���� �Լ�
	bool InitializeTemperatureThresholds();
	std::vector<std::string> CreateRainbowPalette();
	std::vector<std::string> CreateRainbowPalette_1024();
	cv::Vec3b findBrightestColor(const std::vector<cv::Vec3b>& colors); // �ķ�Ʈ���� ���� ���� �÷� ã��
	cv::Vec3b findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor); // �ķ�Ʈ���� ������ ���̶� ����� �� ã��
	Mat applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB);
	std::vector<cv::Vec3b> adjustPaletteScale(const std::vector<cv::Vec3b>& originalPalette, double scaleR, double scaleG, double scaleB);
	std::vector<cv::Vec3b> convertPaletteToBGR(const std::vector<std::string>& hexPalette);
	cv::Vec3b hexStringToBGR(const std::string& hexColor);
	std::vector<std::string> GetPalette(PaletteTypes paletteType);
	void RecordThreadFunction(double frameRate);
	void UpdateFrame(Mat newFrame);
	std::string GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension);
	bool SaveImageWithTimestamp(const cv::Mat& image);
	bool SaveRawDataWithTimestamp(const cv::Mat& rawData);

	void SetRawdataPath(std::string path);
	/*-----------------------------------------*/
	BITMAPINFO* ConvertImageTo32BitWithBitmapInfo(const cv::Mat& imageMat, int w, int h);
	void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex);
	void DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo); // �̹��� �׸���
	/*-----------------------------------------*/

private:

	//std::vector<std::string> CreateIronPalette();
	double GetScaleFactor();
	bool WriteCSV(string strPath, Mat mData); // CSV ���Ͽ� ����

public:
	void RenderDataSequence(PvImage* lImage, byte* imageDataPtr, int nIndex);
	CMDS_Ebus_SampleDlg* GetMainDialog();/*Main Dialog */
	cv::Mat ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg); // �̹��� �б�
	void SetImageSize(cv::Mat& imageMat);
    void ProcessAndRecordFrame(cv::Mat & processedImageMat, int nWidth, int nHeight);
	void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels);
	void putTextOnCamModel(cv::Mat& imageMat);
	void CleanupAfterProcessing();
	std::unique_ptr<uint16_t[]> extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight);
	std::unique_ptr<uint16_t[]> extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight);
	void ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi); // �̹��� �����͸� ó���ϰ� �ּ�/�ִ밪 ������Ʈ
	cv::Mat DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex);
	bool IsRoiValid(const cv::Rect& roi);
	cv::Size GetImageSize(); // ī�޶� �̹��� ũ�� ���

	int UpdateHeightForA50Camera(int& nHeight, int nWidth);
	
	void SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata);
	bool StartRecording(int frameWidth, int frameHeight, double frameRate);
	void StopRecording();
	void SetStartRecordingFlag(bool bFlag);
	bool GetStartRecordingFlag();
	void ProcessAndRecordFrame(const Mat& processedImageMat, int nWidth, int nHeight);
	void LoadCaminiFile(int nIndex); // ī�޶� �Ķ���� �������� 

	void SetYUVYType(bool bFlag);
	bool GetYUVYType();

	void SetGrayType(bool bFlag);
	bool GetGrayType();

	void Set16BitType(bool bFlag);
	bool Get16BitType();

	void SetColorPaletteType(bool bFlag);
	bool GetColorPaletteType();

	void SetRGBType(bool bFlag);
	bool GetRGBType();

	void SetPaletteType(PaletteTypes type);
	PaletteTypes GetPaletteType();

	std::string GetRawdataPath();

	std::string GetImageSavePath();
	std::string GetRawSavePath();
	std::string GetRecordingPath();

	void SetMouseImageSaveFlag(bool bFlag);
	bool GetMouseImageSaveFlag();
};

