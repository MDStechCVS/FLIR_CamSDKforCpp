#include "ImageProcessor.h"

// =============================================================================
ImageProcessor::ImageProcessor(int nIndex, CameraControl_rev* pCameraControl)
    : m_nIndex(nIndex), m_CameraControl(pCameraControl)
{
    // 생성자
    
    // setting ini 파일 불러오기
    LoadCaminiFile(nIndex);

    // 기본값 설정
    defalutCheckType();

    // 변수초기화
    Initvariable_ImageParams();
    //픽셀 포멧에 따라서 GUI Status 변경
    SetPixelFormatParametertoGUI();

    m_CameraControl = pCameraControl;
}

// =============================================================================
ImageProcessor::~ImageProcessor() 
{
    // 소멸자 내용 (필요한 경우 구현)
}

// =============================================================================
CameraControl_rev* ImageProcessor::GetCam()
{
    return m_CameraControl;
}

// =============================================================================
void ImageProcessor::RenderDataSequence(PvImage* lImage, PvBuffer* aBuffer, byte* imageDataPtr, int nIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    FPGA_HEADER* pFPGA;

    // 스트리밍 이미지 가로 세로
    int nWidth = lImage->GetWidth();
    int nHeight = lImage->GetHeight();
    int nArraysize = nWidth * nHeight;

    // 생성할 이미지의 채널 수 및 데이터 타입 선택
    int num_channels = (Get16BitType()) ? 16 : 8;
    bool isYUVYType = GetYUVYType();
    int dataType = (Get16BitType() ? CV_16UC1 : (isYUVYType ? CV_8UC2 : CV_8UC1));


    std::unique_ptr<uint16_t[]> fulldata = nullptr;
    std::unique_ptr<uint16_t[]> roiData = nullptr;

    fulldata = extractWholeImage(imageDataPtr, nArraysize, nWidth, nHeight);

    if (fulldata == nullptr) 
        return; 

    // Here we have successfully received a valid block of data (such as an image)
    m_FrameCount++;
    
    if (GetStartRecordingFlag())
    {
        if (GetCam()->m_Camlist != CAM_MODEL::Ax5)
        {
            // Check trig count
            pFPGA = (FPGA_HEADER*)&fulldata[GetCam()->m_Cam_Params->nHeight * nWidth];
            if (pFPGA->dp1_trig_type & FPGA_TRIG_TYPE_MARK)
            {
                m_TrigCount1++;
                m_TrigCount++;
            }

            m_LineState1 = pFPGA->dp1_trig_state ? 1 : 0;
            m_LineState2 = pFPGA->dp2_trig_state ? 1 : 0;

            if (pFPGA->dp2_trig_type & FPGA_TRIG_TYPE_MARK)
            {
                m_TrigCount2++;
                m_TrigCount++;
            }
        }
    }
    
    // 멀티 ROI 병렬 처리 함수 호출
    if(m_roiResults.size() > 0)
    {
        ProcessROIsConcurrently(imageDataPtr, nWidth, nHeight);
    }

    if (fulldata != nullptr)
    {
        cv::Mat fulldataMat(nHeight, nWidth, dataType, fulldata.get());
        if (fulldataMat.empty()) 
        {
            return;
        }

        cv::Mat tempCopy = fulldataMat.clone(); // fulldataMat을 복제한 새로운 행렬 생성
        {
            if (m_TempData.empty())
            {
                m_TempData = cv::Mat::zeros(nHeight, nWidth, dataType);
                tempCopy.copyTo(m_TempData); // m_TempData에 데이터를 복사합니다.
            }
        }
    }

    // 이미지 노멀라이즈
    cv::Mat processedImageMat = ProcessImageBasedOnSettings(imageDataPtr, nHeight, nWidth, MainDlg);

    //ROI 객체 이미지 사이즈 저장
    SetImageSize(processedImageMat);

    //ROI 사각형 영역이 생겼을 경우에만 그린다
    //if(GetROIEnabled())
    //    DrawAllRoiRectangles(processedImageMat);

    // 카메라 모델명 좌상단에 그리기
    //putTextOnCamModel(processedImageMat);
    
    // 화면에 라이브 이미지 및 ROI 그리기
    processedImageMat = DisplayLiveImage(MainDlg, processedImageMat, nIndex);

    // 일정 간격으로 이미지, 로우데이터 자동 저장 
    SaveFilePeriodically(m_TempData, processedImageMat);

    //녹화 대기 함수
    ProcessAndRecordFrame(processedImageMat, nWidth, nHeight);
    
    //  작업이 완료된 비트맵 정보를 삭제합니다, 동적으로 할당한 메모리를 정리
    CleanupAfterProcessing();
}

// =============================================================================
void ImageProcessor::Initvariable_ImageParams()
{

    CString strLog = _T("");

    m_iCurrentSEQImage = 0;
    m_TrigCount = 0; // Total trig count

    m_bYUVYFlag = false;
    m_bStartRecording = false;
    m_isSEQFileWriting = false;
    m_isAddingNewRoi = true;
    m_ROIEnabled = true;
    m_iSeqFrames = 10;
    m_nCsvFileCount = 1;

    m_TrigCount1 = 0;
    m_TrigCount2 = 0;
    m_LineState1 = 0;
    m_LineState2 = 0;

    m_colormapType = PALETTE_TYPE::PALETTE_IRON; // 컬러맵 변수, 초기값은 IRON으로 설정

    m_TempData = cv::Mat();
    m_videoMat = cv::Mat();

    SetCurrentShapeType(ShapeType:: Rectangle);

    std::string strPath = CT2A(Common::GetInstance()->Getsetingfilepath());
    m_strRawdataPath = strPath + "Camera_" + std::to_string(GetCam()->GetCamIndex() + 1) + "\\";
    SetRawdataPath(m_strRawdataPath);

    Common::GetInstance()->CreateDirectoryRecursively(GetRawdataPath());
    Common::GetInstance()->CreateDirectoryRecursively(GetImageSavePath());
    Common::GetInstance()->CreateDirectoryRecursively(GetRawSavePath());
    Common::GetInstance()->CreateDirectoryRecursively(GetRecordingPath());

    strLog.Format(_T("---------Camera[%d] ImageParams Variable Initialize "), GetCam()->GetCamIndex() + 1);
    Common::GetInstance()->AddLog(0, strLog);
}

// =============================================================================
//설정 파일 불러오기
void ImageProcessor::LoadCaminiFile(int nIndex)
{

    CString filePath = _T(""), strFileName = _T("");
    CString iniSection = _T(""), iniKey = _T(""), iniValue = _T("");
    TCHAR cbuf[MAX_PATH] = { 0, };

    strFileName.Format(_T("CameraParams_%d.ini"), nIndex + 1);
    filePath.Format(Common::GetInstance()->SetProgramPath(strFileName));

    iniSection.Format(_T("Params"));
    iniKey.Format(_T("PixelType"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    GetCam()->m_Cam_Params->strPixelFormat = iniValue;

    iniKey.Format(_T("Save_interval"));
    m_nSaveInterval = GetPrivateProfileInt(iniSection, iniKey, (bool)false, filePath);

    LoadRoisFromIniFile(nIndex);

    // Palette 생성
    std::string baseDir = Common::GetInstance()->GetRootPathA() + "\\res\\palette";
    paletteManager.init(baseDir);
}

// =============================================================================
void ImageProcessor::SaveRoisToIniFile(int nIndex) 
{
    CString filePath, strFileName;
    strFileName.Format(_T("CameraParams_%d.ini"), nIndex + 1);
    filePath.Format(Common::GetInstance()->SetProgramPath(strFileName));

    std::lock_guard<std::mutex> lock(m_roiMutex);
    for (size_t i = 0; i < m_roiResults.size(); ++i) {
        CString iniSection;
        iniSection.Format(_T("ROI_%d"), static_cast<int>(i) + 1);

        WritePrivateProfileString(iniSection, _T("x"), std::to_wstring(m_roiResults[i]->roi.x).c_str(), filePath);
        WritePrivateProfileString(iniSection, _T("y"), std::to_wstring(m_roiResults[i]->roi.y).c_str(), filePath);
        WritePrivateProfileString(iniSection, _T("width"), std::to_wstring(m_roiResults[i]->roi.width).c_str(), filePath);
        WritePrivateProfileString(iniSection, _T("height"), std::to_wstring(m_roiResults[i]->roi.height).c_str(), filePath);
        WritePrivateProfileString(iniSection, _T("shapeType"), ShapeTypeToString(m_roiResults[i]->shapeType), filePath);
    }
}

// =============================================================================
void ImageProcessor::LoadRoisFromIniFile(int nIndex)
{
    CString filePath, strFileName;
    strFileName.Format(_T("CameraParams_%d.ini"), nIndex + 1);
    filePath.Format(Common::GetInstance()->SetProgramPath(strFileName));

    std::lock_guard<std::mutex> lock(m_roiMutex);
    m_roiResults.clear();

    for (int i = 0; ; ++i) {
        CString iniSection;
        iniSection.Format(_T("ROI_%d"), i + 1);

        int x = GetPrivateProfileInt(iniSection, _T("x"), -1, filePath);
        if (x == -1) break; // No more ROI sections

        int y = GetPrivateProfileInt(iniSection, _T("y"), -1, filePath);
        int width = GetPrivateProfileInt(iniSection, _T("width"), -1, filePath);
        int height = GetPrivateProfileInt(iniSection, _T("height"), -1, filePath);
        CString shapeTypeStr;
        GetPrivateProfileString(iniSection, _T("shapeType"), _T("None"), shapeTypeStr.GetBuffer(256), 256, filePath);
        shapeTypeStr.ReleaseBuffer();

        cv::Rect roi(x, y, width, height);
        auto roiResult = std::make_shared<ROIResults>();
        roiResult->roi = roi;
        roiResult->shapeType = StringToShapeType(shapeTypeStr);
        roiResult->nIndex = static_cast<int>(m_roiResults.size()) + 1;

        m_roiResults.push_back(roiResult);
    }
}


// =============================================================================
// ShapeType을 문자열로 변환
CString ImageProcessor::ShapeTypeToString(ShapeType type) 
{
    switch (type) 
    {
        case ShapeType::Rectangle: return _T("Rectangle");
        case ShapeType::Circle: return _T("Circle");
        case ShapeType::Ellipse: return _T("Ellipse");
        case ShapeType::Line: return _T("Line");
        default: return _T("None");
    }
}

// =============================================================================
ShapeType ImageProcessor::StringToShapeType(const CString& str)
{
    if (str == _T("Rectangle")) return ShapeType::Rectangle;
    if (str == _T("Circle")) return ShapeType::Circle;
    if (str == _T("Ellipse")) return ShapeType::Ellipse;
    if (str == _T("Line")) return ShapeType::Line;
    return ShapeType::None;
}


// =============================================================================
void ImageProcessor::defalutCheckType()
{
    // YUVY 경우일경우 체크박스 활성화
    if (GetCam()->m_Cam_Params->strPixelFormat == YUV422_8_UYVY)
    {
        SetYUVYType(TRUE);
        Set16BitType(FALSE);
        SetGrayType(FALSE);
        SetRGBType(FALSE);
    }
    else if (GetCam()->m_Cam_Params->strPixelFormat == MONO8)
    {
        SetGrayType(TRUE);
        SetYUVYType(FALSE);
        Set16BitType(FALSE);
        SetRGBType(FALSE);
    }
    else if (GetCam()->m_Cam_Params->strPixelFormat == MONO16 || GetCam()->m_Cam_Params->strPixelFormat == MONO14)
    {
        Set16BitType(TRUE);
        SetGrayType(TRUE);
        SetYUVYType(FALSE);
        SetRGBType(FALSE);
    }
    else if (GetCam()->m_Cam_Params->strPixelFormat == RGB8PACK)
    {
        SetGrayType(FALSE);
        SetYUVYType(FALSE);
        Set16BitType(FALSE);
        SetRGBType(TRUE);
    }
    else
    {
        SetGrayType(FALSE);
        SetYUVYType(FALSE);
        Set16BitType(FALSE);
        SetRGBType(FALSE);
    }
}

// =============================================================================
// 주 메인 다이얼로그 포인터
CMDS_Ebus_SampleDlg* ImageProcessor::GetMainDialog()
{
    return (CMDS_Ebus_SampleDlg*)AfxGetApp()->GetMainWnd();
}

// =============================================================================
void ImageProcessor::SetImageSize(cv::Mat& img)
{
    m_TempData.copySize(img);
}

// =============================================================================
cv::Size ImageProcessor::GetImageSize()
{
    return m_TempData.size();
}

// =============================================================================
void ImageProcessor::putTextOnCamModel(cv::Mat& imageMat) 
{
    if (!imageMat.empty())
    {
        // CString을 std::string으로 변환합니다.m_CameraControl
        CString cstrText = GetCam()->Manager->m_strSetModelName.at(m_CameraControl->GetCamIndex());
        std::string strText = std::string(CT2CA(cstrText));

        // 텍스트를 이미지에 삽입합니다.
        cv::putText(imageMat, strText, cv::Point(0, 25), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
    }
}

// =============================================================================
// 8비트를 16비트로 변환
std::unique_ptr<uint16_t[]> ImageProcessor::Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length)
{

    // roiWidth와 roiHeight 크기의 uint16_t 배열을 동적으로 할당합니다.
    std::unique_ptr<uint16_t[]> roiArray = std::make_unique<uint16_t[]>(length);
    for (int i = 0; i < length; i++)
    {
        if (!GetCam()->GetCamRunningFlag())
            return nullptr;

        // 데이터를 읽고 예상 형식과 일치하는지 확인
        uint16_t sample = static_cast<uint16_t>(src[i * 2] | (src[i * 2 + 1] << 8));

        roiArray[i] = sample;
    }

    return roiArray;
}

// =============================================================================
// Main GUI 라이브 이미지출력
cv::Mat ImageProcessor::DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex)
{
    if (processedImageMat.empty())
        return cv::Mat();

    cv::Mat ResultImage;
    cv::Mat GrayImage;
    int num_channels = Get16BitType() ? 16 : 8;

    // 16bit
    if (Get16BitType())
    {
        if (GetColorPaletteType())
        {
            ResultImage = MapColorsToPalette(processedImageMat, GetPaletteType());
            if (ResultImage.type() == CV_8UC1)
            {
                cv::cvtColor(ResultImage, ResultImage, cv::COLOR_GRAY2BGR);
            }
        }
        else if (GetGrayType())
        {
            cv::cvtColor(processedImageMat, GrayImage, cv::COLOR_GRAY2BGR);
            cv::normalize(GrayImage, ResultImage, 0, 255, cv::NORM_MINMAX, CV_8UC1);
            cv::cvtColor(ResultImage, ResultImage, cv::COLOR_BGR2GRAY);
        }
        else
        {
            processedImageMat.copyTo(ResultImage);
        }

        if (GetROIEnabled())
        {
            DrawAllRoiRectangles(ResultImage);
        }
    }
    // 8bit
    else
    {
        if (GetYUVYType())
        {
            ResultImage = ConvertUYVYToBGR8(processedImageMat);
        }
        else if (GetGrayType())
        {
            cv::cvtColor(processedImageMat, GrayImage, cv::COLOR_GRAY2BGR);
            cv::normalize(GrayImage, ResultImage, 0, 255, cv::NORM_MINMAX, CV_8UC1);
            cv::cvtColor(ResultImage, ResultImage, cv::COLOR_BGR2GRAY);
        }
        else
        {
            processedImageMat.copyTo(ResultImage);
        }
    }

    CreateAndDrawBitmap(MainDlg, ResultImage, num_channels, nIndex);

    return ResultImage;
}

// =============================================================================
// 이미지 처리 함수
cv::Mat ImageProcessor::ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg)
{
    bool isYUVYType = GetYUVYType() == TRUE;
    bool isRGBType = GetRGBType() == TRUE;
    int dataType = CV_8UC1;

    if (Get16BitType())
    {
        dataType = CV_16UC1;
    }
    else if (isYUVYType)
    {
        dataType = CV_8UC2;
    }
    else if (isRGBType)
    {
        dataType = CV_8UC3;
    }

    return NormalizeAndProcessImage(imageDataPtr, nHeight, nWidth, dataType);
}

