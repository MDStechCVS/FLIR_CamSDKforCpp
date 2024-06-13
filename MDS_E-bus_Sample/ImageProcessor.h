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
	bool updated;

	MDSMeasureMaxSpotValue() : x(0), y(0), pointIdx(0), tempValue(std::numeric_limits<ushort>::min()), updated(false) {}
};

struct MDSMeasureMinSpotValue
{
	int x; // x 좌표
	int y; // y 좌표
	int pointIdx; // 포인트 인덱스
	ushort tempValue; // 온도 값
	bool updated;

	MDSMeasureMinSpotValue() : x(0), y(0), pointIdx(0), tempValue(std::numeric_limits<ushort>::max()), updated(false) {}
};

// ROI (관심 영역) 유형을 정의하는 열거형
enum class ShapeType
{
	None = -1,
	Rectangle = 0,
	Circle,
	Ellipse,
	Line
};

// 각 ROI의 결과를 저장하는 구조체
struct ROIResults
{
	cv::Rect roi; // ROI 영역
	int max_x, max_y, min_x, min_y; // 최대/최소 x, y 좌표
	ushort max_temp, min_temp; // 최대/최소 온도
	ShapeType shapeType; // 도형
	MDSMeasureMaxSpotValue maxSpot; // 최대 스팟 값
	MDSMeasureMinSpotValue minSpot; // 최소 스팟 값
	int span; // 스팬 값
	int level; // 레벨 값
	int nIndex; 
	bool needsRedraw; // 다시 그릴 필요 여부
	cv::Scalar color; 

	ROIResults() : max_x(0), max_y(0), min_x(0), min_y(0),
		max_temp(0), min_temp(65535),
		shapeType(ShapeType::None),
		span(0), level(0), nIndex(-1),
		needsRedraw(true),
		color(cv::Scalar(51, 255, 51)) {}
};

// ImageProcessor 클래스 정의
class ImageProcessor
{
public:
    // 생성자 및 소멸자
    ImageProcessor(int nIndex, CameraControl_rev* pCameraControl);
    virtual ~ImageProcessor();
    CMDS_Ebus_SampleDlg* GetMainDialog();/*Main Dialog */
    CameraControl_rev* GetCam(); // 카메라 제어 객체 반환
    
    

    // 경로 관련 메서드
    std::string GetRawdataPath(); // 원본 데이터 경로 반환
    std::string GetImageSavePath(); // 이미지 저장 경로 반환
    std::string GetRawSavePath(); // 원본 저장 경로 반환
    std::string GetRecordingPath(); // 녹화 경로 반환

