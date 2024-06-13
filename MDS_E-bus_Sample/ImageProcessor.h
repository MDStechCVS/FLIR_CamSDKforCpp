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
	bool updated;

	MDSMeasureMaxSpotValue() : x(0), y(0), pointIdx(0), tempValue(std::numeric_limits<ushort>::min()), updated(false) {}
};

struct MDSMeasureMinSpotValue
{
	int x; // x ��ǥ
	int y; // y ��ǥ
	int pointIdx; // ����Ʈ �ε���
	ushort tempValue; // �µ� ��
	bool updated;

	MDSMeasureMinSpotValue() : x(0), y(0), pointIdx(0), tempValue(std::numeric_limits<ushort>::max()), updated(false) {}
};

// ROI (���� ����) ������ �����ϴ� ������
enum class ShapeType
{
	None = -1,
	Rectangle = 0,
	Circle,
	Ellipse,
	Line
};

// �� ROI�� ����� �����ϴ� ����ü
struct ROIResults
{
	cv::Rect roi; // ROI ����
	int max_x, max_y, min_x, min_y; // �ִ�/�ּ� x, y ��ǥ
	ushort max_temp, min_temp; // �ִ�/�ּ� �µ�
	ShapeType shapeType; // ����
	MDSMeasureMaxSpotValue maxSpot; // �ִ� ���� ��
	MDSMeasureMinSpotValue minSpot; // �ּ� ���� ��
	int span; // ���� ��
	int level; // ���� ��
	int nIndex; 
	bool needsRedraw; // �ٽ� �׸� �ʿ� ����
	cv::Scalar color; 

	ROIResults() : max_x(0), max_y(0), min_x(0), min_y(0),
		max_temp(0), min_temp(65535),
		shapeType(ShapeType::None),
		span(0), level(0), nIndex(-1),
		needsRedraw(true),
		color(cv::Scalar(51, 255, 51)) {}
};

// ImageProcessor Ŭ���� ����
class ImageProcessor
{
public:
    // ������ �� �Ҹ���
    ImageProcessor(int nIndex, CameraControl_rev* pCameraControl);
    virtual ~ImageProcessor();
    CMDS_Ebus_SampleDlg* GetMainDialog();/*Main Dialog */
    CameraControl_rev* GetCam(); // ī�޶� ���� ��ü ��ȯ
    
    

    // ��� ���� �޼���
    std::string GetRawdataPath(); // ���� ������ ��� ��ȯ
    std::string GetImageSavePath(); // �̹��� ���� ��� ��ȯ
    std::string GetRawSavePath(); // ���� ���� ��� ��ȯ
    std::string GetRecordingPath(); // ��ȭ ��� ��ȯ