// =============================================================================
void ImageProcessor::ProcessAndRecordFrame(cv::Mat& processedImageMat, int nWidth, int nHeight)
{
    CString strLog = _T("");



    // ROI 처리 완료 대기
    std::unique_lock<std::mutex> lock(m_roiMutex);
    roiProcessedCond.wait(lock, [this]() { return !isProcessingROIs.load(); }); // ROI 처리가 완료될 때까지 대기

    if (GetStartRecordingFlag())
    {
        if (!m_isRecording && !processedImageMat.empty())
        {
            // ROI가 있다면 삭제
            if (!m_roiResults.empty())
            {
                m_roiResults.clear(); // Clear all stored ROIs
            }

            if (StartRecording(nWidth, nHeight, GetCam()->GetCameraFPS()))
            {
                GetCam()->ClearQueue();
                m_bSaveAsSEQ = true;
                // StartRecording open 성공   
                strLog.Format(_T("---------Camera[%d] Video Start Recording"), GetCam()->GetCamIndex() + 1);
                Common::GetInstance()->AddLog(0, strLog);
            }
            else
            {
                strLog.Format(_T("---------Camera[%d] Video Open Fail"), GetCam()->GetCamIndex() + 1);
                Common::GetInstance()->AddLog(0, strLog);
            }
        }
        else
        {
            // 이미지 프레임을 녹화 전용 데이터로 복사
            UpdateFrame(processedImageMat);
        }
    }
    else
    {

    }
}

// =============================================================================
// 이미지 처리 중 동적 생성 객체 메모리 해제
void ImageProcessor::CleanupAfterProcessing() 
{

    std::lock_guard<std::mutex> lock(m_bitmapMutex);
    if (m_BitmapInfo != nullptr)
    {
        delete[] reinterpret_cast<BYTE*>(m_BitmapInfo); // 메모리 해제
        m_BitmapInfo = nullptr;
        m_BitmapInfoSize = 0;
    }
}

// =============================================================================
std::vector<std::string> ImageProcessor::CreateRainbowPalette()
{
    //int paletteSize = 128; // 색상 개수 확장
    //int segmentLength = paletteSize / 3; // 각 세그먼트의 길이

    //for (int i = 0; i < paletteSize; ++i) {
    //    int r, g, b;

    //    if (i < segmentLength) {
    //        r = static_cast<int>((1 - sin(M_PI * static_cast<double>(i) / segmentLength / 2)) * 255);
    //        g = 0;
    //        b = static_cast<int>((sin(M_PI * static_cast<double>(i) / segmentLength / 2)) * 255);
    //    }
    //    else if (i < 2 * segmentLength) {
    //        r = 0;
    //        g = static_cast<int>((sin(M_PI * static_cast<double>(i - segmentLength) / segmentLength / 2)) * 255);
    //        b = static_cast<int>((1 - sin(M_PI * static_cast<double>(i - segmentLength) / segmentLength / 2)) * 255);
    //    }
    //    else {
    //        r = static_cast<int>((sin(M_PI * static_cast<double>(i - 2 * segmentLength) / segmentLength / 2)) * 255);
    //        g = static_cast<int>((1 - sin(M_PI * static_cast<double>(i - 2 * segmentLength) / segmentLength / 2)) * 255);
    //        b = 0;
    //    }

    //    char color[8];
    //    snprintf(color, sizeof(color), "#%02X%02X%02X", r, g, b);
    //    Rainbow_palette.push_back(color);
    //}
    //return Rainbow_palette;
    std::vector<std::string> palette;
    return palette;
}

// =============================================================================
std::vector<std::string> ImageProcessor::CreateRainbowPalette_1024()
{
    //int paletteSize = 1024; // 색상 개수 확장
    //int segmentLength = paletteSize / 3; // 각 세그먼트의 길이

    //for (int i = 0; i < paletteSize; ++i) {
    //    int r, g, b;

    //    if (i < segmentLength) {
    //        r = static_cast<int>((1 - sin(M_PI * i / segmentLength / 2)) * 255);
    //        g = 0;
    //        b = static_cast<int>((sin(M_PI * i / segmentLength / 2)) * 255);
    //    }
    //    else if (i < 2 * segmentLength) {
    //        r = 0;
    //        g = static_cast<int>((sin(M_PI * (i - segmentLength) / segmentLength / 2)) * 255);
    //        b = static_cast<int>((1 - sin(M_PI * (i - segmentLength) / segmentLength / 2)) * 255);
    //    }
    //    else {
    //        r = static_cast<int>((sin(M_PI * (i - 2 * segmentLength) / segmentLength / 2)) * 255);
    //        g = static_cast<int>((1 - sin(M_PI * (i - 2 * segmentLength) / segmentLength / 2)) * 255);
    //        b = 0;
    //    }

    //    char color[8];
    //    snprintf(color, sizeof(color), "#%02X%02X%02X", r, g, b);
    //    Rainbow_palette.push_back(color);
    //}
    //return Rainbow_palette;
    std::vector<std::string> palette;
    return palette;
}

// =============================================================================
std::vector<cv::Vec3b> ImageProcessor::adjustPaletteScale(const std::vector<cv::Vec3b>&originalPalette, double scaleR, double scaleG, double scaleB)
{
    std::vector<cv::Vec3b> adjustedPalette;
    for (const cv::Vec3b& color : originalPalette)
    {
        // 각 색상의 B, G, R 값을 스케일링
        uchar newB = static_cast<uchar>(color[2] * scaleB);
        uchar newG = static_cast<uchar>(color[1] * scaleG);
        uchar newR = static_cast<uchar>(color[0] * scaleR);


        // 값이 255를 초과하지 않도록 제한
        newB = std::min(255, (int)newB);
        newG = std::min(255, (int)newG);
        newR = std::min(255, (int)newR);

        adjustedPalette.push_back(cv::Vec3b(newB, newG, newR));
    }
    return adjustedPalette;
}

// =============================================================================
void ImageProcessor::saveExpandedPaletteToFile(const std::vector<cv::Vec3b>& expanded_palette, PaletteTypes palette)
{
    int arrayLength = 0;
    while (ColormapArray::colormapStrings[arrayLength] != nullptr)
    {
        ++arrayLength;
    }

    std::basic_string<TCHAR> paletteTypeName;
    int paletteIndex = static_cast<int>(palette);
    if (paletteIndex >= 0 && paletteIndex < arrayLength) {
        paletteTypeName = ColormapArray::colormapStrings[paletteIndex];
    }
    else {
        paletteTypeName = _T("Unknown");
    }

    std::basic_string<TCHAR> fileName = _T("paletteType_") + paletteTypeName + _T(".txt");

    std::ofstream paletteFile(std::string(fileName.begin(), fileName.end())); // Convert std::wstring to std::string

    if (paletteFile.is_open()) {
        for (const auto& color : expanded_palette) {
            paletteFile << static_cast<int>(color[0]) << "," << static_cast<int>(color[1]) << "," << static_cast<int>(color[2]) << std::endl;
        }
        paletteFile.close();
    }
    else {
        std::cerr << "Unable to open palette file for writing!" << std::endl;
    }
}