    void RenderDataSequence(PvImage* lImage, PvBuffer* aBuffer, byte* imageDataPtr, int nIndex); // 데이터 시퀀스 렌더링
    cv::Mat ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg); // 설정에 따른 이미지 처리
    void ProcessAndRecordFrame(cv::Mat& processedImageMat, int nWidth, int nHeight); // 프레임 처리 및 녹화
    void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels, const ROIResults& roiResult); // ROI 사각형 그리기
    void putTextOnCamModel(cv::Mat& imageMat); // 이미지에 텍스트 추가
    void DrawMinMarkerAndText(cv::Mat& imageMat, const MDSMeasureMinSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize); // 최소 마커와 텍스트 그리기
    void DrawMaxMarkerAndText(cv::Mat& imageMat, const MDSMeasureMaxSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize); // 최대 마커와 텍스트 그리기
    void CleanupAfterProcessing(); // 처리 후 정리 작업
    bool StartRecording(int frameWidth, int frameHeight, double frameRate); // 녹화 시작
    void StopRecording(); // 녹화 중지
    void SetStartRecordingFlag(bool bFlag); // 녹화 시작 플래그 설정
    bool GetStartRecordingFlag(); // 녹화 시작 플래그 반환
    void LoadCaminiFile(int nIndex); // 카메라 파라미터 설정 파일 로드
    cv::Size GetImageSize(); // 이미지 크기 얻기
    bool GetRGBType(); // RGB 타입 반환
    bool Get16BitType(); // 16비트 타입 반환
    bool GetYUVYType(); // YUVY 타입 반환
    bool GetGrayType(); // 그레이 스케일 타입 반환
    bool GetColorPaletteType(); // 컬러 팔레트 타입 반환
    void ClearTemporaryROI(); // 임시 ROI 클리어
    cv::Rect mapRectToImage(const cv::Rect& rect, const cv::Size& imageSize, const cv::Size& dialogSize); // 사각형을 이미지에 매핑
    cv::Point mapPointToImage(const CPoint& point, const cv::Size& imageSize, const CRect& displayRect); // 포인트를 이미지에 매핑
    void StartCreatingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& startPos, cv::Point& endPos, bool& selectingROI); // ROI 생성 시작
    void UpdateCreatingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& startPos, cv::Point& endPos); // ROI 생성 업데이트
    void ConfirmCreatingROI(const cv::Point& startPos, const cv::Point& endPos, const CRect& rect, const cv::Size& imageSize, bool& selectingROI); // ROI 생성 확정
    void StartDraggingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& dragStartPos, int& selectedROIIndex, bool& isDraggingROI); // ROI 드래깅 시작
    void UpdateDraggingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& dragStartPos, int selectedROIIndex); // ROI 드래깅 업데이트
    void ConfirmDraggingROI(bool& isDraggingROI, int& selectedROIIndex); // ROI 드래깅 확정
    void deleteROI(int index); // ROI 삭제
    void deleteAllROIs(); // 모든 ROI 삭제
    void updateROI(const cv::Rect& roi, int nIndex); // ROI 업데이트
    bool canAddROI() const; // ROI 추가 가능 여부 반환
    void confirmROI(const cv::Rect& roi, ShapeType shapeType); // ROI 확정
    int findROIIndexAtPoint(const cv::Point& point); // 포인트에서 ROI 인덱스 찾기

    // 이미지 포맷 설정 메서드
    void SetYUVYType(bool bFlag); // YUVY 타입 설정
    void SetGrayType(bool bFlag); // 그레이 스케일 타입 설정
    void Set16BitType(bool bFlag); // 16비트 타입 설정
    void SetColorPaletteType(bool bFlag); // 컬러 팔레트 타입 설정
    void SetRGBType(bool bFlag); // RGB 타입 설정
    void SetPaletteType(PaletteTypes type); // 팔레트 타입 설정
    PaletteTypes GetPaletteType(); // 팔레트 타입 반환

    // 마우스 이미지 저장 플래그
    void SetMouseImageSaveFlag(bool bFlag); // 마우스 이미지 저장 플래그 설정
    bool GetMouseImageSaveFlag(); // 마우스 이미지 저장 플래그 반환

    void SaveRoisToIniFile(int nIndex); // INI 파일로 ROI 저장
    void LoadRoisFromIniFile(int nIndex); // INI 파일에서 ROI 로드

    void SetCurrentShapeType(ShapeType type); // 현재 형태 타입 설정
    ShapeType GetCurrentShapeType(); // 현재 형태 타입 반환

    std::mutex videomtx;
    cv::Mat m_TempData; // 온도 데이터
    cv::Mat m_videoMat; // 녹화 이미지 복사
    std::vector<cv::Rect> m_rois; // 선택한 영역의 사각형
    std::vector<std::shared_ptr<ROIResults>> m_roiResults; // 최종 ROI 결과
    std::vector<std::shared_ptr<ROIResults>> m_temporaryRois; // 임시 ROI 데이터
    std::shared_ptr<ROIResults> m_selectedROI; // 현재 선택된 ROI
    bool m_isAddingNewRoi = true; // 새 ROI 추가 상태 플래그
    bool m_ROIEnabled = true; // ROI 사용 가능 여부
    ShapeType currentShapeType; // 현재 ROI 형태

    // SEQ 관련 변수
    ULONG m_Level;
    ULONG m_Span;
    int m_iCurrentSEQImage;
    bool m_bSaveAsSEQ; // SEQ 파일 저장 여부
    CFile m_SEQFile;
    DWORD m_TrigCount;
    time_t m_ts;
    int m_ms;
    short m_tzBias;

