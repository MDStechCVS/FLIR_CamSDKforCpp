#pragma once

#include "global.h"
#include "CameraControl_rev.h"
#include "MDS_E-bus_SampleDlg.h"

// 최대 및 최소 스팟 값을 저장하기 위한 구조체
struct MDSMeasureMaxSpotValue
{
	int x; // x 좌표
	int y; // y 좌표
	int pointIdx; // 포인트 인덱스
	ushort tempValue; // 온도 값
};

struct MDSMeasureMinSpotValue
{
	int x; // x 좌표
	int y; // y 좌표
	int pointIdx; // 포인트 인덱스
	ushort tempValue; // 온도 값
};

class ImageProcessor
{
public:

	ImageProcessor(int nIndex, CameraControl_rev* pCameraControl);
	virtual ~ImageProcessor();// 소멸자

private:

	int m_nIndex;
	CameraControl_rev *m_CameraControl;

	std::mutex drawmtx;
	std::mutex filemtx;
	std::mutex videomtx;
	std::mutex writermtx;

	cv::VideoWriter videoWriter;

	// Box 영역 내의 최대, 최소 온도값
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
	bool m_PaletteInitialized = false; // 팔레트 설정 변수
	bool m_bYUVYFlag;  // YUVY 사용시 이미지 포멧 변경을 위한 변수
	bool m_bGrayFlag; // Gray Scale 이미지 포멧 변경을 위한 변수
	bool m_bRGBFlag; // RGB 실화상 이미지 포멧 변경을 위한 변수
	bool m_bColorFlag; // Color Scale 이미지 포멧 변경을 위한 변수
	bool m_b16BitFlag; // 8/16Bit 아미지 포멧 변경을 위한 변수
	bool m_isRecording; // 동영상 녹화 중 상태변수
	bool m_bStartRecording; // 동영상 녹화 중 상태변수
	bool m_bMouseImageSave; // 마우스 우클릭으로 이미지 저장 변수

	cv::Vec3b m_Markcolor;
	cv::Vec3b m_findClosestColor;

	BITMAPINFO* m_BitmapInfo;
	PaletteTypes m_colormapType; // 컬러맵 변수
public:
	Mat OriMat; // 이미지 버퍼 사이즈를 저장하기위한 변수
	Mat m_TempData; // 온도데이터를 저장하기위한 변수
	Mat m_videoMat; // 녹화 이미지 복사를 위한 변수
	cv::Rect m_Select_rect; // 선택한 영역의 사각형
	MDSMeasureMaxSpotValue m_MaxSpot; // 최대 스팟 값
	MDSMeasureMinSpotValue m_MinSpot; // 최소 스팟 값

    ///////////////////////////////////////////////////////////////////////////

private:
	template <typename T>
	cv::Mat NormalizeAndProcessImage(const T* data, int height, int width, int cvType); // 이미지 정규화
	cv::Mat MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette); // 파레트 설정
	cv::Mat ConvertUYVYToBGR8(const cv::Mat& input);
	cv::Mat Convert16UC1ToBGR8(const cv::Mat& input);
	void ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx);
	void Initvariable_ImageParams();
	void SetPixelFormatParametertoGUI(); // PixelFormat 파라미터 설정 

	std::unique_ptr<uint16_t[]> Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length);
	BITMAPINFO* CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channels); // 비트맵 생성 및 이미지 맵핑 함수
	bool InitializeTemperatureThresholds();
	std::vector<std::string> CreateRainbowPalette();
	std::vector<std::string> CreateRainbowPalette_1024();
	cv::Vec3b findBrightestColor(const std::vector<cv::Vec3b>& colors); // 파레트에서 가장 밝은 컬러 찾기
	cv::Vec3b findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor); // 파레트에서 지정된 색이랑 비슷한 색 찾기
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
	void DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo); // 이미지 그리기
	/*-----------------------------------------*/

private:

	//std::vector<std::string> CreateIronPalette();
	double GetScaleFactor();
	bool WriteCSV(string strPath, Mat mData); // CSV 파일에 쓰기

public:
	void RenderDataSequence(PvImage* lImage, byte* imageDataPtr, int nIndex);
	CMDS_Ebus_SampleDlg* GetMainDialog();/*Main Dialog */
	cv::Mat ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg); // 이미지 분기
	void SetImageSize(cv::Mat& imageMat);
    void ProcessAndRecordFrame(cv::Mat & processedImageMat, int nWidth, int nHeight);
	void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels);
	void putTextOnCamModel(cv::Mat& imageMat);
	void CleanupAfterProcessing();
	std::unique_ptr<uint16_t[]> extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight);
	std::unique_ptr<uint16_t[]> extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight);
	void ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi); // 이미지 데이터를 처리하고 최소/최대값 업데이트
	cv::Mat DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex);
	bool IsRoiValid(const cv::Rect& roi);
	cv::Size GetImageSize(); // 카메라 이미지 크기 얻기

	int UpdateHeightForA50Camera(int& nHeight, int nWidth);
	
	void SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata);
	bool StartRecording(int frameWidth, int frameHeight, double frameRate);
	void StopRecording();
	void SetStartRecordingFlag(bool bFlag);
	bool GetStartRecordingFlag();
	void ProcessAndRecordFrame(const Mat& processedImageMat, int nWidth, int nHeight);
	void LoadCaminiFile(int nIndex); // 카메라 파라미터 설정파일 

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