    void RenderDataSequence(PvImage* lImage, PvBuffer* aBuffer, byte* imageDataPtr, int nIndex); // ������ ������ ������
    cv::Mat ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg); // ������ ���� �̹��� ó��
    void ProcessAndRecordFrame(cv::Mat& processedImageMat, int nWidth, int nHeight); // ������ ó�� �� ��ȭ
    void DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels, const ROIResults& roiResult); // ROI �簢�� �׸���
    void putTextOnCamModel(cv::Mat& imageMat); // �̹����� �ؽ�Ʈ �߰�
    void DrawMinMarkerAndText(cv::Mat& imageMat, const MDSMeasureMinSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize); // �ּ� ��Ŀ�� �ؽ�Ʈ �׸���
    void DrawMaxMarkerAndText(cv::Mat& imageMat, const MDSMeasureMaxSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize); // �ִ� ��Ŀ�� �ؽ�Ʈ �׸���
    void CleanupAfterProcessing(); // ó�� �� ���� �۾�
    bool StartRecording(int frameWidth, int frameHeight, double frameRate); // ��ȭ ����
    void StopRecording(); // ��ȭ ����
    void SetStartRecordingFlag(bool bFlag); // ��ȭ ���� �÷��� ����
    bool GetStartRecordingFlag(); // ��ȭ ���� �÷��� ��ȯ
    void LoadCaminiFile(int nIndex); // ī�޶� �Ķ���� ���� ���� �ε�
    cv::Size GetImageSize(); // �̹��� ũ�� ���
    bool GetRGBType(); // RGB Ÿ�� ��ȯ
    bool Get16BitType(); // 16��Ʈ Ÿ�� ��ȯ
    bool GetYUVYType(); // YUVY Ÿ�� ��ȯ
    bool GetGrayType(); // �׷��� ������ Ÿ�� ��ȯ
    bool GetColorPaletteType(); // �÷� �ȷ�Ʈ Ÿ�� ��ȯ
    void ClearTemporaryROI(); // �ӽ� ROI Ŭ����
    cv::Rect mapRectToImage(const cv::Rect& rect, const cv::Size& imageSize, const cv::Size& dialogSize); // �簢���� �̹����� ����
    cv::Point mapPointToImage(const CPoint& point, const cv::Size& imageSize, const CRect& displayRect); // ����Ʈ�� �̹����� ����
    void StartCreatingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& startPos, cv::Point& endPos, bool& selectingROI); // ROI ���� ����
    void UpdateCreatingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& startPos, cv::Point& endPos); // ROI ���� ������Ʈ
    void ConfirmCreatingROI(const cv::Point& startPos, const cv::Point& endPos, const CRect& rect, const cv::Size& imageSize, bool& selectingROI); // ROI ���� Ȯ��
    void StartDraggingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& dragStartPos, int& selectedROIIndex, bool& isDraggingROI); // ROI �巡�� ����
    void UpdateDraggingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& dragStartPos, int selectedROIIndex); // ROI �巡�� ������Ʈ
    void ConfirmDraggingROI(bool& isDraggingROI, int& selectedROIIndex); // ROI �巡�� Ȯ��
    void deleteROI(int index); // ROI ����
    void deleteAllROIs(); // ��� ROI ����
    void updateROI(const cv::Rect& roi, int nIndex); // ROI ������Ʈ
    bool canAddROI() const; // ROI �߰� ���� ���� ��ȯ
    void confirmROI(const cv::Rect& roi, ShapeType shapeType); // ROI Ȯ��
    int findROIIndexAtPoint(const cv::Point& point); // ����Ʈ���� ROI �ε��� ã��

    // �̹��� ���� ���� �޼���
    void SetYUVYType(bool bFlag); // YUVY Ÿ�� ����
    void SetGrayType(bool bFlag); // �׷��� ������ Ÿ�� ����
    void Set16BitType(bool bFlag); // 16��Ʈ Ÿ�� ����
    void SetColorPaletteType(bool bFlag); // �÷� �ȷ�Ʈ Ÿ�� ����
    void SetRGBType(bool bFlag); // RGB Ÿ�� ����
    void SetPaletteType(PaletteTypes type); // �ȷ�Ʈ Ÿ�� ����
    PaletteTypes GetPaletteType(); // �ȷ�Ʈ Ÿ�� ��ȯ

    // ���콺 �̹��� ���� �÷���
    void SetMouseImageSaveFlag(bool bFlag); // ���콺 �̹��� ���� �÷��� ����
    bool GetMouseImageSaveFlag(); // ���콺 �̹��� ���� �÷��� ��ȯ

    void SaveRoisToIniFile(int nIndex); // INI ���Ϸ� ROI ����
    void LoadRoisFromIniFile(int nIndex); // INI ���Ͽ��� ROI �ε�

    void SetCurrentShapeType(ShapeType type); // ���� ���� Ÿ�� ����
    ShapeType GetCurrentShapeType(); // ���� ���� Ÿ�� ��ȯ

    std::mutex videomtx;
    cv::Mat m_TempData; // �µ� ������
    cv::Mat m_videoMat; // ��ȭ �̹��� ����
    std::vector<cv::Rect> m_rois; // ������ ������ �簢��
    std::vector<std::shared_ptr<ROIResults>> m_roiResults; // ���� ROI ���
    std::vector<std::shared_ptr<ROIResults>> m_temporaryRois; // �ӽ� ROI ������
    std::shared_ptr<ROIResults> m_selectedROI; // ���� ���õ� ROI
    bool m_isAddingNewRoi = true; // �� ROI �߰� ���� �÷���
    bool m_ROIEnabled = true; // ROI ��� ���� ����
    ShapeType currentShapeType; // ���� ROI ����

    // SEQ ���� ����
    ULONG m_Level;
    ULONG m_Span;
    int m_iCurrentSEQImage;
    bool m_bSaveAsSEQ; // SEQ ���� ���� ����
    CFile m_SEQFile;
    DWORD m_TrigCount;
    time_t m_ts;
    int m_ms;
    short m_tzBias;