cv::Mat ImageProcessor::CreateLUT(const std::vector<cv::Vec3b>& adjusted_palette) 
{
    cv::Mat b(256, 1, CV_8U), g(256, 1, CV_8U), r(256, 1, CV_8U);
    for (int i = 0; i < 256; ++i) {
        b.at<uchar>(i) = adjusted_palette[i][0];
        g.at<uchar>(i) = adjusted_palette[i][1];
        r.at<uchar>(i) = adjusted_palette[i][2];
    }
    cv::Mat channels[] = { b, g, r };
    cv::Mat lut;
    cv::merge(channels, 3, lut);
    return lut;
}

// =============================================================================
cv::Mat ImageProcessor::applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB)
{
    // 팔레트 가져오기
    std::vector<cv::Vec3b> Selectedpalette = paletteManager.GetPalette(palette);

    // 스케일을 적용한 조정된 팔레트 생성
    std::vector<cv::Vec3b> adjusted_palette = adjustPaletteScale(Selectedpalette, scaleR, scaleG, scaleB);
   

    //사용 안함
    //     //m_Markcolor = findBrightestColor(adjusted_palette);
    //cv::Vec3b greenColor(0, 0, 0);
    //m_findClosestColor = findClosestColor(adjusted_palette, greenColor);
    
    cv::Mat lut = CreateLUT(adjusted_palette);
    cv::Mat im_color;
    cv::LUT(im_gray, lut, im_color);

    return im_color;
}

// =============================================================================
std::string ImageProcessor::GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension)
{
    // 현재 시간 정보 가져오기
    auto currentTime = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::to_time_t(currentTime);
    struct tm localTime;
    localtime_s(&localTime, &now);

    // 시간 정보를 이용하여 파일 이름 생성
    char timeStr[50];
    strftime(timeStr, sizeof(timeStr), "_%Y_%m_%d_%H_%M_%S", &localTime);

    // 최종 파일 경로 생성
    std::string filePath = basePath + prefix + timeStr + extension;

    return filePath;
}

// =============================================================================
bool ImageProcessor::SaveImageWithTimestamp(const cv::Mat& image)
{
    std::string basePath = GetImageSavePath(); // 이미지 저장 경로
    std::string prefix = "temperature_image"; // 파일 이름 접두사
    std::string extension = ".jpg"; // 확장자

    // 파일 이름 생성
    std::string filePath = GenerateFileNameWithTimestamp(basePath, prefix, extension);

    // 이미지 저장
    if (!cv::imwrite(filePath, image))
    {
        // 오류 처리
        return false;
    }

    // 성공적으로 저장한 경우 로그 출력
    CString strLog;
    std::wstring filePathW(filePath.begin(), filePath.end());
    strLog.Format(_T("Completed writeback to image file: \n%s"), filePathW.c_str());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

// =============================================================================
bool ImageProcessor::SaveRawDataWithTimestamp(const cv::Mat& rawData)
{
    std::string basePath = GetRawSavePath(); // Raw 데이터 저장 경로
    std::string prefix = "rawdata"; // 파일 이름 접두사
    std::string extension = ".tmp"; // 확장자

    // 파일 이름 생성
    std::string filePath = GenerateFileNameWithTimestamp(basePath, prefix, extension);

    // Raw 데이터 저장
    if (!WriteCSV(filePath, rawData))
    {
        // 오류 처리
        return false;
    }

    // 성공적으로 저장한 경우 로그 출력
    CString strLog;
    std::wstring filePathW(filePath.begin(), filePath.end());
    strLog.Format(_T("Completed writeback to raw data file: \n%s"), filePathW.c_str());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

// =============================================================================
void ImageProcessor::SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata)
{
    if (rawdata.empty() || imagedata.empty())
        return;

    std::lock_guard<std::mutex> lock(filemtx);
    CString strLog = _T("");

    //유저 선택 이미지 저장
    if (GetMouseImageSaveFlag())
    {
        SaveImageWithTimestamp(imagedata);
        SetMouseImageSaveFlag(FALSE);
    }

    if (lastCallTime.time_since_epoch().count() == 0)
    {
        lastCallTime = std::chrono::system_clock::now(); // 첫 함수 호출시 lastCallTime 초기화
    }

    auto currentTime = std::chrono::system_clock::now();
    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastCallTime).count();

    int seconds = m_nSaveInterval * 60;

    if (elapsedSeconds >= seconds) //
    {
        // 추가: Mat 객체를 JPEG, tmp 파일로 저장

        SaveRawDataWithTimestamp(rawdata);
        SaveImageWithTimestamp(imagedata);

        m_nCsvFileCount++;
        lastCallTime = currentTime;
    }
}

// =============================================================================
void ImageProcessor::UpdateFrame(Mat newFrame)
{
}

// =============================================================================
bool ImageProcessor::StartRecording(int frameWidth, int frameHeight, double frameRate)
{
    CString strLog = _T("");

    if (m_isRecording)
    {
        strLog.Format(_T("---------Camera[%d] Already recording"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        return false; // 이미 녹화 중이므로 false 반환
    }
    if (GetGrayType())
    {
        SetGrayType(FALSE);
        SetColorPaletteType(TRUE);
    }
    


    m_isRecording = true;

    // 녹화 이미지 생성을 위한 쓰레드 시작
    std::thread recordThread(&ImageProcessor::RecordThreadFunction, this, frameRate);
    recordThread.detach();  // 안전한 쓰레드 종료 관리


    bool bSaveType = !GetGrayType(); // GetGrayType()가 true면 false, 아니면 true로 설정

    /*
    auto currentTime = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::to_time_t(currentTime);
    struct tm localTime;
    localtime_s(&localTime, &now);
    char timeStr[50];
    strftime(timeStr, sizeof(timeStr), "_%Y_%m_%d_%H_%M_%S", &localTime);
    std::string outputFileName = GetRecordingPath() + "recording" + std::string(timeStr) + ".avi";
    int fourcc = videoWriter.fourcc('D', 'I', 'V', 'X');
    videoWriter.open(outputFileName, fourcc, frameRate, cv::Size(frameWidth, frameHeight), bSaveType);
    if (!videoWriter.isOpened())
    {
        strLog.Format(_T("---------Camera[%d] VideoWriter for video recording cannot be opened"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
        return false; // 녹화 실패 반환
    }
    else
    {
        strLog.Format(_T("---------Camera[%d] Video Writer open success"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    */
    

    return true; // 녹화 시작 성공 반환
}

// =============================================================================
void ImageProcessor::RecordThreadFunction(double frameRate) 
{
    while (GetStartRecordingFlag())
    {
        if (GetCam()->GetQueueBufferCount() > 0)
            SaveSEQ((int)GetCam()->m_Cam_Params->nWidth, (int)GetCam()->m_Cam_Params->nHeight);

        std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
}

// =============================================================================
void ImageProcessor::StopRecording()
{
    if (!m_isRecording)
    {
        return;
    }
    //videoWriter.release();

    CString cstrFilePath(m_SEQfilePath.c_str());
    CString strLog = _T("");

    if (m_SEQFile.m_hFile != CFile::hFileNull)
    {
        m_SEQFile.Close();
    }
    m_isRecording = false;
    m_bSaveAsSEQ = false;
    SetStartRecordingFlag(false);

    strLog.Format(_T("---------Camera[%d] Video Stop Recording"), m_CameraControl->GetCamIndex() + 1);
    Common::GetInstance()->AddLog(0, strLog);
}

// =============================================================================
void ImageProcessor::SetStartRecordingFlag(bool bFlag)
{
    m_bStartRecording = bFlag;
    m_bSaveAsSEQ = true;

    if(bFlag)
        m_iCurrentSEQImage = 0;
}

// =============================================================================
bool ImageProcessor::GetStartRecordingFlag()
{
    return m_bStartRecording;
}

// =============================================================================
void ImageProcessor::SetMouseImageSaveFlag(bool bFlag)
{
    m_bMouseImageSave = bFlag;
}
// =============================================================================
bool ImageProcessor::GetMouseImageSaveFlag()
{
    return m_bMouseImageSave;
}

// =============================================================================
BITMAPINFO* ImageProcessor::ConvertImageTo32BitWithBitmapInfo(const cv::Mat& imageMat, int w, int h)
{
    int imageSize = 0;
    int nbiClrUsed = 0;

    try
    {
        // 이미지 데이터 복사
        cv::Mat copiedImage;
        imageMat.copyTo(copiedImage);

        // 이미지를 32비트로 변환 (4채널)
        cv::Mat convertedImage;
        copiedImage.convertTo(convertedImage, CV_32FC1, 1.0 / 65535.0); // 16비트에서 32비트로 변환

        // 이미지 크기 계산
        imageSize = w * h * 4;

        // 새로운 BITMAPINFO 할당
        BITMAPINFO* m_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)]);
        nbiClrUsed = 0; // 색상 팔레트를 사용하지 않음

        // 이미지 데이터 복사 및 변환
        unsigned char* dstData = (unsigned char*)((BITMAPINFO*)m_BitmapInfo)->bmiColors;
        for (int i = 0; i < w * h; ++i)
        {
            float pixel32 = convertedImage.at<float>(i);
            unsigned char pixelByte = static_cast<unsigned char>(pixel32 * 255.0f);
            dstData[i * 4] = pixelByte; // Blue 채널
            dstData[i * 4 + 1] = pixelByte; // Green 채널
            dstData[i * 4 + 2] = pixelByte; // Red 채널
            dstData[i * 4 + 3] = 255; // Alpha 채널 (255: 완전 불투명)
        }

        return m_BitmapInfo;
    }
    catch (const std::bad_alloc&)
    {
        throw std::runtime_error("Failed to allocate memory for BITMAPINFO");
    }
}

// =============================================================================
cv::Vec3b ImageProcessor::hexStringToBGR(const std::string& hexColor)
{
    int r, g, b;
    sscanf(hexColor.c_str(), "#%02x%02x%02x", &r, &g, &b);

    r = std::min(255, std::max(0, r));
    g = std::min(255, std::max(0, g));
    b = std::min(255, std::max(0, b));

    return cv::Vec3b(b, g, r);
}

/**
 * @brief 주어진 16진수 문자열 색상 팔레트를 BGR (Blue, Green, Red) 형식의 벡터로 변환합니다.
 *
 * 이 함수는 16진수 문자열 형태의 색상 팔레트를 받아와 각 색상을 BGR 형식의 벡터로 변환합니다.
 * 변환된 결과는 캐시를 사용하여 메모리와 연산 비용을 줄이기 위해 저장됩니다.
 * 이미 변환된 색상 팔레트에 대한 요청은 캐시에서 검색하여 성능을 최적화합니다.
 *
 * @param hexPalette 16진수 문자열 형태의 색상 팔레트. 예를 들어, "#FF0000"은 빨간색을 나타냅니다.
 * @return BGR 형식의 벡터로 변환된 색상 팔레트.
 *
 * @note 변환된 팔레트는 캐시를 사용하여 저장되며, 이미 변환된 경우 캐시에서 결과를 반환합니다.
 *       이를 통해 중복 변환 및 연산 비용을 효율적으로 관리합니다.
 *
 * @warning 이 함수는 캐시를 사용하기 때문에 입력 색상 팔레트의 내용이 변경되면
 *          결과가 예상치 못하게 달라질 수 있으므로 주의해야 합니다.
 */
 // =============================================================================
