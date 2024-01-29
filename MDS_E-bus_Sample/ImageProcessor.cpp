#include "ImageProcessor.h"

// =============================================================================
ImageProcessor::ImageProcessor(int nIndex, CameraControl_rev* pCameraControl)
    : m_nIndex(nIndex), m_CameraControl(pCameraControl)
{
    // 생성자
    // setting ini 파일 불러오기
    LoadCaminiFile(nIndex);

    Initvariable_ImageParams();
    //픽셀 포멧에 따라서 GUI Status 변경
    SetPixelFormatParametertoGUI();

    //Rainbow Palette 생성
    CreateRainbowPalette();
    //CreateIronPalette();
}

// =============================================================================
ImageProcessor::~ImageProcessor() 
{
    // 소멸자 내용 (필요한 경우 구현)
}

void ImageProcessor::RenderDataSequence(PvImage* lImage, byte* imageDataPtr, int nIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();

    // 이미지 가로 세로
    int nWidth = lImage->GetWidth();
    int nHeight = lImage->GetHeight();
    int nArraysize = nWidth * nHeight;

    // 생성할 이미지의 채널 수 및 데이터 타입 선택
    int num_channels = (Get16BitType()) ? 16 : 8;
    bool isYUVYType = GetYUVYType();
    int dataType = (Get16BitType() ? CV_16UC1 : (isYUVYType ? CV_8UC2 : CV_8UC1));

    // 마지막 ROI 좌표 대입
    cv::Rect roi = m_Select_rect;

    std::unique_ptr<uint16_t[]> fulldata = nullptr;
    std::unique_ptr<uint16_t[]> roiData = nullptr;

    fulldata = extractWholeImage(imageDataPtr, nArraysize, nWidth, nHeight);

    if (IsRoiValid(roi))
    {
        // ROI 데이터 저장
        roiData = extractROI(imageDataPtr, nWidth, roi.x, roi.x + roi.width, roi.y, roi.y + roi.height, roi.width, roi.height);
        // ROI 이미지 데이터 처리 및 최소/최대값 업데이트
        ProcessImageData(std::move(roiData), roi.width * roi.height, nWidth, nHeight, roi);
    }

    // ROI 이미지를 cv::Mat으로 변환
    if (roiData != nullptr)
    {
        cv::Mat roiMat(roi.height, roi.width, dataType, roiData.get());
    }

    if (fulldata != nullptr)
    {
        cv::Mat fulldataMat(nHeight, nWidth, dataType, fulldata.get());
        cv::Mat tempCopy = fulldataMat.clone(); // fulldataMat을 복제한 새로운 행렬 생성
        tempCopy.copyTo(m_TempData);
    }

    // 주기적으로 mat data 파일을 저장할 함수 호출

    // 이미지 노멀라이즈
    cv::Mat processedImageMat = ProcessImageBasedOnSettings(imageDataPtr, nHeight, nWidth, MainDlg);

    //ROI 객체 이미지 사이즈 저장
    SetImageSize(processedImageMat);

    //ROI 사각형 영역이 생겼을 경우에만 그린다
    if (IsRoiValid(roi) && !processedImageMat.empty())
    {
        // ROI 사각형 그리기
        DrawRoiRectangle(processedImageMat, m_Select_rect, num_channels);
    }

    // 카메라 모델명 좌상단에 그리기
    putTextOnCamModel(processedImageMat);

    // 화면에 라이브 이미지 그리기
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

    m_bYUVYFlag = false;
    m_bStartRecording = false;

    m_nCsvFileCount = 1;
    m_colormapType = (PaletteTypes)PALETTE_IRON; // 컬러맵 변수, 초기값은 IRON으로 설정

    m_TempData = cv::Mat();
    m_videoMat = cv::Mat();

    std::string strPath = CT2A(Common::GetInstance()->Getsetingfilepath());
    m_strRawdataPath = strPath + "Camera_" + std::to_string(m_CameraControl->GetCamIndex() + 1) + "\\";
    SetRawdataPath(m_strRawdataPath);

    Common::GetInstance()->CreateDirectoryRecursively(GetRawdataPath());
    Common::GetInstance()->CreateDirectoryRecursively(GetImageSavePath());
    Common::GetInstance()->CreateDirectoryRecursively(GetRawSavePath());
    Common::GetInstance()->CreateDirectoryRecursively(GetRecordingPath());

    strLog.Format(_T("---------Camera[%d] ImageParams Variable Initialize "), m_CameraControl->GetCamIndex() + 1);
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
    m_CameraControl->m_Cam_Params->strPixelFormat = iniValue;

    iniKey.Format(_T("Save_interval"));
    m_nSaveInterval = GetPrivateProfileInt(iniSection, iniKey, (bool)false, filePath);

    // YUVY 경우일경우 체크박스 활성화
    if (m_CameraControl->m_Cam_Params->strPixelFormat == YUV422_8_UYVY)
    {
        SetYUVYType(TRUE);
        Set16BitType(FALSE);
        SetGrayType(FALSE);
        SetRGBType(FALSE);
    }
    else if (m_CameraControl->m_Cam_Params->strPixelFormat == MONO8)
    {
        SetGrayType(TRUE);
        SetYUVYType(FALSE);
        Set16BitType(FALSE);
        SetRGBType(FALSE);
    }
    else if (m_CameraControl->m_Cam_Params->strPixelFormat == MONO16 || m_CameraControl->m_Cam_Params->strPixelFormat == MONO14)
    {
        Set16BitType(TRUE);
        SetGrayType(TRUE);
        SetYUVYType(FALSE);
        SetRGBType(FALSE);
    }
    else if (m_CameraControl->m_Cam_Params->strPixelFormat == RGB8PACK)
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
    OriMat.copySize(img);
}