private:

    int m_nIndex; // ī�޶� �ε���
    CameraControl_rev* m_CameraControl; // ī�޶� ���� ��ü
    
    PaletteManager paletteManager;
    std::mutex drawmtx; // �׸��� ����ȭ
    std::mutex filemtx; // ���� ����ȭ
    std::mutex m_roiMutex; // ROI ����ȭ
    std::mutex m_temporaryRoiMutex; // �ӽ� ROI ������ ��ȣ�� ���ؽ�
    std::mutex m_bitmapMutex; // ��Ʈ�� ����ȭ
    std::mutex roiDrawMtx; // ROI �׸��� ����ȭ
    ROIResults* roiResult; // ROI ���
    cv::VideoWriter videoWriter; 

    DWORD m_LineState1;
    DWORD m_LineState2;
    DWORD m_TrigCount1;
    DWORD m_TrigCount2;
    DWORD m_FrameCount;
    bool m_isSEQFileWriting; // SEQ ���� �ۼ� ����
    std::atomic<bool> isProcessingROIs{ false }; // ROI ó�� �� ����
    std::condition_variable roiProcessedCond; // ROI ó�� �Ϸ� ���� ����
    ULONG m_currentOffset;

    std::chrono::system_clock::time_point lastCallTime; // ������ ȣ�� �ð�
    std::string m_strRawdataPath; // ���� ������ ���
    std::string m_SEQfilePath; // SEQ ���� ���
    int m_nCsvFileCount; // CSV ���� ����
    int m_nSaveInterval; // ���� ����
    int m_iSeqFrames; // SEQ ������ ��
    bool m_PaletteInitialized = false; // �ȷ�Ʈ �ʱ�ȭ ����
    bool m_bYUVYFlag; // YUVY ��� ����
    bool m_bGrayFlag; // �׷��� ������ ��� ����
    bool m_bRGBFlag; // RGB ��ȭ�� ��� ����
    bool m_bColorFlag; // �÷� ��� ����
    bool m_b16BitFlag; // 16��Ʈ ��� ����
    bool m_isRecording; // ��ȭ �� ����
    bool m_bStartRecording; // ��ȭ ���� ����
    bool m_bMouseImageSave; // ���콺 ��Ŭ������ �̹��� ���� ����
    cv::Vec3b m_Markcolor; // ��Ŀ ����
    cv::Vec3b m_findClosestColor; // ���� ����� ����
    BITMAPINFO* m_BitmapInfo; // ��Ʈ�� ����
    size_t m_BitmapInfoSize; // ��Ʈ�� ���� ũ��
    PaletteTypes m_colormapType; // �÷��� Ÿ��

    // ����� ��� �Լ�
    template <typename T>
    cv::Mat NormalizeAndProcessImage(const T* data, int height, int width, int cvType); // �̹��� ����ȭ
    cv::Mat MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette); // �ȷ�Ʈ ���� ����
    cv::Mat ConvertUYVYToBGR8(const cv::Mat& input); // UYVY ������ BGR8�� ��ȯ
    cv::Mat Convert16UC1ToBGR8(const cv::Mat& input); // 16��Ʈ ���� ä���� BGR8�� ��ȯ
    void ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx, ROIResults& roiResult); // ROI �� �ִ�/�ּ� �µ��� ���
    void Initvariable_ImageParams(); // �̹��� �Ķ���� �ʱ�ȭ
    void SetPixelFormatParametertoGUI(); // PixelFormat �Ķ���� ����
    std::unique_ptr<uint16_t[]> Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length); // 8��Ʈ �����͸� 16��Ʈ�� ��ȯ
    BITMAPINFO* CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channels); // ��Ʈ�� ���� ����
    bool InitializeTemperatureThresholds(); // �µ� �Ӱ谪 �ʱ�ȭ
    void InitializeSingleRoiResult(ROIResults& roiResult); // ���� ROI ��� �ʱ�ȭ
    std::vector<std::string> CreateRainbowPalette(); // ������ �ȷ�Ʈ ����
    std::vector<std::string> CreateRainbowPalette_1024(); // 1024 ������ �ȷ�Ʈ ����
    cv::Vec3b findBrightestColor(const std::vector<cv::Vec3b>& colors); // ���� ���� ���� ã��
    cv::Vec3b findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor); // ���� ����� ���� ã��
    cv::Mat applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB); // ���̾� �÷��� ����
    std::vector<cv::Vec3b> adjustPaletteScale(const std::vector<cv::Vec3b>& originalPalette, double scaleR, double scaleG, double scaleB); // �ȷ�Ʈ ������ ����
    std::vector<cv::Vec3b> convertPaletteToBGR(const std::vector<std::string>& hexPalette); // �ȷ�Ʈ�� BGR�� ��ȯ
    cv::Vec3b hexStringToBGR(const std::string& hexColor); // 16���� ���ڿ��� BGR�� ��ȯ
    void RecordThreadFunction(double frameRate); // ��ȭ ������ �Լ�
    void UpdateFrame(cv::Mat newFrame); // ������ ������Ʈ
    std::string GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension); // Ÿ�ӽ������� ������ ���� �̸� ����
    bool SaveImageWithTimestamp(const cv::Mat& image); // Ÿ�ӽ������� �Բ� �̹��� ����
    bool SaveRawDataWithTimestamp(const cv::Mat& rawData); // Ÿ�ӽ������� �Բ� ���� ������ ����

    void SetRawdataPath(std::string path); // ���� ������ ��� ����
    void saveExpandedPaletteToFile(const std::vector<cv::Vec3b>& expanded_palette, PaletteTypes palette); // Ȯ��� �ȷ�Ʈ�� ���Ϸ� ����
    BITMAPINFO* ConvertImageTo32BitWithBitmapInfo(const cv::Mat& imageMat, int w, int h); // �̹����� 32��Ʈ ��Ʈ�� ������ ��ȯ
    void CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex); // ��Ʈ�� ���� �� �׸���
    void DrawImage(cv::Mat mImage, int nID, BITMAPINFO* BitmapInfo); // �̹��� �׸���

    double GetScaleFactor(); // ������ ���� ���
    bool WriteCSV(std::string strPath, cv::Mat mData); // CSV ���Ϸ� ����

    // ROI ���� ���� �޼���
    std::unique_ptr<uint16_t[]> extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight); // ROI ����
    std::unique_ptr<uint16_t[]> extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight); // ��ü �̹��� ����
    std::unique_ptr<uint16_t[]> extractImageData(const cv::Mat& roiMat); // �̹��� ������ ����
    void ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, std::unique_ptr<ROIResults>& roiResult, double dScale); // �̹��� ������ ó��

    // SEQ ��ȭ �޼���
    bool StartSEQRecording(CString strfilePath); // SEQ ��ȭ ����
    void SaveSEQ(int nWidth, int nHeight); // SEQ ����

    // �ȷ�Ʈ ���� �޼���
    std::vector<cv::Vec3b> interpolatePalette(const std::vector<cv::Vec3b>& palette, int target_size = 256); // �ȷ�Ʈ ����

    // ��� ���� �޼���
    void DetectAndDisplayPeople(cv::Mat& image, int& personCount, double& detectionRate); // ��� ���� �� ���÷���
    void DisplayPersonCountAndRate(cv::Mat& image, int personCount, double detectionRate); // ��� �� �� ������ ���÷���

    // ROI ���� �޼���
    bool IsRoiValid(const cv::Rect& roi, int imageWidth, int imageHeight); // ROI ��ȿ�� �˻�
    cv::Mat DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex); // ���̺� �̹��� ���÷���

    void SetImageSize(cv::Mat& imageMat); // �̹��� ũ�� ����
    void SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata); // �ֱ������� ���� ����
    bool IsRoiValid(const uint8_t* imageDataPtr, int imageWidth, int imageHeight, const std::vector<cv::Rect>& rois); // ROI ��ȿ�� �˻� (�̹��� ������ ������ ���)
    void ProcessROIsConcurrently(const byte* imageDataPtr, int imageWidth, int imageHeight);

    void addROI(const cv::Rect& rect); // ROI �߰�
    void DrawAllRoiRectangles(cv::Mat& image); // ��� ROI �簢�� �׸���

    void temporaryUpdateROI(const cv::Rect& roi, ShapeType shapeType); // �ӽ� ROI ������Ʈ
    std::unique_ptr<ROIResults> ProcessSingleROI(std::unique_ptr<uint16_t[]>&& data, const cv::Rect& roi, double dScale, int nIndex); // ���� ROI ó��
    void processAllROIs(byte* imageDataPtr, int width, int height); // ��� ROI ó��
    bool IsValidBitmapInfo(const BITMAPINFO* bitmapInfo); // ��Ʈ�� ���� ��ȿ�� �˻�
    void defalutCheckType(); // �⺻ üũ Ÿ��
    bool GetROIEnabled(); // ROI ��� ���� ���� ��ȯ
    cv::Mat CreateLUT(const std::vector<cv::Vec3b>& adjusted_palette); // LUT ����
    void DrawRoiShape(cv::Mat& imageMat, const ROIResults& roiResult, int num_channels, ShapeType shapeType, const cv::Scalar& color = cv::Scalar(-1, -1, -1)); // ROI ���� �׸���

    CString ShapeTypeToString(ShapeType type); // ���� Ÿ���� ���ڿ��� ��ȯ
    ShapeType StringToShapeType(const CString& str); // ���ڿ��� ���� Ÿ������ ��ȯ

};