private:

    int m_nIndex; // 카메라 인덱스
    CameraControl_rev* m_CameraControl; // 카메라 제어 객체
    
    PaletteManager paletteManager;
    std::mutex drawmtx; // 그리기 동기화
    std::mutex filemtx; // 파일 동기화
    std::mutex m_roiMutex; // ROI 동기화
    std::mutex m_temporaryRoiMutex; // 임시 ROI 데이터 보호용 뮤텍스
    std::mutex m_bitmapMutex; // 비트맵 동기화
    std::mutex roiDrawMtx; // ROI 그리기 동기화
    ROIResults* roiResult; // ROI 결과
    cv::VideoWriter videoWriter; 

    DWORD m_LineState1;
    DWORD m_LineState2;
    DWORD m_TrigCount1;
    DWORD m_TrigCount2;
    DWORD m_FrameCount;
    bool m_isSEQFileWriting; // SEQ 파일 작성 여부
    std::atomic<bool> isProcessingROIs{ false }; // ROI 처리 중 여부
    std::condition_variable roiProcessedCond; // ROI 처리 완료 조건 변수
    ULONG m_currentOffset;

    std::chrono::system_clock::time_point lastCallTime; // 마지막 호출 시간
    std::string m_strRawdataPath; // 원본 데이터 경로
    std::string m_SEQfilePath; // SEQ 파일 경로
    int m_nCsvFileCount; // CSV 파일 개수
    int m_nSaveInterval; // 저장 간격
    int m_iSeqFrames; // SEQ 프레임 수
    bool m_PaletteInitialized = false; // 팔레트 초기화 여부
    bool m_bYUVYFlag; // YUVY 사용 여부
    bool m_bGrayFlag; // 그레이 스케일 사용 여부
    bool m_bRGBFlag; // RGB 실화상 사용 여부
    bool m_bColorFlag; // 컬러 사용 여부
    bool m_b16BitFlag; // 16비트 사용 여부
    bool m_isRecording; // 녹화 중 여부
    bool m_bStartRecording; // 녹화 시작 여부
    bool m_bMouseImageSave; // 마우스 우클릭으로 이미지 저장 여부
    cv::Vec3b m_Markcolor; // 마커 색상
    cv::Vec3b m_findClosestColor; // 가장 가까운 색상
    BITMAPINFO* m_BitmapInfo; // 비트맵 정보
    size_t m_BitmapInfoSize; // 비트맵 정보 크기
    PaletteTypes m_colormapType; // 컬러맵 타입

    // 비공개 멤버 함수
    template <typename T>
    cv::Mat NormalizeAndProcessImage(const T* data, int height, int width, int cvType); // 이미지 정규화
    cv::Mat MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette); // 팔레트 색상 매핑
    cv::Mat ConvertUYVYToBGR8(const cv::Mat& input); // UYVY 포맷을 BGR8로 변환
    cv::Mat Convert16UC1ToBGR8(const cv::Mat& input); // 16비트 단일 채널을 BGR8로 변환
    void ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx, ROIResults& roiResult); // ROI 내 최대/최소 온도값 계산
    void Initvariable_ImageParams(); // 이미지 파라미터 초기화
    void SetPixelFormatParametertoGUI(); // PixelFormat 파라미터 설정
    std::unique_ptr<uint16_t[]> Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length); // 8비트 데이터를 16비트로 변환
    BITMAPINFO* CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channels); // 비트맵 정보 생성
    bool InitializeTemperatureThresholds(); // 온도 임계값 초기화
    void InitializeSingleRoiResult(ROIResults& roiResult); // 단일 ROI 결과 초기화
    std::vector<std::string> CreateRainbowPalette(); // 무지개 팔레트 생성
    std::vector<std::string> CreateRainbowPalette_1024(); // 1024 무지개 팔레트 생성
    cv::Vec3b findBrightestColor(const std::vector<cv::Vec3b>& colors); // 가장 밝은 색상 찾기
    cv::Vec3b findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor); // 가장 가까운 색상 찾기
    cv::Mat applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB); // 아이언 컬러맵 적용
    std::vector<cv::Vec3b> adjustPaletteScale(const std::vector<cv::Vec3b>& originalPalette, double scaleR, double scaleG, double scaleB); // 팔레트 스케일 조정
    std::vector<cv::Vec3b> convertPaletteToBGR(const std::vector<std::string>& hexPalette); // 팔레트를 BGR로 변환
    cv::Vec3b hexStringToBGR(const std::string& hexColor); // 16진수 문자열을 BGR로 변환
    void RecordThreadFunction(double frameRate); // 녹화 쓰레드 함수
    void UpdateFrame(cv::Mat newFrame); // 프레임 업데이트
    std::string GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension); // 타임스탬프를 포함한 파일 이름 생성
    bool SaveImageWithTimestamp(const cv::Mat& image); // 타임스탬프와 함께 이미지 저장
    bool SaveRawDataWithTimestamp(const cv::Mat& rawData); // 타임스탬프와 함께 원본 데이터 저장

    void SetRawdataPath(std::string path); // 원본 데이터 경로 설정
    void saveExpandedPaletteToFile(const std::vector<cv::Vec3b>& expanded_palette, PaletteTypes palette); // 확장된 팔레트를 파일로 저장
    BITMAPINFO* ConvertImageTo32BitWithBitmapInfo(const cv::Mat& imageMat, int w, int h); // 이미지를 32비트 비트맵 정보로 변환
    void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex); // 비트맵 생성 및 그리기
    void DrawImage(cv::Mat mImage, int nID, BITMAPINFO* BitmapInfo); // 이미지 그리기

    double GetScaleFactor(); // 스케일 팩터 얻기
    bool WriteCSV(std::string strPath, cv::Mat mData); // CSV 파일로 쓰기

    // ROI 연산 관련 메서드
    std::unique_ptr<uint16_t[]> extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight); // ROI 추출
    std::unique_ptr<uint16_t[]> extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight); // 전체 이미지 추출
    std::unique_ptr<uint16_t[]> extractImageData(const cv::Mat& roiMat); // 이미지 데이터 추출
    void ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, std::unique_ptr<ROIResults>& roiResult, double dScale); // 이미지 데이터 처리

    // SEQ 녹화 메서드
    bool StartSEQRecording(CString strfilePath); // SEQ 녹화 시작
    void SaveSEQ(int nWidth, int nHeight); // SEQ 저장

    // 팔레트 관련 메서드
    std::vector<cv::Vec3b> interpolatePalette(const std::vector<cv::Vec3b>& palette, int target_size = 256); // 팔레트 보간

    // 사람 감지 메서드
    void DetectAndDisplayPeople(cv::Mat& image, int& personCount, double& detectionRate); // 사람 감지 및 디스플레이
    void DisplayPersonCountAndRate(cv::Mat& image, int personCount, double detectionRate); // 사람 수 및 감지율 디스플레이

    // ROI 관리 메서드
    bool IsRoiValid(const cv::Rect& roi, int imageWidth, int imageHeight); // ROI 유효성 검사
    cv::Mat DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex); // 라이브 이미지 디스플레이

    void SetImageSize(cv::Mat& imageMat); // 이미지 크기 저장
    void SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata); // 주기적으로 파일 저장
    bool IsRoiValid(const uint8_t* imageDataPtr, int imageWidth, int imageHeight, const std::vector<cv::Rect>& rois); // ROI 유효성 검사 (이미지 데이터 포인터 사용)
    void ProcessROIsConcurrently(const byte* imageDataPtr, int imageWidth, int imageHeight);

    void addROI(const cv::Rect& rect); // ROI 추가
    void DrawAllRoiRectangles(cv::Mat& image); // 모든 ROI 사각형 그리기

    void temporaryUpdateROI(const cv::Rect& roi, ShapeType shapeType); // 임시 ROI 업데이트
    std::unique_ptr<ROIResults> ProcessSingleROI(std::unique_ptr<uint16_t[]>&& data, const cv::Rect& roi, double dScale, int nIndex); // 단일 ROI 처리
    void processAllROIs(byte* imageDataPtr, int width, int height); // 모든 ROI 처리
    bool IsValidBitmapInfo(const BITMAPINFO* bitmapInfo); // 비트맵 정보 유효성 검사
    void defalutCheckType(); // 기본 체크 타입
    bool GetROIEnabled(); // ROI 사용 가능 여부 반환
    cv::Mat CreateLUT(const std::vector<cv::Vec3b>& adjusted_palette); // LUT 생성
    void DrawRoiShape(cv::Mat& imageMat, const ROIResults& roiResult, int num_channels, ShapeType shapeType, const cv::Scalar& color = cv::Scalar(-1, -1, -1)); // ROI 형태 그리기

    CString ShapeTypeToString(ShapeType type); // 형태 타입을 문자열로 변환
    ShapeType StringToShapeType(const CString& str); // 문자열을 형태 타입으로 변환

};
