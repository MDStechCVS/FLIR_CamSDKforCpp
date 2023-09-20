#pragma once


#include "global.h"
#include "CameraManager.h"


class CameraManager; // CameraManager 클래스의 선언을 위한 전방 선언, 전처리 하기 위함
class CMDS_Ebus_SampleDlg;


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

	double m_dCam_FPS;
	int m_nCamIndex;

	CameraManager* Manager; 
	bool m_bRunning; 
	bool m_bReStart; // 리커버리 시퀀스 변수
	bool m_PaletteInitialized = false; // 팔레트 설정 변수

	Mat OriMat; // 이미지 버퍼 사이즈를 저장하기위한 변수
	BITMAPINFO* m_BitmapInfo;

	ColormapTypes m_colormapType; // 컬러맵 변수
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
	cv::Mat MapColorsToPalette(const cv::Mat& inputImage, cv::ColormapTypes colormap); // 파레트 설정
	PvImage* GetImageFromBuffer(PvBuffer* aBuffer);
	uint16_t* extractROI(const uint8_t* byteArr, int byteArrLength, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight); // ROI 데이터 처리
	uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
	void ProcessImageData(const uint16_t* data, int size); // 이미지 데이터를 처리하고 최소/최대값 업데이트
	void ImageProcessing(PvBuffer* aBuffer, int nIndex); // 이미지 후가공 시퀀스
	void DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo); // 이미지 그리기
	void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels);
	void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex);
	void SetupImagePixelType(PvImage* lImage);
	int UpdateHeightForA50Camera(int& nHeight, int nWidth);
	void DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex);
	void CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex, uint16_t* roiArr);
	void Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length);
	void WriteCSV(string filename, Mat m); // CSV 파일에 쓰기
	BITMAPINFO* CreateBitmapInfo(int w, int h, int bpp, int nIndex); // 비트맵 생성 및 이미지 맵핑 함수
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

	/*Status*/
	void SetRunningFlag(bool bFlag); // 카메라 러닝 플래그 
	bool GetRunningFlag();
	void SetReStartFlag(bool bFlag); // 리커버리 시퀀스 플래그
	bool GetReStartFlag();
	void SetThreadFlag(bool bFlag);
	bool GetThreadFlag();
	void SetStartFlag(bool bFlag);
	bool GetStartFlag();
	/*-----------------------------------------*/

	/*Image Func*/
	cv::Size GetImageSize(); // 카메라 이미지 크기 얻기

	void SetColormapType(ColormapTypes type);
	ColormapTypes GetColormapType();
	/*-----------------------------------------*/
};