std::vector<cv::Vec3b> ImageProcessor::convertPaletteToBGR(const std::vector<std::string>& hexPalette)
{

    //// 이미 변환된 결과를 저장할 캐시
    //static std::map<std::vector<std::string>, std::vector<cv::Vec3b>> cache; // std::map으로 변경

    //// 입력 벡터에 대한 캐시 키 생성
    //std::vector<std::string> cacheKey = hexPalette;

    //// 캐시에서 이미 변환된 결과를 찾아 반환
    //if (cache.find(cacheKey) != cache.end())
    //{
    //    return cache[cacheKey];
    //}

    //// 변환 작업 수행
    //std::vector<cv::Vec3b> bgrPalette;
    //for (const std::string& hexColor : hexPalette)
    //{
    //    cv::Vec3b bgrColor = hexStringToBGR(hexColor);
    //    bgrPalette.push_back(bgrColor);
    //}

    //// 변환 결과를 캐시에 저장
    //cache[cacheKey] = bgrPalette;

    //// 캐시에 저장된 복사본을 반환
    //return cache[cacheKey];
    return std::vector<cv::Vec3b>();
}

// =============================================================================
cv::Vec3b ImageProcessor::findBrightestColor(const std::vector<cv::Vec3b>& colors)
{
    // 가장 밝은 색상을 저장할 변수 및 초기화
    cv::Vec3b brightestColor = colors[0];

    // 가장 밝은 색상의 밝기 값 계산 (B + G + R)
    int maxBrightness = brightestColor[0] + brightestColor[1] + brightestColor[2];

    // 모든 색상을 반복하면서 가장 밝은 색상 찾기
    for (const cv::Vec3b& color : colors)
    {
        // 현재 색상의 밝기 계산
        int brightness = color[0] + color[1] + color[2];

        // 가장 밝은 색상보다 더 밝으면 업데이트
        if (brightness > maxBrightness) 
        {
            maxBrightness = brightness;
            brightestColor = color;
        }
    }

    // 가장 밝은 색상 반환
    return brightestColor;
}

// =============================================================================
cv::Vec3b ImageProcessor::findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor)
{
    // 가장 가까운 색상을 저장할 변수 및 초기화
    cv::Vec3b closestColor = colorPalette[0];

    // 최소 거리 초기화
    int minDistance = (int)cv::norm(targetColor - colorPalette[0]);

    // 모든 팔레트 색상을 반복하면서 가장 가까운 색상 찾기
    for (const cv::Vec3b& color : colorPalette)
    {
        // 현재 색상과 대상 색상 사이의 거리 계산
        int distance = (int)cv::norm(targetColor - color);

        // 현재까지의 최소 거리보다 더 가까우면 업데이트
        if (distance < minDistance) {
            minDistance = distance;
            closestColor = color;
        }
    }

    // 가장 가까운 색상 반환
    return closestColor;
}

