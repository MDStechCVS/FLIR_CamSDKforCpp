#pragma once


#include "global.h"
#include "CameraManager.h"


class CameraManager; // CameraManager 클래스의 선언을 위한 전방 선언, 전처리 하기 위함
class CMDS_Ebus_SampleDlg;
extern std::mutex drawmtx;

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
	ushort tempValue; // 임시 값
};



// 카메라 컨트롤 클래스인 CameraControl_rev
class CameraControl_rev : public CameraManager
{
public:
	
	CameraControl_rev(int nIndex); // 생성자
	~CameraControl_rev(); // 소멸자

private:

	CameraModelList m_Camlist; 
	ThreadStatus m_TStatus; // 스레드 상태


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

	double m_dCam_FPS;
	int m_nCamIndex;

	CameraManager* Manager; 
	bool m_bRunning; 
	bool m_bReStart; // 리커버리 시퀀스 변수
	bool m_PaletteInitialized = false; // 팔레트 설정 변수
	bool m_bYUVYFlag;  // YUVY 사용시 이미지 포멧 변경을 위한 변수
	bool m_bGrayFlag; // Gray Scale 이미지 포멧 변경을 위한 변수
	bool m_bColorFlag; // Color Scale 이미지 포멧 변경을 위한 변수
	bool m_b16BitFlag; // 8/16Bit 아미지 포멧 변경을 위한 변수
	bool m_isRecording; // 동영상 녹화 중 상태변수
	bool m_bStartRecording; // 동영상 녹화 중 상태변수
	cv::Vec3b m_Markcolor;
	cv::Vec3b m_findClosestColor;

	Mat OriMat; // 이미지 버퍼 사이즈를 저장하기위한 변수
	Mat m_TempData; // 온도데이터를 저장하기위한 변수
	Mat m_videoMat; // 녹화 이미지 복사를 위한 변수

	BITMAPINFO* m_BitmapInfo;

	PaletteTypes m_colormapType; // 컬러맵 변수

	int m_nCsvFileCount;
	int m_nSaveInterval;
	std::string m_strRawdataPath;
public:

	cv::Rect m_Select_rect; // 선택한 영역의 사각형
	PvDevice* m_Device;
	PvStream* m_Stream;
	PvPipeline* m_Pipeline;

	CWinThread* pThread[CAMERA_COUNT];
	Camera_Parameter* m_Cam_Params;

	bool m_bThreadFlag;
	bool m_bStart; // 카메라 시작 여부 플래그

	MDSMeasureMaxSpotValue m_MaxSpot; // 최대 스팟 값
	MDSMeasureMinSpotValue m_MinSpot; // 최소 스팟 값

	PvPixelType m_pixeltype;

	

private:

	void SetImageSize(Mat Img); // 이미지 크기 설정
	void SetCameraFPS(double dValue); // 카메라 FPS 설정
	DWORD ConvertHexValue(const CString& hexString); // CString to Hex
	
	/*Utill&Init*/
	void Initvariable();
	
	/*Camera Func*/
	PvDevice* ConnectToDevice(int nIndex); // PvDevice 연결 함수
	PvStream* OpenStream(int nIndex); // PvStream Open 함수
	void ConfigureStream(PvDevice* aDevice, PvStream* aStream, int nIndex); // Stream 파라미터 설정
	PvPipeline* CreatePipeline(PvDevice* aDevice, PvStream* aStream, int nIndex); // 파이프라인 생성자 함수
	int SetStreamingCameraParameters(PvGenParameterArray* lDeviceParams, int nIndex, CameraModelList Camlist); // 스트리밍 시 파라미터 설정하는 함수
	bool FindCameraModelName(int nCamIndex, CString strModel); // 카메라 모델 이름 찾기
	bool CameraParamSetting(int nIndex, PvDevice* aDevice); // 카메라 파라미터 설정
	 /*-----------------------------------------*/
	
	 /*Image Func*/
	template <typename T>
	cv::Mat NormalizeAndProcessImage(const T* data, int height, int width, int cvType); // 이미지 정규화
	cv::Mat ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg); // 이미지 분기
	cv::Mat MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette); // 파레트 설정
	cv::Mat ConvertUYVYToBGR8(const cv::Mat& input);
	cv::Mat Convert16UC1ToBGR8(const cv::Mat& input);
	cv::Mat ConvertToCV_8UC4(const cv::Mat& input);
	PvImage* GetImageFromBuffer(PvBuffer* aBuffer);
	std::unique_ptr<uint16_t[]> extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight);
	std::unique_ptr<uint16_t[]> extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight);
	uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
	void ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi); // 이미지 데이터를 처리하고 최소/최대값 업데이트
	double GetScaleFactor();
	bool InitializeTemperatureThresholds();
	void ProcessROIData(std::unique_ptr<uint16_t[]>& data, int size, int nROIWidth, int nROIHeight, int adjustedStartX, int adjustedStartY, double dScale);
	void ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx);
	void ImageProcessing(PvBuffer* aBuffer, int nIndex); // 이미지 후가공 시퀀스
	void DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo); // 이미지 그리기
	void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels);
	void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex);
	void SetupImagePixelType(PvImage* lImage);
	void putTextOnCamModel(cv::Mat& imageMat);
	int UpdateHeightForA50Camera(int& nHeight, int nWidth);
	cv::Mat DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex);
	void CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex);
	std::unique_ptr<uint16_t[]> Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length);
	bool WriteCSV(string filename, Mat m, char* strtime); // CSV 파일에 쓰기
	BITMAPINFO* CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channels); // 비트맵 생성 및 이미지 맵핑 함수
	unsigned char* GetImageDataPointer(PvImage* image);
	bool IsValidBuffer(PvBuffer* aBuffer);
	bool IsInvalidState(int nIndex, PvBuffer* buffer);
	bool IsRoiValid(const cv::Rect& roi);

	 /*-----------------------------------------*/
public:

	/*Thread Func*/
	static UINT ThreadCam(LPVOID _mothod); // 카메라 스레드
	bool DestroyThread(void); // 스레드 소멸

	/*Main Dialog */
	CMDS_Ebus_SampleDlg* GetMainDialog();
	std::chrono::system_clock::time_point lastCallTime;
	/*Camera Func*/
	void CameraManagerLink(CameraManager* _link); // CameraManager와 연결
	void CameraSequence(int nIndex); // 카메라 시퀀스
	bool CameraStart(int nIndex); // 카메라 시작
	bool CameraStop(int nIndex); // 카메라 정지
	void CameraDisconnect(); // 카메라 연결 해제
	double GetCameraFPS(); // 카메라 FPS 얻기
	bool AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex); // 파라미터 설정
	int ReStartSequence(int nIndex); // 다시 시작 시퀀스
	bool Camera_destroy(); // 카메라 종료시 동적 할당된 메모리 정리
	void LoadCaminiFile(int nIndex); // 카메라 파라미터 설정파일 
	void SetPixelFormatParameter(); // PixelFormat 파라미터 설정 

	/*Status*/
	void SetRunningFlag(bool bFlag); // 카메라 러닝 플래그 
	bool GetRunningFlag();
	void SetReStartFlag(bool bFlag); // 리커버리 시퀀스 플래그
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
	cv::Size GetImageSize(); // 카메라 이미지 크기 얻기
	cv::Vec3b findBrightestColor(const std::vector<cv::Vec3b>& colors); // 파레트에서 가장 밝은 컬러 찾기
	cv::Vec3b findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor); // 파레트에서 지정된 색이랑 비슷한 색 찾기
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