// =============================================================================
cv::Size ImageProcessor::GetImageSize()
{
    return OriMat.size();
}

// =============================================================================
void ImageProcessor::putTextOnCamModel(cv::Mat& imageMat) 
{
    if (!imageMat.empty())
    {
        // CString을 std::string으로 변환합니다.m_CameraControl
        CString cstrText = m_CameraControl->Manager->m_strSetModelName.at(m_CameraControl->GetCamIndex());
        std::string strText = std::string(CT2CA(cstrText));

        // 텍스트를 이미지에 삽입합니다.
        cv::putText(imageMat, strText, cv::Point(0, 25), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
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
        if (!m_CameraControl->GetCamRunningFlag())
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

    cv::Mat colorMappedImage;
    cv::Mat ResultImage;
    Mat GrayImage;
    int num_channels = Get16BitType() ? 16 : 8;
    int nImageType = processedImageMat.type();
    CString strLog;
    // 16bit
    if (Get16BitType())
    {
        if (GetColorPaletteType())
        {
            ResultImage = MapColorsToPalette(processedImageMat, GetPaletteType());
            if (ResultImage.type() == CV_8UC1)
            {
                cv::cvtColor(colorMappedImage, ResultImage, cv::COLOR_GRAY2BGR);
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
    }
    // 8bit
    else
    {
        if (GetYUVYType())
        {
            ResultImage = ConvertUYVYToBGR8(processedImageMat);
        }
        else if (GetGrayType() == TRUE)
        {
            cv::cvtColor(processedImageMat, GrayImage, cv::COLOR_GRAY2BGR);
            cv::normalize(GrayImage, ResultImage, 0, 255, cv::NORM_MINMAX, CV_8UC1);
            cv::cvtColor(ResultImage, ResultImage, cv::COLOR_BGR2GRAY);
        }
        else if (GetRGBType())
        {
            processedImageMat.copyTo(ResultImage);
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

    if (GetStartRecordingFlag())
    {
        if (!m_isRecording && !processedImageMat.empty())
        {
            if (StartRecording(nWidth, nHeight, m_CameraControl->GetCameraFPS()))
            {
                // StartRecording open 성공   
                strLog.Format(_T("---------Camera[%d] Video Start Recording"), m_CameraControl->GetCamIndex() + 1);
                Common::GetInstance()->AddLog(0, strLog);
            }
            else
            {
                strLog.Format(_T("---------Camera[%d] Video Open Fail"), m_CameraControl->GetCamIndex() + 1);
                Common::GetInstance()->AddLog(0, strLog);
            }
        }
        else
        {
            // 이미지 프레임을 녹화 전용 데이터로 복사
            UpdateFrame(processedImageMat);
        }
    }
}

// =============================================================================
// 이미지 처리 중 동적 생성 객체 메모리 해제
void ImageProcessor::CleanupAfterProcessing() 
{
    if (m_BitmapInfo != nullptr)
    {
        delete[] m_BitmapInfo;
        m_BitmapInfo = nullptr;
    }
}

// =============================================================================
std::vector<std::string> ImageProcessor::CreateRainbowPalette()
{
    int paletteSize = 256; // 색상 개수를 512로 수정
    int segmentLength = paletteSize / 3; // 각 세그먼트의 길이

    for (int i = 0; i < paletteSize; ++i) 
    {
        int r, g, b;

        if (i < segmentLength) 
        {
            r = static_cast<int>((1 - sin(M_PI * i / segmentLength / 2)) * 255);
            g = 0;
            b = static_cast<int>((sin(M_PI * i / segmentLength / 2)) * 255);
        }
        else if (i < 2 * segmentLength) 
        {
            r = 0;
            g = static_cast<int>((sin(M_PI * (i - segmentLength) / segmentLength / 2)) * 255);
            b = static_cast<int>((1 - sin(M_PI * (i - segmentLength) / segmentLength / 2)) * 255);
        }
        else 
        {
            r = static_cast<int>((sin(M_PI * (i - 2 * segmentLength) / segmentLength / 2)) * 255);
            g = static_cast<int>((1 - sin(M_PI * (i - 2 * segmentLength) / segmentLength / 2)) * 255);
            b = 0;
        }

        char color[8];
        snprintf(color, sizeof(color), "#%02X%02X%02X", r, g, b);
        Rainbow_palette.push_back(color);
    }

    return Rainbow_palette;
}

// =============================================================================
std::vector<std::string> ImageProcessor::CreateRainbowPalette_1024()
{
    int paletteSize = 1024; // 색상 개수 확장
    int segmentLength = paletteSize / 3; // 각 세그먼트의 길이

    for (int i = 0; i < paletteSize; ++i) {
        int r, g, b;

        if (i < segmentLength) {
            r = static_cast<int>((1 - sin(M_PI * i / segmentLength / 2)) * 255);
            g = 0;
            b = static_cast<int>((sin(M_PI * i / segmentLength / 2)) * 255);
        }
        else if (i < 2 * segmentLength) {
            r = 0;
            g = static_cast<int>((sin(M_PI * (i - segmentLength) / segmentLength / 2)) * 255);
            b = static_cast<int>((1 - sin(M_PI * (i - segmentLength) / segmentLength / 2)) * 255);
        }
        else {
            r = static_cast<int>((sin(M_PI * (i - 2 * segmentLength) / segmentLength / 2)) * 255);
            g = static_cast<int>((1 - sin(M_PI * (i - 2 * segmentLength) / segmentLength / 2)) * 255);
            b = 0;
        }

        char color[8];
        snprintf(color, sizeof(color), "#%02X%02X%02X", r, g, b);
        Rainbow_palette.push_back(color);
    }
    return Rainbow_palette;
}

// =============================================================================
std::vector<cv::Vec3b> ImageProcessor::adjustPaletteScale(const std::vector<cv::Vec3b>&originalPalette, double scaleR, double scaleG, double scaleB)
{
    std::vector<cv::Vec3b> adjustedPalette;
    for (const cv::Vec3b& color : originalPalette)
    {
        // 각 색상의 B, G, R 값을 스케일링
        uchar newB = static_cast<uchar>(color[0] * scaleB);
        uchar newG = static_cast<uchar>(color[1] * scaleG);
        uchar newR = static_cast<uchar>(color[2] * scaleR);


        // 값이 255를 초과하지 않도록 제한
        newB = std::min(255, (int)newB);
        newG = std::min(255, (int)newG);
        newR = std::min(255, (int)newR);

        adjustedPalette.push_back(cv::Vec3b(newB, newG, newR));
    }
    return adjustedPalette;
}

// =============================================================================
cv::Mat ImageProcessor::applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB)
{
    std::vector<cv::Vec3b> bgr_palette = convertPaletteToBGR(GetPalette(palette));

    // 스케일을 적용한 조정된 팔레트 생성
    std::vector<cv::Vec3b> adjusted_palette = adjustPaletteScale(bgr_palette, scaleR, scaleG, scaleB);

    //사용 안함
    //m_Markcolor = findBrightestColor(adjusted_palette);
    //cv::Vec3b greenColor(0, 0, 255);
    //m_findClosestColor = findClosestColor(adjusted_palette, greenColor);

    // B, G, R 채널 배열 생성
    cv::Mat b(256, 1, CV_8U);
    cv::Mat g(256, 1, CV_8U);
    cv::Mat r(256, 1, CV_8U);

    for (int i = 0; i < 256; ++i) {
        b.at<uchar>(i, 0) = adjusted_palette[i][0];
        g.at<uchar>(i, 0) = adjusted_palette[i][1];
        r.at<uchar>(i, 0) = adjusted_palette[i][2];
    }

    // B, G, R 채널을 합치고 룩업 테이블 생성
    cv::Mat channels[] = { b, g, r };
    cv::Mat lut; // 룩업 테이블 생성
    cv::merge(channels, 3, lut);

    // 아이언 파레트를 이미지에 적용
    cv::Mat im_color;
    cv::LUT(im_gray, lut, im_color);

    return im_color;
}

// =============================================================================
std::vector<std::string> ImageProcessor::GetPalette(PaletteTypes paletteType)
{
    switch (paletteType)
    {
    case PALETTE_IRON:
        return iron_palette;
    case PALETTE_RAINBOW:
        return Rainbow_palette;
    case PALETTE_ARCTIC:
        return Arctic_palette;
    case PALETTE_JET:
        return Jet_palette;
    case PALETTE_INFER:
        return Infer_palette;
    case PALETTE_PLASMA:
        return Plasma_palette;
    case PALETTE_RED_GRAY:
        return Red_gray_palette;
    case PALETTE_VIRIDIS:
        return Viridis_palette;
    case PALETTE_MAGMA:
        return Magma_palette;
    case PALETTE_CIVIDIS:
        return Cividis_palette;
    case PALETTE_COOLWARM:
        return Coolwarm_palette;
    case PALETTE_SPRING:
        return Spring_palette;
    case PALETTE_SUMMER:
        return Summer_palette;
    default:
        // 기본값 또는 오류 처리를 수행할 수 있음
        return std::vector<std::string>();
    }
}

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
    std::lock_guard<std::mutex> lock(videomtx);
    m_videoMat = newFrame.clone(); // 새 프레임으로 업데이트
}

// =============================================================================
bool ImageProcessor::StartRecording(int frameWidth, int frameHeight, double frameRate)
{
    std::lock_guard<std::mutex> lock(videomtx);

    CString strLog = _T("");

    if (m_isRecording)
    {
        strLog.Format(_T("---------Camera[%d] Already recording"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        return false; // 이미 녹화 중이므로 false 반환
    }

    // 녹화 이미지 생성을 위한 쓰레드 시작
    std::thread recordThread(&ImageProcessor::RecordThreadFunction, this, frameRate);
    recordThread.detach(); // 쓰레드를 분리하여 독립적으로 실행

    bool bSaveType = !GetGrayType(); // GetGrayType()가 true면 false, 아니면 true로 설정

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

    m_isRecording = true;

    return true; // 녹화 시작 성공 반환
}

// =============================================================================
void ImageProcessor::RecordThreadFunction(double frameRate)
{
    std::chrono::milliseconds frameDuration(static_cast<int>(1000 / frameRate));

    while (GetStartRecordingFlag())
    {
        auto start = std::chrono::high_resolution_clock::now();  // 시작 시간 기록

        Mat frameToWrite;
        {
            std::lock_guard<std::mutex> lock(videomtx);
            if (!m_videoMat.empty())
            {
                frameToWrite = m_videoMat.clone(); // 현재 프레임 복사
            }
        }

        // 복사된 프레임을 사용하여 기록
        if (!frameToWrite.empty())
        {
            std::lock_guard<std::mutex> writerLock(writermtx);
            videoWriter.write(frameToWrite);
        }

        auto end = std::chrono::high_resolution_clock::now();  // 종료 시간 기록
        auto elapsed = end - start;
        auto timeToWait = frameDuration - elapsed;

        // 다음 프레임까지 대기
        if (timeToWait.count() > 0)
        {
            std::this_thread::sleep_for(timeToWait);
        }
    }
}

// =============================================================================
void ImageProcessor::StopRecording()
{
    std::lock_guard<std::mutex> lock(videomtx);

    if (!m_isRecording)
    {
        return;
    }

    videoWriter.release();
    m_isRecording = false;
    SetStartRecordingFlag(false);

    CString strLog = _T("");
    strLog.Format(_T("---------Camera[%d] Video Stop Recording"), m_CameraControl->GetCamIndex() + 1);
    Common::GetInstance()->AddLog(0, strLog);
}

// =============================================================================
void ImageProcessor::SetStartRecordingFlag(bool bFlag)
{
    m_bStartRecording = bFlag;
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
    // 이미 변환된 결과를 저장할 캐시
    static std::map<std::vector<std::string>, std::vector<cv::Vec3b>> cache; // std::map으로 변경

    // 입력 벡터에 대한 캐시 키 생성
    std::vector<std::string> cacheKey = hexPalette;

    // 캐시에서 이미 변환된 결과를 찾아 반환
    if (cache.find(cacheKey) != cache.end())
    {
        return cache[cacheKey];
    }

    // 변환 작업 수행
    std::vector<cv::Vec3b> bgrPalette;
    for (const std::string& hexColor : hexPalette)
    {
        cv::Vec3b bgrColor = hexStringToBGR(hexColor);
        bgrPalette.push_back(bgrColor);
    }

    // 변환 결과를 캐시에 저장
    cache[cacheKey] = bgrPalette;

    // 캐시에 저장된 복사본을 반환
    return cache[cacheKey];
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
        strLog.Format(_T("[Camera[%d]] YUV422_8_UYVY Mode"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        MainDlg->m_chUYVYCheckBox.SetCheck(TRUE);

        Set16BitType(true); //8비트 고정
        MainDlg->m_chEventsCheckBox.EnableWindow(FALSE); //8비트 고정
        MainDlg->m_chColorMapCheckBox.EnableWindow(FALSE);
        MainDlg->m_chMonoCheckBox.EnableWindow(FALSE);

        strLog.Format(_T("[Camera[%d]] UYVY CheckBox Checked"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
        strLog.Format(_T("[Camera[%d]] 16Bit CheckBox Checked, Disable Windows"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("[Camera[%d]] ColorMap  Disable Windows"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("[Camera[%d]] Mono  Disable Windows"), m_CameraControl->GetCamIndex() + 1);
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
// ROI 존재 유효성 체크
bool ImageProcessor::IsRoiValid(const cv::Rect& roi)
{
    return roi.width > 1 && roi.height > 1;
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
    int nTemp = std::numeric_limits<T>::max();
    cv::normalize(imageMat, imageMat, 0, std::numeric_limits<T>::max(), cv::NORM_MINMAX);
    double min_val, max_val;
    cv::minMaxLoc(imageMat, &min_val, &max_val);

    int range = static_cast<int>(max_val) - static_cast<int>(min_val);
    if (range == 0)
        range = 1;

    //imageMat.convertTo(imageMat, cvType, std::numeric_limits<T>::max() / range, -(min_val)* std::numeric_limits<T>::max() / range);

    return imageMat;
}

// =============================================================================
// ROI 객체 그리기 함수
void ImageProcessor::DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels)
{
    std::lock_guard<std::mutex> lock(drawmtx);

    int markerSize = 15; // 마커의 크기 설정
    cv::Scalar redColor(0, 0, 0); // 빨간색
    cv::Scalar blueColor(255, 255, 0); // 파란색
    cv::Scalar roiColor = (255, 255, 255);
    cv::rectangle(imageMat, roiRect, blueColor, 2, 8, 0);

    cv::Point mincrossCenter(m_MinSpot.x, m_MinSpot.y);
    cv::Point maxcrossCenter(m_MaxSpot.x, m_MaxSpot.y);

    CString cstrValue;
    std::string strValue;
    cstrValue.Format(_T(" [%d]"), m_MinSpot.tempValue);
    strValue = std::string(CT2CA(cstrValue));
    cv::putText(imageMat, strValue, mincrossCenter, cv::FONT_HERSHEY_PLAIN, 1.5, roiColor, 1, cv::LINE_AA);
    cv::drawMarker(imageMat, mincrossCenter, roiColor, cv::MARKER_TRIANGLE_DOWN, markerSize, 2);

    // 텍스트를 이미지에 삽입합니다.
    cstrValue.Format(_T(" [%d]"), m_MaxSpot.tempValue);
    strValue = std::string(CT2CA(cstrValue));
    cv::putText(imageMat, strValue, maxcrossCenter, cv::FONT_HERSHEY_PLAIN, 1.5, roiColor, 1, cv::LINE_AA);
    cv::drawMarker(imageMat, maxcrossCenter, roiColor, cv::MARKER_TRIANGLE_UP, markerSize, 2);
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

    if ((PaletteTypes)palette == PALETTE_IRON ||
        (PaletteTypes)palette == PALETTE_INFER ||
        (PaletteTypes)palette == PALETTE_PLASMA ||
        (PaletteTypes)palette == PALETTE_MAGMA ||
        (PaletteTypes)palette == PALETTE_SUMMER)
    {
        dScaleG = 1.5;
    }
    else if((PaletteTypes)palette == PALETTE_RED_GRAY)
    {
        dScaleR = 1;
    }

    // 아이언 1/2/1
    colorMappedImage = applyIronColorMap(normalizedImage3channel, palette, dScaleR, dScaleG, dScaleB);
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
        imageArray[i] = (nRawdata * GetScaleFactor()) - FAHRENHEIT;

    }
    //CString strLog;
    //OutputDebugString(strLog);

    return imageArray; // std::unique_ptr 반환
}

// =============================================================================
// 지정된 영역의 데이터 처리
std::unique_ptr<uint16_t[]> ImageProcessor::extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight)
{
    // roiWidth와 roiHeight 크기의 uint16_t 배열을 동적으로 할당합니다.
    int nArraySize = roiWidth * roiHeight;
    std::unique_ptr<uint16_t[]> roiArray = std::make_unique<uint16_t[]>(nArraySize);

    int k = 0;

    for (int y = nStartY; y < nEndY; y++)
    {
        int rowOffset = y * nWidth;
        for (int x = nStartX; x < nEndX; x++)
        {
            int index = rowOffset + x;

            // 입력 데이터의 유효성 검사

            // 데이터를 읽고 예상 형식과 일치하는지 확인
            uint16_t sample = static_cast<uint16_t>(byteArr[index * 2] | (byteArr[index * 2 + 1] << 8));
            roiArray[k++] = sample;
        }
    }
    //CString logMessage;
    //logMessage.Format(_T("roisize = %d"), k);
    //OutputDebugString(logMessage);

    return roiArray; // std::unique_ptr 반환
}

// =============================================================================
// MinMax 변수 초기화
bool ImageProcessor::InitializeTemperatureThresholds()
{
    // min, max 변수값 reset
    m_Max = 0;
    m_Min = 65535;
    m_Max_X = 0;
    m_Max_Y = 0;
    m_Min_X = 0;
    m_Min_Y = 0;

    return true;
}

// =============================================================================
// 카메라 종류별 스케일값 적용
double ImageProcessor::GetScaleFactor()
{
    double dScale = 0;

    switch (m_CameraControl->m_Camlist)
    {
    case FT1000:
    case A400:
    case A500:
    case A600:
    case A700:
    case A615:
    case A50:
        dScale = (0.01f);
        break;
    case Ax5:
        dScale = (0.04f);
        break;
    case BlackFly:
        dScale = 0;
        break;
    default:
        dScale = (0.01f);
        break;
    }

    return dScale;
}

// =============================================================================
// ROI 내부의 데이터 연산
void ImageProcessor::ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx)
{
    int absoluteX = nCurrentX + roi.x; // ROI 상대 좌표를 절대 좌표로 변환
    int absoluteY = nCurrentY + roi.y; // ROI 상대 좌표를 절대 좌표로 변환

    // 최대 최소 온도 체크 후 백업
    if (uTempValue <= m_Min)
    {
        m_Min = uTempValue;
        m_Min_X = absoluteX;
        m_Min_Y = absoluteY;

        m_MinSpot.pointIdx = nPointIdx;

        m_MinSpot.x = m_Min_X;
        m_MinSpot.y = m_Min_Y;
        m_MinSpot.tempValue = ((static_cast<float>(m_Min) * dScale) - FAHRENHEIT);
    }
    else if (uTempValue >= m_Max)
    {
        m_Max = uTempValue;
        m_Max_X = absoluteX;
        m_Max_Y = absoluteY;

        m_MaxSpot.x = m_Max_X;
        m_MaxSpot.y = m_Max_Y;
        m_MaxSpot.tempValue = ((static_cast<float>(m_Max) * dScale) - FAHRENHEIT);
        m_MaxSpot.pointIdx = nPointIdx;
    }
}

// =============================================================================
// 16비트 데이터 최소,최대값 산출
void ImageProcessor::ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi)
{
    double dScale = GetScaleFactor();
    InitializeTemperatureThresholds();


    int nPointIdx = 0; // 포인트 인덱스 초기화

    for (int y = 0; y < roi.height; y++)
    {
        for (int x = 0; x < roi.width; x++)
        {
            int dataIndex = y * roi.width + x; // ROI 내에서의 인덱스 계산
            ushort tempValue = static_cast<ushort>(data[dataIndex]);

            ROIXYinBox(tempValue, dScale, x, y, roi, nPointIdx);

            nPointIdx++; // 포인트 인덱스 증가
        }
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
    BITMAPINFO* m_BitmapInfo = nullptr;
    int imageSize = 0;
    int bpp = 0;
    int nbiClrUsed = 0;
    int nType = imageMat.type();

    try
    {
        m_BitmapInfo = new BITMAPINFO;
        bpp = (int)imageMat.elemSize() * 8;

        if (GetYUVYType() || GetColorPaletteType())
        {
            bpp = 24;
            imageSize = w * h * 3;
            m_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[sizeof(BITMAPINFO)]);
            // 픽셀당 비트 수 (bpp)
            nbiClrUsed = 0;

        }
        else if (GetGrayType())
        {
            m_BitmapInfo = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)];
            imageSize = w * h;
            nbiClrUsed = 256;

            memcpy(((BITMAPINFO*)m_BitmapInfo)->bmiColors, GrayPalette, 256 * sizeof(RGBQUAD));
        }
        else
        {
            bpp = 24;
            imageSize = w * h;
            m_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[sizeof(BITMAPINFO)]);
            // 픽셀당 비트 수 (bpp)
            nbiClrUsed = 0;
        }

        BITMAPINFOHEADER& bmiHeader = m_BitmapInfo->bmiHeader;

        bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiHeader.biWidth = w;
        bmiHeader.biHeight = -h;  // 이미지를 아래에서 위로 표시
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = bpp;
        bmiHeader.biCompression = BI_RGB;
        bmiHeader.biSizeImage = imageSize; // 이미지 크기 초기화
        bmiHeader.biClrUsed = nbiClrUsed;   // 색상 팔레트 적용

        return m_BitmapInfo;
    }
    catch (const std::bad_alloc&)
    {
        delete m_BitmapInfo; // 메모리 해제
        m_BitmapInfo = nullptr;
        throw std::runtime_error("Failed to allocate memory for BITMAPINFO");
    }
}

// =============================================================================
// Mat data to BITMAPINFO
// 라이브 이미지 깜박임을 최소화하기 위하여 더블 버퍼링 기법 사용
void ImageProcessor::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CClientDC dc(MainDlg->GetDlgItem(nID));
    CRect rect;
    cv::Mat imageCopy = mImage.clone();
    MainDlg->GetDlgItem(nID)->GetClientRect(&rect);

    // 백 버퍼 생성
    CBitmap backBufferBitmap;
    CDC backBufferDC;
    backBufferDC.CreateCompatibleDC(&dc);
    backBufferBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBackBufferBitmap = backBufferDC.SelectObject(&backBufferBitmap);

    // 백 버퍼에 그림 그리기
    backBufferDC.SetStretchBltMode(COLORONCOLOR);
    StretchDIBits(backBufferDC.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, imageCopy.cols, imageCopy.rows, imageCopy.data,
        BitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    // 프론트 버퍼로 스왑
    dc.BitBlt(0, 0, rect.Width(), rect.Height(), &backBufferDC, 0, 0, SRCCOPY);

    // 선택한 객체 해제
    backBufferDC.SelectObject(pOldBackBufferBitmap);

    // 생성한 객체 해제
    backBufferBitmap.DeleteObject();
    backBufferDC.DeleteDC();

    // CClientDC dc(MainDlg->GetDlgItem(nID)) 객체가 메모리 누수를 일으키지 않는다고 가정하고, DeleteObject(dc)는 제거.
}

// =============================================================================
//화면에 출력한 비트맵 데이터를 생성
void ImageProcessor::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex)
{
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
// A50 카메라의 경우 높이를 업데이트하는 함수 FFF 데이터를 얻기위해서 지정된 헤더 영역을 더 얻어와야한다.
int ImageProcessor::UpdateHeightForA50Camera(int& nHeight, int nWidth)
{
    if (nHeight <= 0 || nWidth <= 0)
    {
        // nHeight나 nWidth가 0 이하인 경우 처리
        nHeight = 0;
        return nHeight;
    }

    if (m_CameraControl->m_Camlist == A50 && nHeight > 0)
    {
        int heightAdjustment = 1392 / nWidth + (1392 % nWidth ? 1 : 0);

        if (nHeight <= heightAdjustment)
        {
            // 예외 처리
            // nHeight가 heightAdjustment보다 작거나 같은 경우 처리  
            // 현재는 nHeight를 0으로 설정하여 음수 값 방지.
            nHeight = 0;
        }
        else
        {
            nHeight -= heightAdjustment;
        }
    }

    return nHeight;
}