// =============================================================================
void ImageProcessor::SetPixelFormatParametertoGUI()
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CString strLog = _T("");

    // YUVY
    if (GetYUVYType())
    {
        strLog.Format(_T("[Camera[%d]] YUV422_8_UYVY Mode"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        MainDlg->m_chUYVYCheckBox.SetCheck(TRUE);

        Set16BitType(true); //8비트 고정
        MainDlg->m_chEventsCheckBox.EnableWindow(FALSE); //8비트 고정
        MainDlg->m_chColorMapCheckBox.EnableWindow(FALSE);
        MainDlg->m_chMonoCheckBox.EnableWindow(FALSE);

        strLog.Format(_T("[Camera[%d]] UYVY CheckBox Checked"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
        strLog.Format(_T("[Camera[%d]] 16Bit CheckBox Checked, Disable Windows"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("[Camera[%d]] ColorMap  Disable Windows"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("[Camera[%d]] Mono  Disable Windows"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    // mono 8
    else if (GetGrayType() && Get16BitType() == FALSE)
    {
        SetYUVYType(FALSE);
        MainDlg->m_chEventsCheckBox.SetCheck(FALSE);

        MainDlg->m_chColorMapCheckBox.EnableWindow(FALSE);
        MainDlg->m_chMonoCheckBox.EnableWindow(FALSE);

    }
    // mono 16
    else if (GetGrayType() && Get16BitType())
    {
        SetYUVYType(FALSE);

        MainDlg->m_chMonoCheckBox.SetCheck(TRUE);
        MainDlg->m_chEventsCheckBox.SetCheck(TRUE);

        MainDlg->m_chColorMapCheckBox.EnableWindow(TRUE);
    }
    // rgb 8
    else if (GetRGBType())
    {

    }
}

// =============================================================================
// 컬러맵 정보를 맴버변수에 저장
void ImageProcessor::SetPaletteType(PaletteTypes type)
{
    m_colormapType = type;
}

// =============================================================================
PaletteTypes ImageProcessor::GetPaletteType()
{
    return m_colormapType;
}

// =============================================================================
//이미지 정규화
template <typename T>
cv::Mat ImageProcessor::NormalizeAndProcessImage(const T* data, int height, int width, int cvType)
{
    if (data == nullptr)
    {
        // data가 nullptr일 경우 예외처리 또는 기본값 설정을 수행할 수 있습니다.
        // 여기서는 기본값으로 빈 이미지를 반환합니다.
        return cv::Mat();
    }

    cv::Mat imageMat(height, width, cvType, (void*)data);
    constexpr int nTemp = std::numeric_limits<T>::max();
    cv::normalize(imageMat, imageMat, 0, std::numeric_limits<T>::max(), cv::NORM_MINMAX);
    double min_val = 0, max_val = 0;
    cv::minMaxLoc(imageMat, &min_val, &max_val);

    int range = static_cast<int>(max_val) - static_cast<int>(min_val);
    if (range == 0)
        range = 1;

    //imageMat.convertTo(imageMat, cvType, std::numeric_limits<T>::max() / range, -(min_val)* std::numeric_limits<T>::max() / range);

    return imageMat;
}

// =============================================================================
// ROI 객체 그리기 함수
void ImageProcessor::DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels, const ROIResults& roiResult)
{
    cv::Scalar roiColor(51, 255, 51); // 흰색 (BGR 순서로 지정)
    // ROI 사각형 그리기
    cv::rectangle(imageMat, roiRect, roiColor, 2);

}

// =============================================================================
// 최소 값 텍스트와 마커를 그리는 헬퍼 함수
void ImageProcessor::DrawMinMarkerAndText(cv::Mat& imageMat, const MDSMeasureMinSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize)
{
    if (imageMat.empty()) 
        return;
    cv::Scalar TextColor(255, 255, 255); // 흰색 (BGR 순서로 지정)
    cv::Point center(spot.x, spot.y);
    cv::Point centerText(spot.x, spot.y);
    std::string text = label + " [" + std::to_string(static_cast<int>(spot.tempValue)) + "]";
    if (center.x >= 0 && center.x < imageMat.cols && center.y >= 0 && center.y < imageMat.rows)
    {
        // 텍스트 그리기
        cv::putText(imageMat, text, centerText, cv::FONT_HERSHEY_PLAIN, 1.5, TextColor, 1, cv::LINE_AA);
        // 마커 그리기
        cv::drawMarker(imageMat, center, color, cv::MARKER_TRIANGLE_DOWN, markerSize, 2);
    }
}

// =============================================================================
// 최대 값 텍스트와 마커를 그리는 헬퍼 함수
void ImageProcessor::DrawMaxMarkerAndText(cv::Mat& imageMat, const MDSMeasureMaxSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize)
{
    if (imageMat.empty())
        return;

    cv::Scalar TextColor(255, 255, 255); // 흰색 (BGR 순서로 지정)
    cv::Point center(spot.x, spot.y);
    std::string text = label + " [" + std::to_string(static_cast<int>(spot.tempValue)) + "]";

    if (center.x >= 0 && center.x < imageMat.cols && center.y >= 0 && center.y < imageMat.rows)
    {
        // 텍스트 그리기
        cv::putText(imageMat, text, center, cv::FONT_HERSHEY_PLAIN, 1.5, TextColor, 1, cv::LINE_AA);
        // 마커 그리기
        cv::drawMarker(imageMat, center, color, cv::MARKER_TRIANGLE_UP, markerSize, 2);
    }
}

// =============================================================================
void ImageProcessor::DrawAllRoiRectangles(cv::Mat& image) 
{
    int num_channels = image.channels();

    std::lock_guard<std::mutex> roiDrawLock(roiDrawMtx);

    // 최종 ROI를 그립니다.
    {
        std::lock_guard<std::mutex> roiLock(m_roiMutex);  // 확정된 ROI 데이터를 보호하기 위해 뮤텍스를 사용합니다.
        cv::Scalar MinColor(255, 0, 0); // 흰색 (BGR 순서로 지정)
        cv::Scalar MaxColor(0, 0, 255); // 흰색 (BGR 순서로 지정)
        int markerSize = 20; // 마커의 크기 설정

        for (const auto& roiResult : m_roiResults)
        {
            if (roiResult->needsRedraw)
            {              
                DrawRoiShape(image, *roiResult, num_channels, roiResult->shapeType);
                roiResult->needsRedraw = false;
            }

            if (roiResult->minSpot.updated || roiResult->maxSpot.updated)
            {
                DrawMinMarkerAndText(image, roiResult->minSpot, "Min", MinColor, markerSize);
                DrawMaxMarkerAndText(image, roiResult->maxSpot, "Max", MaxColor, markerSize);

                roiResult->minSpot.updated = false;
                roiResult->maxSpot.updated = false;
            }           
        }    
    }

    // 임시 ROI를 그립니다.
    {
        std::lock_guard<std::mutex> tempLock(m_temporaryRoiMutex);  // 임시 ROI 데이터를 보호하기 위해 뮤텍스를 사용합니다.
        for (const auto& tempRoi : m_temporaryRois)
        {
            DrawRoiShape(image, *tempRoi, num_channels, tempRoi->shapeType);
        }
    }
}

// =============================================================================
//이미지 데이터에 따라 파레트 설정
cv::Mat ImageProcessor::MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette)
{
    // Input 이미지는 CV_16UC1 이다
    cv::Mat normalizedImage(inputImage.size(), CV_16UC1); // 정규화된 16비트 이미지

    // input 이미지 내에서 최소값과 최대값 검색.
    double minVal, maxVal;
    cv::minMaxLoc(inputImage, &minVal, &maxVal);

    // [0, 65535] 범위로 정규화
    inputImage.convertTo(normalizedImage, CV_16UC1, 65535.0 / (maxVal - minVal), -65535.0 * minVal / (maxVal - minVal));

    // 컬러 맵 적용은 일반적으로 8비트 이미지에 사용되므로,
    // 여기에서는 정규화된 이미지를 임시로 8비트로 변환하여 컬러 맵을 적용.
    cv::Mat normalizedImage8bit;
    normalizedImage.convertTo(normalizedImage8bit, CV_8UC1, 255.0 / 65535.0);



    cv::Mat normalizedImage3channel(inputImage.size(), CV_8UC3);
    cv::cvtColor(normalizedImage8bit, normalizedImage3channel, cv::COLOR_GRAY2BGR);
    // 메모리 해제
    normalizedImage8bit.release();

    cv::Mat smoothedImage, claheImage;

    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    cv::Mat colorMappedImage;

    //CString strValue, strValue2, strValue3;
    //MainDlg->m_Color_Scale.GetWindowText(strValue);
    //MainDlg->m_Color_Scale2.GetWindowText(strValue2);
    //MainDlg->m_Color_Scale3.GetWindowText(strValue3);

    double dScaleR = 1;
    double dScaleG = 1;
    double dScaleB = 1;

    //if ((PaletteTypes)palette == PALETTE_TYPE::PALETTE_IRON ||
    //    (PaletteTypes)palette == PALETTE_TYPE::PALETTE_INFER ||
    //    (PaletteTypes)palette == PALETTE_TYPE::PALETTE_PLASMA ||
    //    (PaletteTypes)palette == PALETTE_TYPE::PALETTE_MAGMA ||
    //    (PaletteTypes)palette == PALETTE_TYPE::PALETTE_SUMMER)
    //{
    //    dScaleG = 1;
    //}
    //else if((PaletteTypes)palette == PALETTE_TYPE::PALETTE_RED_GRAY)
    //{
    //    dScaleR = 1;
    //}

    // 아이언 1/2/1
    colorMappedImage = applyIronColorMap(normalizedImage3channel, palette, dScaleR, dScaleG, dScaleB);



    // 메모리 해제
    normalizedImage3channel.release();

    //cv::applyColorMap(colorMappedImage, colorMappedImage, colormap);
    //cv::GaussianBlur(colorMappedImage, smoothedImage, cv::Size(5, 5), 0, 0);

    return colorMappedImage;

}

// =============================================================================
cv::Mat ImageProcessor::ConvertUYVYToBGR8(const cv::Mat& input)
{
    if (input.empty() || input.channels() != 2) {
        std::cerr << "Input Mat is empty or not a 2-channel image." << std::endl;
        return cv::Mat();
    }

    int width = input.cols;
    int height = input.rows;

    // UYVY 형식을 다시 Mat으로 변환 (CV_8UC2 -> CV_8UC4)
    cv::Mat uyvyImage(height, width, CV_8UC2, input.data);
    cv::Mat bgrImage(height, width, CV_8UC3);  // BGR 형식으로 결과 이미지 생성

    // UYVY에서 BGR로 변환
    cv::cvtColor(uyvyImage, bgrImage, cv::COLOR_YUV2BGR_UYVY);

    return bgrImage;
}

// =============================================================================
cv::Mat ImageProcessor::Convert16UC1ToBGR8(const cv::Mat& input)
{
    if (input.empty() || input.type() != CV_16UC1) {
        std::cerr << "Input Mat is empty or not a 16-bit single-channel image." << std::endl;
        return cv::Mat();
    }

    int width = input.cols;
    int height = input.rows;

    cv::Mat bgrImage(height, width, CV_8UC3);  // BGR 형식으로 결과 이미지 생성

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            ushort pixelValue = input.at<ushort>(y, x);
            bgrImage.at<cv::Vec3b>(y, x) = cv::Vec3b(pixelValue & 0xFF, (pixelValue >> 8) & 0xFF, (pixelValue >> 8) & 0xFF);
        }
    }

    return bgrImage;
}

// =============================================================================
std::unique_ptr<uint16_t[]> ImageProcessor::extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight)
{
    // 전체 영역의 크기 계산
    int imageSize = nWidth * nHeight;

    // imageSize 크기의 uint16_t 배열을 동적으로 할당합니다.
    std::unique_ptr<uint16_t[]> imageArray = std::make_unique<uint16_t[]>(imageSize);
    for (int i = 0; i < imageSize; ++i)
    {
        uint16_t nRawdata = static_cast<uint16_t>(byteArr[i * 2] | (byteArr[i * 2 + 1] << 8));
        if (m_CameraControl->GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
            imageArray[i] = (nRawdata * GetScaleFactor()) - FAHRENHEIT;
        else
            imageArray[i] = nRawdata;

    }

    CString strLog;
    OutputDebugString(strLog);

    return imageArray; // std::unique_ptr 반환
}

// =============================================================================
// 지정된 영역의 데이터 처리
std::unique_ptr<uint16_t[]> ImageProcessor::extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight)
{
    int nArraySize = roiWidth * roiHeight;
    std::unique_ptr<uint16_t[]> roiArray(new uint16_t[nArraySize]);

    int k = 0;
    for (int y = nStartY; y < nEndY; y++)
    {
        int rowOffset = y * nWidth;
        for (int x = nStartX; x < nEndX; x++)
        {
            int index = rowOffset + x;
            roiArray[k++] = static_cast<uint16_t>(byteArr[index * 2] + (byteArr[index * 2 + 1] << 8));
        }
    }
    return roiArray;
}

// =============================================================================
// ROI 영역의 이미지 데이터 추출
std::unique_ptr<uint16_t[]> ImageProcessor::extractImageData(const cv::Mat& roiMat)
{
    size_t dataSize = roiMat.total();
    auto data = std::make_unique<uint16_t[]>(dataSize);
    memcpy(data.get(), roiMat.data, dataSize * sizeof(uint16_t));
    return data;
}

// =============================================================================
// MinMax 변수 초기화
bool ImageProcessor::InitializeTemperatureThresholds()
{
    return true;
}

// =============================================================================
// 단일 ROI 결과 초기화 함수
void ImageProcessor::InitializeSingleRoiResult(ROIResults& roiResult)
{
    roiResult.maxSpot = MDSMeasureMaxSpotValue();
    roiResult.minSpot = MDSMeasureMinSpotValue();


    roiResult.span = 0;
    roiResult.level = 0;

    roiResult.min_x = roiResult.roi.x;
    roiResult.min_y = roiResult.roi.y;
    roiResult.max_x = roiResult.roi.x + roiResult.roi.width;
    roiResult.max_y = roiResult.roi.y + roiResult.roi.height;

    roiResult.min_temp = 65535;
    roiResult.max_temp = 0;
}

// =============================================================================
// 카메라 종류별 스케일값 적용
double ImageProcessor::GetScaleFactor()
{
    double dScale = 0;
    if (GetCam()->GetIRFormat() == Camera_IRFormat::TEMPERATURELINEAR10MK)
    {
        switch (GetCam()->m_Camlist)
        {
        case CAM_MODEL::FT1000:
        case CAM_MODEL::XSC:
        case CAM_MODEL::A300:
        case CAM_MODEL::A400:
        case CAM_MODEL::A500:
        case CAM_MODEL::A700:
        case CAM_MODEL::A615:
        case CAM_MODEL::A50:
            dScale = (0.01f);
            break;
        case CAM_MODEL::Ax5:
            dScale = (0.04f);
            break;
        case CAM_MODEL::BlackFly:
            dScale = 0;
            break;
        default:
            dScale = (0.01f);
            break;
        }
    }
    else if (GetCam()->GetIRFormat() == Camera_IRFormat::TEMPERATURELINEAR100MK)
    {
        switch (GetCam()->m_Camlist)
        {
        case CAM_MODEL::FT1000:
        case CAM_MODEL::XSC:
        case CAM_MODEL::A300:
        case CAM_MODEL::A400:
        case CAM_MODEL::A500:
        case CAM_MODEL::A700:
        case CAM_MODEL::A615:
        case CAM_MODEL::A50:
            dScale = (0.1f);
            break;
        case CAM_MODEL::Ax5:
            dScale = (0.4f);
            break;
        case CAM_MODEL::BlackFly:
            dScale = 0;
            break;
        default:
            dScale = (0.1f);
            break;
        }
    }

    return dScale;
}

// =============================================================================
// ROI 내부의 데이터 연산
void ImageProcessor::ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx, ROIResults& roiResult)
{
    int absoluteX = nCurrentX + roi.x;
    int absoluteY = nCurrentY + roi.y;
    CTemperature T;

    // 최소 온도 체크 및 업데이트
    if (uTempValue <= roiResult.min_temp)
    {
        roiResult.min_temp = uTempValue;
        roiResult.min_x = absoluteX;
        roiResult.min_y = absoluteY;
        roiResult.minSpot.x = roiResult.min_x;  // 스팟의 X 좌표 업데이트
        roiResult.minSpot.y = roiResult.min_y;  // 스팟의 Y 좌표 업데이트
        roiResult.minSpot.pointIdx = nPointIdx; // 포인트 인덱스 저장
        roiResult.minSpot.updated = true;

        if (GetCam()->GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
            roiResult.minSpot.tempValue = static_cast<float>(uTempValue) * dScale - FAHRENHEIT;
        else
        {
            T = GetCam()->imgToTemp(uTempValue);
            roiResult.minSpot.tempValue = T.Value(CTemperature::Celsius);

            
        }
    }
    // 최대 온도 체크 및 업데이트
    if (uTempValue >= roiResult.max_temp)
    {
        roiResult.max_temp = uTempValue;
        roiResult.max_x = absoluteX;
        roiResult.max_y = absoluteY;
        roiResult.maxSpot.x = roiResult.max_x;  // 스팟의 X 좌표 업데이트
        roiResult.maxSpot.y = roiResult.max_y;  // 스팟의 Y 좌표 업데이트
        roiResult.maxSpot.pointIdx = nPointIdx; // 포인트 인덱스 저장
        roiResult.maxSpot.updated = true;

        if (GetCam()->GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
            roiResult.maxSpot.tempValue = static_cast<float>(uTempValue) * dScale - FAHRENHEIT;
        else
        {
            T = GetCam()->imgToTemp(uTempValue);
            roiResult.maxSpot.tempValue = T.Value(CTemperature::Celsius);
        }
    }

    // 스팬 및 레벨 계산
    roiResult.span = roiResult.max_temp - roiResult.min_temp;
    roiResult.level = (roiResult.max_temp + roiResult.min_temp) / 2.0;
}



// =============================================================================
// 16비트 데이터 최소,최대값 산출
void ImageProcessor::ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, std::unique_ptr<ROIResults>& roiResult, double dScale)
{
    int nPointIdx = 0; // 포인트 인덱스 초기화

    for (int i = 0; i < size; i++) 
    {
        ushort tempValue = data[i];
        int x = i % roiResult->roi.width;
        int y = i / roiResult->roi.width;
        ROIXYinBox(tempValue, dScale, x, y, roiResult->roi, nPointIdx, *roiResult);
        nPointIdx++;
    }
}

// =============================================================================
void ImageProcessor::SetYUVYType(bool bFlag)
{
    m_bYUVYFlag = bFlag;
}

// =============================================================================
bool ImageProcessor::GetYUVYType()
{
    return m_bYUVYFlag;
}

// =============================================================================
void ImageProcessor::SetRGBType(bool bFlag)
{
    m_bRGBFlag = bFlag;
}

// =============================================================================
bool ImageProcessor::GetRGBType()
{
    return m_bRGBFlag;
}

// =============================================================================
bool ImageProcessor::GetGrayType()
{
    return m_bGrayFlag;
}

// =============================================================================
void ImageProcessor::SetGrayType(bool bFlag)
{
    m_bGrayFlag = bFlag;
}

// =============================================================================
bool ImageProcessor::Get16BitType()
{
    return m_b16BitFlag;
}

// =============================================================================
void ImageProcessor::Set16BitType(bool bFlag)
{
    m_b16BitFlag = bFlag;
}

// =============================================================================
bool ImageProcessor::GetColorPaletteType()
{
    return m_bColorFlag;
}

// =============================================================================
void ImageProcessor::SetColorPaletteType(bool bFlag)
{
    m_bColorFlag = bFlag;
}

// =============================================================================
void ImageProcessor::SetRawdataPath(std::string path)
{
    m_strRawdataPath = path;
}

// =============================================================================
std::string ImageProcessor::GetRawdataPath()
{
    return m_strRawdataPath;
}

// =============================================================================
std::string ImageProcessor::GetImageSavePath()
{
    return GetRawdataPath() + "Image\\";
}

// =============================================================================
std::string ImageProcessor::GetRawSavePath()
{
    return GetRawdataPath() + "raw\\";
}

// =============================================================================
std::string ImageProcessor::GetRecordingPath()
{
    return GetRawdataPath() + "recording\\";
}

// =============================================================================
// BITMAPINFO, BITMAPINFOHEADER  setting
BITMAPINFO* ImageProcessor::CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channel)
{

    std::lock_guard<std::mutex> lock(m_bitmapMutex);

    BITMAPINFO* bitmapInfo = nullptr;
    size_t bitmapInfoSize = 0;
    int imageSize = 0;
    int bpp = 0;
    int nbiClrUsed = 0;
    int nType = imageMat.type();

    try
    {
        bpp = (int)imageMat.elemSize() * 8;

        if (GetYUVYType() || GetColorPaletteType())
        {
            bpp = 24;
            imageSize = w * h * 3;
            bitmapInfoSize = sizeof(BITMAPINFOHEADER) + imageSize;
            bitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[bitmapInfoSize]);
            nbiClrUsed = 0;
        }
        else if (GetGrayType())
        {
            bitmapInfoSize = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
            bitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[bitmapInfoSize]);
            imageSize = w * h;
            nbiClrUsed = 256;
            memcpy(bitmapInfo->bmiColors, GrayPalette, 256 * sizeof(RGBQUAD));
        }
        else
        {
            bpp = 24;
            imageSize = w * h * 3;
            bitmapInfoSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[bitmapInfoSize]);
            nbiClrUsed = 0;
        }

        if (!bitmapInfo) {
            throw std::runtime_error("Failed to allocate memory for BITMAPINFO");
        }

        BITMAPINFOHEADER& bmiHeader = bitmapInfo->bmiHeader;

        bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiHeader.biWidth = w;
        bmiHeader.biHeight = -h;  // 이미지를 아래에서 위로 표시
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = bpp;
        bmiHeader.biCompression = BI_RGB;
        bmiHeader.biSizeImage = imageSize; // 이미지 크기 초기화
        bmiHeader.biClrUsed = nbiClrUsed;   // 색상 팔레트 적용

        // 초기화 상태 출력
        std::cout << "BitmapInfo initialized: biSize = " << bmiHeader.biSize << std::endl;

        return bitmapInfo;
    }
    catch (const std::bad_alloc&)
    {
        delete[] reinterpret_cast<BYTE*>(bitmapInfo); // 메모리 해제
        bitmapInfo = nullptr;
        throw std::runtime_error("Failed to allocate memory for BITMAPINFO");
    }
}

bool ImageProcessor::IsValidBitmapInfo(const BITMAPINFO* bitmapInfo) 
{
    if (!bitmapInfo) 
    {
        return false;
    }
    const BITMAPINFOHEADER& bmiHeader = bitmapInfo->bmiHeader;
    if (bmiHeader.biSize != sizeof(BITMAPINFOHEADER)) 
    {
        return false;
    }
    if (bmiHeader.biWidth <= 0 || bmiHeader.biHeight == 0)
    {
        return false;
    }
    if (bmiHeader.biPlanes != 1) 
    {
        return false;
    }
    //if (bmiHeader.biBitCount != 24 && bmiHeader.biBitCount != 32) 
    //{
    //    return false;
    //}
    if (bmiHeader.biCompression != BI_RGB)
    {
        return false;
    }
    return true;
}

// =============================================================================
// description : Mat data to BITMAPINFO
// 라이브 이미지 깜박임을 최소화하기 위하여 더블 버퍼링 기법 사용
void ImageProcessor::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
{
    // 입력 이미지 또는 BITMAPINFO가 유효하지 않은 경우 함수를 종료합니다.
    if (mImage.empty() || BitmapInfo == nullptr) {
        CString errorLog;
        errorLog.Format(_T("Invalid input data. Image empty: %d, BitmapInfo null: %d"), mImage.empty(), BitmapInfo == nullptr);
        Common::GetInstance()->AddLog(1, errorLog);
        return;
    }

    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    if (!MainDlg) {
        Common::GetInstance()->AddLog(1, _T("Main dialog pointer is null."));
        return;
    }

    CClientDC dc(MainDlg->GetDlgItem(nID));
    if (!dc) {
        Common::GetInstance()->AddLog(1, _T("Failed to get device context."));
        return;
    }

    CRect rect;
    MainDlg->GetDlgItem(nID)->GetClientRect(&rect);

    // 복사를 위해 이미지 복제
    cv::Mat imageCopy;
    {
        std::lock_guard<std::mutex> lock(drawmtx);
        mImage.copyTo(imageCopy);
    }

    // 더블 버퍼링을 위한 준비
    CDC backBufferDC;
    CBitmap backBufferBitmap;
    if (!backBufferDC.CreateCompatibleDC(&dc)) {
        Common::GetInstance()->AddLog(1, _T("Failed to create compatible DC."));
        return;
    }
    if (!backBufferBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height())) {
        backBufferDC.DeleteDC();
        Common::GetInstance()->AddLog(1, _T("Failed to create compatible bitmap."));
        return;
    }

    CBitmap* pOldBackBufferBitmap = backBufferDC.SelectObject(&backBufferBitmap);
    if (!pOldBackBufferBitmap) {
        backBufferBitmap.DeleteObject();
        backBufferDC.DeleteDC();
        Common::GetInstance()->AddLog(1, _T("Failed to select bitmap object into DC."));
        return;
    }

    // 이미지 스트레칭
    backBufferDC.SetStretchBltMode(COLORONCOLOR);
    if (GDI_ERROR == StretchDIBits(backBufferDC.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, imageCopy.cols, imageCopy.rows, imageCopy.data, BitmapInfo, DIB_RGB_COLORS, SRCCOPY)) {
        Common::GetInstance()->AddLog(1, _T("StretchDIBits failed."));
    }

    // 더블 버퍼링을 사용하여 화면에 그리기
    if (!dc.BitBlt(0, 0, rect.Width(), rect.Height(), &backBufferDC, 0, 0, SRCCOPY)) {
        Common::GetInstance()->AddLog(1, _T("BitBlt failed."));
    }

    // 정리
    backBufferDC.SelectObject(pOldBackBufferBitmap);
    backBufferBitmap.DeleteObject();
    backBufferDC.DeleteDC();
}

// =============================================================================
//화면에 출력한 비트맵 데이터를 생성
void ImageProcessor::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex)
{
    // 이전에 할당된 메모리 해제
    CleanupAfterProcessing();

    // 비트맵 정보 생성
    m_BitmapInfo = CreateBitmapInfo(imageMat, imageMat.cols, imageMat.rows, num_channels);

    const int CAM_IDS[] = { IDC_CAM1, IDC_CAM2, IDC_CAM3, IDC_CAM4 };
    for (int i = 0; i < sizeof(CAM_IDS) / sizeof(CAM_IDS[0]); i++)
    {
        if (nIndex == i)
        {
            DrawImage(imageMat, CAM_IDS[i], m_BitmapInfo);
            break;
        }
    }
}

// =============================================================================
// mat data *.csv format save
bool ImageProcessor::WriteCSV(string strPath, Mat mData)
{
    ofstream myfile;
    myfile.open(strPath.c_str());
    myfile << cv::format(mData, cv::Formatter::FMT_CSV) << std::endl;
    myfile.close();

    return true;
}

// =============================================================================
bool ImageProcessor::StartSEQRecording(CString strfilePath)
{

    if (m_SEQFile.m_hFile != CFile::hFileNull) 
    {
        m_SEQFile.Close();  // 이미 열려 있는 파일이 있다면 닫기
    }

    m_SEQfilePath = CT2A(strfilePath);  // 파일 경로 저장
    CFileException e;
    // Try to create new file
    // 파일 생성 시도
    // 파일 열기 시도
    try 
    {
        m_SEQFile.Open(strfilePath, CFile::modeCreate | CFile::modeReadWrite | CFile::typeBinary);
    }
    catch (CFileException* e)
    {
        std::cerr << "Failed to open file: " << e->m_cause << "\n";
        e->Delete();
        m_bSaveAsSEQ = false;
        return false;
    }
    return true;
}



//=============================================================================
// Trig flag bits - description
// Bit 15: 1=This trig info is relevant 
//         0=Look for trig info in pixel data instead
// Bit 5:  1=Stop trig type
// Bit 4:  1=Start trig type
// Bit 3:  0=Device 1=Serial port trig (or LPT)
// Bit 2:  0=TTL       1=OPTO 
// Bit 1:  0=Negative  1=Positive
// Bit 0:  0=No trig   1=Trigged
//=============================================================================
void ImageProcessor::SaveSEQ(int nWidth, int nHeight)
{
    auto currentTime = std::chrono::system_clock::now();
    bool bFileCreateFlag = false;

    // 헤더가 포함된 데이터인지 유효성 체크
    PvBuffer* pBuf = nullptr;
    pBuf = GetCam()->GetBufferFromQueue();
    int Height = 0;
    PvImage* lImage = pBuf->GetImage();
    Height = lImage->GetHeight();

    // 헤더 데이터가 포함되지 않으면 바로 리턴한다
    if (Height == 480 || Height == 348)
    {
        StopRecording();
        return;
    }
        
    // 로직 첫 진입 시 파일 만들기 
    if (m_iCurrentSEQImage == 0)
    {
        std::time_t nowAsTimeT = std::chrono::system_clock::to_time_t(currentTime);
        std::tm localTime;
        localtime_s(&localTime, &nowAsTimeT);
        char timeStr[50];
        strftime(timeStr, sizeof(timeStr), "_%Y_%m_%d_%H_%M_%S", &localTime);
        std::string strSeqName = GetRecordingPath() + "SEQ" + std::string(timeStr) + ".seq";
        CString strSeqFileName = Common::GetInstance()->str2CString(strSeqName.c_str());

        bFileCreateFlag = StartSEQRecording(strSeqFileName);
        if (!bFileCreateFlag)
            return;
    }

    // buffer to SEQ 로직
    int nPixelSize = nHeight * nWidth * 2;
    uint8_t* pSrc = nullptr;
    pSrc = (uint8_t*)pBuf->GetDataPointer();

    if (!pSrc || *pSrc == 0) return;

    if (m_bSaveAsSEQ)
    {
        FPGA_HEADER* pFPGA;
        FLIRFILEHEAD* pHeader;
        FLIRFILEINDEX* pIndex;
        pFPGA = (FPGA_HEADER*)&pSrc[nPixelSize];
        WORD trgflags = 0;
        GEOMETRIC_INFO_T geom;
        DWORD dwNumUsedIndex = 0;

        memset(&geom, 0, sizeof(GEOMETRIC_INFO_T));

        geom.firstValidX = 0;
        geom.firstValidY = 0;
        geom.lastValidX = nWidth - 1;
        geom.lastValidY = nHeight - 1;
        geom.imageHeight = nHeight;
        geom.imageWidth = nWidth;
        geom.pixelSize = 2;

        pHeader = (FLIRFILEHEAD*)&pSrc[nPixelSize + sizeof(FPGA_HEADER)];
        pIndex = (FLIRFILEINDEX*)&pSrc[nPixelSize + sizeof(FPGA_HEADER) + sizeof(FLIRFILEHEAD)];
        dwNumUsedIndex = pHeader->dwNumUsedIndex;

        ULONG offs = sizeof(FLIRFILEHEAD) + (sizeof(FLIRFILEINDEX) * dwNumUsedIndex);

        for (int i = 0; i < (int)dwNumUsedIndex; i++)
        {
            pIndex[i].dwChecksum = 0;
            if (pIndex[i].wMainType == FFF_TAGID_Pixels)
            {
                pIndex[i].dwDataSize = nPixelSize + sizeof(GEOMETRIC_INFO_T);
            }
            pIndex[i].dwDataPtr = offs;
            offs += pIndex[i].dwDataSize;
        }
        if (pFPGA == nullptr || pHeader == nullptr || pIndex == nullptr) {
            Common::GetInstance()->AddLog(0, _T("데이터 포인터 오류 발생"));
            return;
        }

        try
        {
            if (!pHeader || !pIndex) {
                Common::GetInstance()->AddLog(0, _T("pHeader, pIndex NULL"));
                return;
            }

            // Write to file
            if (m_SEQFile.m_hFile != CFile::hFileNull)  // 파일이 열려 있는지 확인
            {
                m_SEQFile.SeekToEnd(); // Ensure the file pointer is at the correct position before writing
                m_SEQFile.Write(pHeader, sizeof(FLIRFILEHEAD));
                m_SEQFile.Write(pIndex, sizeof(FLIRFILEINDEX) * dwNumUsedIndex);
            }

            for (int i = 0; i < (int)dwNumUsedIndex; i++)
            {         
                if (pIndex[i].wMainType == FFF_TAGID_BasicData)
                {
                    PBYTE pData;
                    BI_DATA_T* pBI;

                    pData = (PBYTE)&pIndex[dwNumUsedIndex];
                    pBI = (BI_DATA_T*)pData;

                    if (!pData) {
                        Common::GetInstance()->AddLog(0, _T("pData null"));
                        return;
                    }

                    pBI->GeometricInfo.pixelSize = 2;
                    pBI->GeometricInfo.imageHeight = nHeight;
                    pBI->GeometricInfo.imageWidth = nWidth;
                    pBI->ImageInfo.imageTime = (unsigned long)m_ts;
                    pBI->ImageInfo.imageMilliTime = m_ms;
                    pBI->ImageInfo.timeZoneBias = m_tzBias;

                    pBI->PresentParameters.level = m_Level;
                    pBI->PresentParameters.span = m_Span;

                    // Transfer trig info - if any
                    if ((pFPGA->dp1_trig_type | pFPGA->dp2_trig_type) &
                        (FPGA_TRIG_TYPE_MARK |
                            FPGA_TRIG_TYPE_MARK_START |
                            FPGA_TRIG_TYPE_MARK_STOP)
                        )
                    {

                        trgflags = 0x8000; // Trig info is relevant 
                        if ((pFPGA->dp1_trig_type | pFPGA->dp2_trig_type) & FPGA_TRIG_TYPE_MARK)
                            trgflags |= 0x0001; // Normal trig mark
                        if ((pFPGA->dp1_trig_type | pFPGA->dp2_trig_type) & FPGA_TRIG_TYPE_MARK_START)
                            trgflags |= 0x0010; // Start trig type
                        if ((pFPGA->dp1_trig_type | pFPGA->dp2_trig_type) & FPGA_TRIG_TYPE_MARK_STOP)
                            trgflags |= 0x0020; // Stop trig type
                    }

                    pBI->ImageInfo.trigFlags = trgflags;
                    pBI->ImageInfo.trigCount = m_TrigCount;
                    pBI->ImageInfo.trigHit = 0;
                    pBI->ImageInfo.trigInfoType = 1;

                    // Save a copy of geometric data
                    memcpy(&geom, pData, sizeof(GEOMETRIC_INFO_T));

                    if (m_SEQFile.m_hFile != CFile::hFileNull)  // 파일이 열려 있는지 확인
                    {
                        // Write the modified Basic Image data block to file
                        m_SEQFile.Write(pData, pIndex[i].dwDataSize);
                    }
                }
            }
            if (m_SEQFile.m_hFile != CFile::hFileNull)  // 파일이 열려 있는지 확인
            {
                m_SEQFile.Write(&geom, sizeof(GEOMETRIC_INFO_T));
                m_SEQFile.Write(pSrc, nPixelSize);
                m_iCurrentSEQImage++;
            }
        }
        catch (CFileException* e)
        {
            // 예외 발생 시 로그 남기기
            //CString errorMessage;
            //errorMessage.Format(_T("SEQ File Write Fail: %d"), e->m_cause);
            //Common::GetInstance()->AddLog(0, errorMessage);
            return;
        }
    }
}

//=============================================================================
// 팔레트 갯수를 256개로 맞추어 선형보간
std::vector<cv::Vec3b> ImageProcessor::interpolatePalette(const std::vector<cv::Vec3b>& palette, int target_size) 
{
    std::vector<cv::Vec3b> interpolated_palette(target_size);
    int original_size = static_cast<int>(palette.size());

    for (int i = 0; i < target_size; ++i) {
        float position = static_cast<float>(i) * (original_size - 1) / (target_size - 1);
        int lower_index = static_cast<int>(std::floor(position));
        int upper_index = static_cast<int>(std::ceil(position));
        float weight = position - lower_index;

        for (int j = 0; j < 3; ++j) {
            interpolated_palette[i][j] = static_cast<uchar>(
                (1 - weight) * palette[lower_index][j] + weight * palette[upper_index][j]);
        }
    }

    return interpolated_palette;
}

//=============================================================================
// 작업필요
void ImageProcessor::DetectAndDisplayPeople(cv::Mat& image, int& personCount, double& detectionRate)
{
    cv::CascadeClassifier fullbody_cascade;

    // 실행 파일 경로 가져오기
    CString programDir = Common::GetInstance()->GetProgramDirectory();
    CString cascadePath = programDir + _T("\\haarcascade_fullbody.xml");

    // CString을 std::string으로 변환
    CT2CA pszConvertedAnsiString(cascadePath);
    std::string stdCascadePath(pszConvertedAnsiString);

    std::cout << "Trying to load: " << stdCascadePath << std::endl;


    if (!fullbody_cascade.load(stdCascadePath))
    {
        std::cerr << "Error loading " << cascadePath << std::endl;
        return;
    }

    //cv::Mat gray;
    //cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    //cv::equalizeHist(image, image); // 히스토그램 평활화

    std::vector<cv::Rect> detections;
    fullbody_cascade.detectMultiScale(image, detections);

    personCount = detections.size();
    detectionRate = (double)personCount / (image.rows * image.cols) * 100;

    for (const auto& detection : detections)
    {
        cv::rectangle(image,
            cv::Point(detection.x, detection.y),
            cv::Point(detection.x + detection.width, detection.y + detection.height),
            cv::Scalar(0, 255, 0), 2);
    }
}

//=============================================================================
//작업 필요
void ImageProcessor::DisplayPersonCountAndRate(cv::Mat& image, int personCount, double detectionRate)
{
    std::string text = "Count: " + std::to_string(personCount) + " Rate: " + std::to_string(detectionRate) + "%";
    cv::putText(image, text, cv::Point(0, 200), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
}

//=============================================================================
void ImageProcessor::addROI(const cv::Rect& roi)
{
    //std::lock_guard<std::mutex> lock(m_roiMutex);  // m_roiResults에 대한 변경을 동기화
    //auto roiResult = std::make_shared<ROIResults>();
    //roiResult->roi = roi;

    //roiResult->nIndex = m_roiResults.size()+1;  // 인덱스는 추가되는 순서대로 할당
    //m_roiResults.push_back(roiResult);


    //CString strLog;
    //strLog.Format(_T("Added ROI: %d [%d, %d, %d, %d]"), roiResult->nIndex, roiResult->roi.x, roiResult->roi.y, roiResult->roi.width, roiResult->roi.height);
    //Common::GetInstance()->AddLog(0, strLog);
}

//=============================================================================
// ROI를 삭제하는 함수
void ImageProcessor::deleteROI(int index)
{
    std::lock_guard<std::mutex> lock(m_roiMutex);

    auto it = std::find_if(m_roiResults.begin(), m_roiResults.end(), [index](const std::shared_ptr<ROIResults>& roiResult) 
    {
        return roiResult->nIndex == index;
    });

    if (it != m_roiResults.end()) 
    {
        CString strLog;
        strLog.Format(_T("Deleting ROI: %d [%d, %d, %d, %d]"), (*it)->nIndex, (*it)->roi.x, (*it)->roi.y, (*it)->roi.width, (*it)->roi.height);
        Common::GetInstance()->AddLog(0, strLog);

        m_roiResults.erase(it);
    }
    else 
    {
        CString strLog;
        strLog.Format(_T("Failed to find ROI with index: %d"), index);
        Common::GetInstance()->AddLog(0, strLog);
    }
}

//=============================================================================
// Function to delete all ROIs
void ImageProcessor::deleteAllROIs()
{
    std::lock_guard<std::mutex> lock(m_roiMutex); // Lock to ensure thread safety

    if (!m_roiResults.empty())
    {
        CString strLog;
        strLog.Format(_T("Deleting all ROIs, count: %d"), m_roiResults.size());
        Common::GetInstance()->AddLog(0, strLog);

        m_roiResults.clear(); // Clear all stored ROIs
    }
    else
    {
        CString strLog;
        strLog.Format(_T("No ROIs to delete."));
        Common::GetInstance()->AddLog(0, strLog);
    }
}

//=============================================================================
void ImageProcessor::updateROI(const cv::Rect& roi, int nIndex)
{
    std::lock_guard<std::mutex> lock(m_roiMutex);
    if (nIndex >= 0 && nIndex < m_roiResults.size())
    {
        if (m_roiResults[nIndex]->roi != roi) {
            m_roiResults[nIndex]->roi = roi;
            m_roiResults[nIndex]->needsRedraw = true;  // 변경 플래그 설정
        }
    }
}

//=============================================================================
bool ImageProcessor::canAddROI() const 
{
    return m_roiResults.size() < 4;  // 최대 4개의 ROI 허용
}

//=============================================================================
// 임시 ROI 업데이트 함수
void ImageProcessor::temporaryUpdateROI(const cv::Rect& roi, ShapeType shapeType)
{
    std::lock_guard<std::mutex> lock(m_temporaryRoiMutex);  // 동기화를 위해 락을 걸어줍니다.
    if (m_isAddingNewRoi)
    {
        if (m_temporaryRois.empty())
        {
            auto tempRoiResult = std::make_shared<ROIResults>();
            tempRoiResult->roi = roi;
            tempRoiResult->shapeType = shapeType; // 도형 유형 설정
            m_temporaryRois.push_back(tempRoiResult);
        }
        else
        {
            m_temporaryRois.back()->roi = roi;
            m_temporaryRois.back()->shapeType = shapeType; // 기존 임시 ROI의 도형 유형 업데이트
        }
    }
}

//=============================================================================
// 최종 ROI로 확정하는 함수
void ImageProcessor::confirmROI(const cv::Rect& roi, ShapeType shapeType)
{
    {
        std::lock_guard<std::mutex> lock(m_temporaryRoiMutex);
        if (!m_temporaryRois.empty())
        {
            m_temporaryRois.clear();
        }
    }

    std::lock_guard<std::mutex> lock(m_roiMutex);

    auto roiResult = std::make_shared<ROIResults>();
    roiResult->roi = roi;
    roiResult->shapeType = shapeType;
    roiResult->needsRedraw = true;
    // 인덱스는 추가되는 순서대로 할당
    roiResult->nIndex = m_roiResults.size()+1;
    m_roiResults.push_back(roiResult);

    CString strLog;
    strLog.Format(_T("[Camera[%d] Added ROI: %d [%d, %d, %d, %d] Cnt = [%d] "), GetCam()->GetCamIndex() + 1, roiResult->nIndex, roiResult->roi.x, roiResult->roi.y, roiResult->roi.width, roiResult->roi.height, m_roiResults.size());
    Common::GetInstance()->AddLog(0, strLog);
}

//=============================================================================
std::unique_ptr<ROIResults> ImageProcessor::ProcessSingleROI(std::unique_ptr<uint16_t[]>&& data, const cv::Rect& roi, double dScale, int nIndex)
{
    auto roiResult = std::make_unique<ROIResults>();
    roiResult->roi = roi;
    roiResult->nIndex = nIndex;

    InitializeSingleRoiResult(*roiResult);
    ProcessImageData(std::move(data), roi.width * roi.height, roiResult, dScale);

    return roiResult;
}

//=============================================================================
// 이미지 크기 유효성 검사 추가
bool ImageProcessor::IsRoiValid(const cv::Rect& roi, int imageWidth, int imageHeight)
{
    // ROI가 이미지 경계를 벗어나지 않는지 검사
    return roi.x >= 0 && roi.y >= 0 && (roi.x + roi.width) <= imageWidth && (roi.y + roi.height) <= imageHeight;
}

//=============================================================================
void ImageProcessor::ProcessROIsConcurrently(const byte* imageDataPtr, int imageWidth, int imageHeight)
{
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<ROIResults>> localCopies;

    // 로컬 복사본을 생성하기 전에 잠금을 걸어 동기화 문제를 방지
    {
        std::lock_guard<std::mutex> lock(m_roiMutex);
        isProcessingROIs = true;
        localCopies = m_roiResults;
    }

    std::vector<std::shared_ptr<ROIResults>> updatedResults;

    for (auto& roiResult : localCopies) {
        threads.emplace_back([&, roiResult]() {
            auto roiData = extractROI(imageDataPtr, imageWidth, roiResult->roi.x, roiResult->roi.x + roiResult->roi.width, roiResult->roi.y, roiResult->roi.y + roiResult->roi.height, roiResult->roi.width, roiResult->roi.height);
            std::shared_ptr<ROIResults> processedResult = ProcessSingleROI(std::move(roiData), roiResult->roi, GetScaleFactor(), roiResult->nIndex);
            if (processedResult) {
                std::lock_guard<std::mutex> lock(m_roiMutex); // 결과 업데이트 시 잠금
                updatedResults.push_back(processedResult);
            }
            });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    // 결과를 메인 리스트에 반영
    {
        std::lock_guard<std::mutex> lock(m_roiMutex);
        for (auto& updatedResult : updatedResults) 
        {
            auto it = std::find_if(m_roiResults.begin(), m_roiResults.end(), [&](const std::shared_ptr<ROIResults>& result) 
            {
                return result->nIndex == updatedResult->nIndex;
             });

            if (it != m_roiResults.end()) 
            {
                // 기존의 ROIResult 객체를 업데이트하되, ShapeType을 유지
                updatedResult->shapeType = (*it)->shapeType;
                *it = updatedResult;
            }
        }
        isProcessingROIs = false;
        roiProcessedCond.notify_all();  // ROI 처리 완료 알림
    }
}




//=============================================================================
// 클릭한 위치에 roi 인덱스 검색
int ImageProcessor::findROIIndexAtPoint(const cv::Point& point)
{
    std::lock_guard<std::mutex> lock(m_roiMutex);

    for (const auto& roiResult : m_roiResults)
    {
        if (roiResult->roi.contains(point))
        {
            return roiResult->nIndex;
        }
    }

    return -1; // Point가 포함된 ROI를 찾지 못한 경우
}

//=============================================================================
bool ImageProcessor::GetROIEnabled()
{
    return m_ROIEnabled;
}

//=============================================================================
void ImageProcessor::DrawRoiShape(cv::Mat& imageMat, const ROIResults& roiResult, int num_channels, ShapeType shapeType, const cv::Scalar& color)
{
    // 구조체에 있는 색을 기본값으로 
    cv::Scalar drawColor = (color == cv::Scalar(-1, -1, -1)) ? roiResult.color : color;


    switch (shapeType)
    {
    case ShapeType::Rectangle:
        cv::rectangle(imageMat, roiResult.roi, drawColor, 2);
        break;
    case ShapeType::Circle:
    {
        cv::Point center(roiResult.roi.x + roiResult.roi.width / 2, roiResult.roi.y + roiResult.roi.height / 2);
        int radius = roiResult.roi.width / 2;
        cv::circle(imageMat, center, radius, drawColor, 2);
    }
    break;
    case ShapeType::Ellipse:
    {
        cv::Point center(roiResult.roi.x + roiResult.roi.width / 2, roiResult.roi.y + roiResult.roi.height / 2);
        cv::Size axes(roiResult.roi.width / 2, roiResult.roi.height / 2);
        cv::ellipse(imageMat, center, axes, 0, 0, 360, drawColor, 2);
    }
    break;
    case ShapeType::Line:
    {
        if (abs(roiResult.roi.width) > abs(roiResult.roi.height))
        {
            cv::line(imageMat, cv::Point(roiResult.roi.x, roiResult.roi.y), cv::Point(roiResult.roi.x + roiResult.roi.width, roiResult.roi.y), drawColor, 2);
        }
        else
        {
            cv::line(imageMat, cv::Point(roiResult.roi.x, roiResult.roi.y), cv::Point(roiResult.roi.x, roiResult.roi.y + roiResult.roi.height), drawColor, 2);
        }
    }
    break;

    default:
        break;
    }
}

//=============================================================================
void ImageProcessor::SetCurrentShapeType(ShapeType type)
{
    currentShapeType = type;
    // 로그나 상태 업데이트를 위한 추가 처리
    CString logMessage;
    logMessage.Format(_T("Shape type changed to: %d"), static_cast<int>(type));
    Common::GetInstance()->AddLog(0, logMessage);

    // 도형 유형이 변경되었음을 다른 메소드에 통지할 수 있습니다.
    //NotifyShapeTypeChanged();
}

//=============================================================================
ShapeType ImageProcessor::GetCurrentShapeType()
{
    return currentShapeType;
}

//=============================================================================
void ImageProcessor::ClearTemporaryROI()
{
    std::lock_guard<std::mutex> lock(m_temporaryRoiMutex);
    m_temporaryRois.clear();
}

//=============================================================================
void ImageProcessor::StartCreatingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& startPos, cv::Point& endPos, bool& selectingROI)
{
    const int maxROIs = 4;  // 최대 ROI 개수

    // 현재 ROI 개수가 최대 개수를 초과하는지 확인
    if (m_roiResults.size() >= maxROIs) 
    {
        CString logMessage;
        logMessage.Format(_T("Maximum number of ROIs reached."));
        return;
    }

    startPos = mapPointToImage(clientPoint, imageSize, rect);
    endPos = startPos;
    selectingROI = true;
    m_isAddingNewRoi = true;
}

//=============================================================================
void ImageProcessor::UpdateCreatingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& startPos, cv::Point& endPos)
{
    endPos = mapPointToImage(clientPoint, imageSize, rect);
    cv::Rect dialog_rect(startPos, endPos);
    cv::Rect mapped_rect = mapRectToImage(dialog_rect, imageSize, cv::Size(rect.Width(), rect.Height()));
    temporaryUpdateROI(mapped_rect, GetCurrentShapeType());
}

//=============================================================================
void ImageProcessor::ConfirmCreatingROI(const cv::Point& startPos, const cv::Point& endPos, const CRect& rect, const cv::Size& imageSize, bool& selectingROI)
{
    selectingROI = false;
    m_isAddingNewRoi = false;
    cv::Rect final_rect(startPos, endPos);
    cv::Rect mapped_rect = mapRectToImage(final_rect, imageSize, cv::Size(rect.Width(), rect.Height()));
    confirmROI(mapped_rect, GetCurrentShapeType());
}

//=============================================================================
void ImageProcessor::StartDraggingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& dragStartPos, int& selectedROIIndex, bool& isDraggingROI)
{
    cv::Point imagePoint = mapPointToImage(clientPoint, imageSize, rect);
    for (int i = 0; i < m_roiResults.size(); i++) 
    {
        auto& roi = m_roiResults[i]->roi;
        if (roi.contains(imagePoint)) {
            selectedROIIndex = i;
            isDraggingROI = true;
            dragStartPos = cv::Point(imagePoint.x - roi.x, imagePoint.y - roi.y);
            // 하이라이트 색상으로 변경
            m_roiResults[i]->color = cv::Scalar(0, 255, 255);
            break;
        }
    }
}

//=============================================================================
void ImageProcessor::UpdateDraggingROI(const CPoint& clientPoint, const CRect& rect, const cv::Size& imageSize, cv::Point& dragStartPos, int selectedROIIndex)
{
    cv::Point newPoint = mapPointToImage(clientPoint, imageSize, rect);
    auto& roi = m_roiResults[selectedROIIndex]->roi;
    roi.x = newPoint.x - dragStartPos.x;
    roi.y = newPoint.y - dragStartPos.y;

    m_roiResults[selectedROIIndex]->color = cv::Scalar(0, 255, 255);
}

void ImageProcessor::ConfirmDraggingROI(bool& isDraggingROI, int& selectedROIIndex)
{
    if (selectedROIIndex >= 0) 
    {
        // 정상 색상으로 되돌림
        m_roiResults[selectedROIIndex]->color = cv::Scalar(51, 255, 51);
    }
    isDraggingROI = false;
    selectedROIIndex = -1;
}

// =============================================================================
cv::Rect ImageProcessor::mapRectToImage(const cv::Rect& rect, const cv::Size& imageSize, const cv::Size& dialogSize)
{
    double x_ratio = static_cast<double>(imageSize.width) / dialogSize.width;
    double y_ratio = static_cast<double>(imageSize.height) / dialogSize.height;

    int x = static_cast<int>(rect.x * x_ratio);
    int y = static_cast<int>(rect.y * y_ratio);
    int width = static_cast<int>(rect.width * x_ratio);
    int height = static_cast<int>(rect.height * y_ratio);

    return cv::Rect(x, y, width, height);
}

// =============================================================================
cv::Point ImageProcessor::mapPointToImage(const CPoint& point, const cv::Size& imageSize, const CRect& displayRect)
{
    double x_ratio = static_cast<double>(imageSize.width) / displayRect.Width();
    double y_ratio = static_cast<double>(imageSize.height) / displayRect.Height();

    int x = static_cast<int>((point.x - displayRect.left) * x_ratio);
    int y = static_cast<int>((point.y - displayRect.top) * y_ratio);

    return cv::Point(x, y);
}