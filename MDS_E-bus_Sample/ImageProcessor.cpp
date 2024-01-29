#include "ImageProcessor.h"

// =============================================================================
ImageProcessor::ImageProcessor(int nIndex, CameraControl_rev* pCameraControl)
    : m_nIndex(nIndex), m_CameraControl(pCameraControl)
{
    // ������
    // setting ini ���� �ҷ�����
    LoadCaminiFile(nIndex);

    Initvariable_ImageParams();
    //�ȼ� ���信 ���� GUI Status ����
    SetPixelFormatParametertoGUI();

    //Rainbow Palette ����
    CreateRainbowPalette();
    //CreateIronPalette();
}

// =============================================================================
ImageProcessor::~ImageProcessor() 
{
    // �Ҹ��� ���� (�ʿ��� ��� ����)
}

void ImageProcessor::RenderDataSequence(PvImage* lImage, byte* imageDataPtr, int nIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();

    // �̹��� ���� ����
    int nWidth = lImage->GetWidth();
    int nHeight = lImage->GetHeight();
    int nArraysize = nWidth * nHeight;

    // ������ �̹����� ä�� �� �� ������ Ÿ�� ����
    int num_channels = (Get16BitType()) ? 16 : 8;
    bool isYUVYType = GetYUVYType();
    int dataType = (Get16BitType() ? CV_16UC1 : (isYUVYType ? CV_8UC2 : CV_8UC1));

    // ������ ROI ��ǥ ����
    cv::Rect roi = m_Select_rect;

    std::unique_ptr<uint16_t[]> fulldata = nullptr;
    std::unique_ptr<uint16_t[]> roiData = nullptr;

    fulldata = extractWholeImage(imageDataPtr, nArraysize, nWidth, nHeight);

    if (IsRoiValid(roi))
    {
        // ROI ������ ����
        roiData = extractROI(imageDataPtr, nWidth, roi.x, roi.x + roi.width, roi.y, roi.y + roi.height, roi.width, roi.height);
        // ROI �̹��� ������ ó�� �� �ּ�/�ִ밪 ������Ʈ
        ProcessImageData(std::move(roiData), roi.width * roi.height, nWidth, nHeight, roi);
    }

    // ROI �̹����� cv::Mat���� ��ȯ
    if (roiData != nullptr)
    {
        cv::Mat roiMat(roi.height, roi.width, dataType, roiData.get());
    }

    if (fulldata != nullptr)
    {
        cv::Mat fulldataMat(nHeight, nWidth, dataType, fulldata.get());
        cv::Mat tempCopy = fulldataMat.clone(); // fulldataMat�� ������ ���ο� ��� ����
        tempCopy.copyTo(m_TempData);
    }

    // �ֱ������� mat data ������ ������ �Լ� ȣ��

    // �̹��� ��ֶ�����
    cv::Mat processedImageMat = ProcessImageBasedOnSettings(imageDataPtr, nHeight, nWidth, MainDlg);

    //ROI ��ü �̹��� ������ ����
    SetImageSize(processedImageMat);

    //ROI �簢�� ������ ������ ��쿡�� �׸���
    if (IsRoiValid(roi) && !processedImageMat.empty())
    {
        // ROI �簢�� �׸���
        DrawRoiRectangle(processedImageMat, m_Select_rect, num_channels);
    }

    // ī�޶� �𵨸� �»�ܿ� �׸���
    putTextOnCamModel(processedImageMat);

    // ȭ�鿡 ���̺� �̹��� �׸���
    processedImageMat = DisplayLiveImage(MainDlg, processedImageMat, nIndex);

    // ���� �������� �̹���, �ο쵥���� �ڵ� ���� 
    SaveFilePeriodically(m_TempData, processedImageMat);

    //��ȭ ��� �Լ�
    ProcessAndRecordFrame(processedImageMat, nWidth, nHeight);

    //  �۾��� �Ϸ�� ��Ʈ�� ������ �����մϴ�, �������� �Ҵ��� �޸𸮸� ����
    CleanupAfterProcessing();
}

// =============================================================================
void ImageProcessor::Initvariable_ImageParams()
{

    CString strLog = _T("");

    m_bYUVYFlag = false;
    m_bStartRecording = false;

    m_nCsvFileCount = 1;
    m_colormapType = (PaletteTypes)PALETTE_IRON; // �÷��� ����, �ʱⰪ�� IRON���� ����

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
//���� ���� �ҷ�����
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

    // YUVY ����ϰ�� üũ�ڽ� Ȱ��ȭ
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
// �� ���� ���̾�α� ������
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
        // CString�� std::string���� ��ȯ�մϴ�.m_CameraControl
        CString cstrText = m_CameraControl->Manager->m_strSetModelName.at(m_CameraControl->GetCamIndex());
        std::string strText = std::string(CT2CA(cstrText));

        // �ؽ�Ʈ�� �̹����� �����մϴ�.
        cv::putText(imageMat, strText, cv::Point(0, 25), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
    }
}

// =============================================================================
// 8��Ʈ�� 16��Ʈ�� ��ȯ
std::unique_ptr<uint16_t[]> ImageProcessor::Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length)
{

    // roiWidth�� roiHeight ũ���� uint16_t �迭�� �������� �Ҵ��մϴ�.
    std::unique_ptr<uint16_t[]> roiArray = std::make_unique<uint16_t[]>(length);
    for (int i = 0; i < length; i++)
    {
        if (!m_CameraControl->GetCamRunningFlag())
            return nullptr;

        // �����͸� �а� ���� ���İ� ��ġ�ϴ��� Ȯ��
        uint16_t sample = static_cast<uint16_t>(src[i * 2] | (src[i * 2 + 1] << 8));

        roiArray[i] = sample;
    }

    return roiArray;
}

// =============================================================================
// Main GUI ���̺� �̹������
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
// �̹��� ó�� �Լ�
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
                // StartRecording open ����   
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
            // �̹��� �������� ��ȭ ���� �����ͷ� ����
            UpdateFrame(processedImageMat);
        }
    }
}

// =============================================================================
// �̹��� ó�� �� ���� ���� ��ü �޸� ����
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
    int paletteSize = 256; // ���� ������ 512�� ����
    int segmentLength = paletteSize / 3; // �� ���׸�Ʈ�� ����

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
    int paletteSize = 1024; // ���� ���� Ȯ��
    int segmentLength = paletteSize / 3; // �� ���׸�Ʈ�� ����

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
        // �� ������ B, G, R ���� �����ϸ�
        uchar newB = static_cast<uchar>(color[0] * scaleB);
        uchar newG = static_cast<uchar>(color[1] * scaleG);
        uchar newR = static_cast<uchar>(color[2] * scaleR);


        // ���� 255�� �ʰ����� �ʵ��� ����
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

    // �������� ������ ������ �ȷ�Ʈ ����
    std::vector<cv::Vec3b> adjusted_palette = adjustPaletteScale(bgr_palette, scaleR, scaleG, scaleB);

    //��� ����
    //m_Markcolor = findBrightestColor(adjusted_palette);
    //cv::Vec3b greenColor(0, 0, 255);
    //m_findClosestColor = findClosestColor(adjusted_palette, greenColor);

    // B, G, R ä�� �迭 ����
    cv::Mat b(256, 1, CV_8U);
    cv::Mat g(256, 1, CV_8U);
    cv::Mat r(256, 1, CV_8U);

    for (int i = 0; i < 256; ++i) {
        b.at<uchar>(i, 0) = adjusted_palette[i][0];
        g.at<uchar>(i, 0) = adjusted_palette[i][1];
        r.at<uchar>(i, 0) = adjusted_palette[i][2];
    }

    // B, G, R ä���� ��ġ�� ��� ���̺� ����
    cv::Mat channels[] = { b, g, r };
    cv::Mat lut; // ��� ���̺� ����
    cv::merge(channels, 3, lut);

    // ���̾� �ķ�Ʈ�� �̹����� ����
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
        // �⺻�� �Ǵ� ���� ó���� ������ �� ����
        return std::vector<std::string>();
    }
}

std::string ImageProcessor::GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension)
{
    // ���� �ð� ���� ��������
    auto currentTime = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::to_time_t(currentTime);
    struct tm localTime;
    localtime_s(&localTime, &now);

    // �ð� ������ �̿��Ͽ� ���� �̸� ����
    char timeStr[50];
    strftime(timeStr, sizeof(timeStr), "_%Y_%m_%d_%H_%M_%S", &localTime);

    // ���� ���� ��� ����
    std::string filePath = basePath + prefix + timeStr + extension;

    return filePath;
}

bool ImageProcessor::SaveImageWithTimestamp(const cv::Mat& image)
{
    std::string basePath = GetImageSavePath(); // �̹��� ���� ���
    std::string prefix = "temperature_image"; // ���� �̸� ���λ�
    std::string extension = ".jpg"; // Ȯ����

    // ���� �̸� ����
    std::string filePath = GenerateFileNameWithTimestamp(basePath, prefix, extension);

    // �̹��� ����
    if (!cv::imwrite(filePath, image))
    {
        // ���� ó��
        return false;
    }

    // ���������� ������ ��� �α� ���
    CString strLog;
    std::wstring filePathW(filePath.begin(), filePath.end());
    strLog.Format(_T("Completed writeback to image file: \n%s"), filePathW.c_str());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

// =============================================================================
bool ImageProcessor::SaveRawDataWithTimestamp(const cv::Mat& rawData)
{
    std::string basePath = GetRawSavePath(); // Raw ������ ���� ���
    std::string prefix = "rawdata"; // ���� �̸� ���λ�
    std::string extension = ".tmp"; // Ȯ����

    // ���� �̸� ����
    std::string filePath = GenerateFileNameWithTimestamp(basePath, prefix, extension);

    // Raw ������ ����
    if (!WriteCSV(filePath, rawData))
    {
        // ���� ó��
        return false;
    }

    // ���������� ������ ��� �α� ���
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

    //���� ���� �̹��� ����
    if (GetMouseImageSaveFlag())
    {
        SaveImageWithTimestamp(imagedata);
        SetMouseImageSaveFlag(FALSE);
    }

    if (lastCallTime.time_since_epoch().count() == 0)
    {
        lastCallTime = std::chrono::system_clock::now(); // ù �Լ� ȣ��� lastCallTime �ʱ�ȭ
    }

    auto currentTime = std::chrono::system_clock::now();
    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastCallTime).count();

    int seconds = m_nSaveInterval * 60;

    if (elapsedSeconds >= seconds) //
    {
        // �߰�: Mat ��ü�� JPEG, tmp ���Ϸ� ����

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
    m_videoMat = newFrame.clone(); // �� ���������� ������Ʈ
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

        return false; // �̹� ��ȭ ���̹Ƿ� false ��ȯ
    }

    // ��ȭ �̹��� ������ ���� ������ ����
    std::thread recordThread(&ImageProcessor::RecordThreadFunction, this, frameRate);
    recordThread.detach(); // �����带 �и��Ͽ� ���������� ����

    bool bSaveType = !GetGrayType(); // GetGrayType()�� true�� false, �ƴϸ� true�� ����

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
        return false; // ��ȭ ���� ��ȯ
    }
    else
    {
        strLog.Format(_T("---------Camera[%d] Video Writer open success"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    m_isRecording = true;

    return true; // ��ȭ ���� ���� ��ȯ
}

// =============================================================================
void ImageProcessor::RecordThreadFunction(double frameRate)
{
    std::chrono::milliseconds frameDuration(static_cast<int>(1000 / frameRate));

    while (GetStartRecordingFlag())
    {
        auto start = std::chrono::high_resolution_clock::now();  // ���� �ð� ���

        Mat frameToWrite;
        {
            std::lock_guard<std::mutex> lock(videomtx);
            if (!m_videoMat.empty())
            {
                frameToWrite = m_videoMat.clone(); // ���� ������ ����
            }
        }

        // ����� �������� ����Ͽ� ���
        if (!frameToWrite.empty())
        {
            std::lock_guard<std::mutex> writerLock(writermtx);
            videoWriter.write(frameToWrite);
        }

        auto end = std::chrono::high_resolution_clock::now();  // ���� �ð� ���
        auto elapsed = end - start;
        auto timeToWait = frameDuration - elapsed;

        // ���� �����ӱ��� ���
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
        // �̹��� ������ ����
        cv::Mat copiedImage;
        imageMat.copyTo(copiedImage);

        // �̹����� 32��Ʈ�� ��ȯ (4ä��)
        cv::Mat convertedImage;
        copiedImage.convertTo(convertedImage, CV_32FC1, 1.0 / 65535.0); // 16��Ʈ���� 32��Ʈ�� ��ȯ

        // �̹��� ũ�� ���
        imageSize = w * h * 4;

        // ���ο� BITMAPINFO �Ҵ�
        BITMAPINFO* m_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)]);
        nbiClrUsed = 0; // ���� �ȷ�Ʈ�� ������� ����

        // �̹��� ������ ���� �� ��ȯ
        unsigned char* dstData = (unsigned char*)((BITMAPINFO*)m_BitmapInfo)->bmiColors;
        for (int i = 0; i < w * h; ++i)
        {
            float pixel32 = convertedImage.at<float>(i);
            unsigned char pixelByte = static_cast<unsigned char>(pixel32 * 255.0f);
            dstData[i * 4] = pixelByte; // Blue ä��
            dstData[i * 4 + 1] = pixelByte; // Green ä��
            dstData[i * 4 + 2] = pixelByte; // Red ä��
            dstData[i * 4 + 3] = 255; // Alpha ä�� (255: ���� ������)
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
 * @brief �־��� 16���� ���ڿ� ���� �ȷ�Ʈ�� BGR (Blue, Green, Red) ������ ���ͷ� ��ȯ�մϴ�.
 *
 * �� �Լ��� 16���� ���ڿ� ������ ���� �ȷ�Ʈ�� �޾ƿ� �� ������ BGR ������ ���ͷ� ��ȯ�մϴ�.
 * ��ȯ�� ����� ĳ�ø� ����Ͽ� �޸𸮿� ���� ����� ���̱� ���� ����˴ϴ�.
 * �̹� ��ȯ�� ���� �ȷ�Ʈ�� ���� ��û�� ĳ�ÿ��� �˻��Ͽ� ������ ����ȭ�մϴ�.
 *
 * @param hexPalette 16���� ���ڿ� ������ ���� �ȷ�Ʈ. ���� ���, "#FF0000"�� �������� ��Ÿ���ϴ�.
 * @return BGR ������ ���ͷ� ��ȯ�� ���� �ȷ�Ʈ.
 *
 * @note ��ȯ�� �ȷ�Ʈ�� ĳ�ø� ����Ͽ� ����Ǹ�, �̹� ��ȯ�� ��� ĳ�ÿ��� ����� ��ȯ�մϴ�.
 *       �̸� ���� �ߺ� ��ȯ �� ���� ����� ȿ�������� �����մϴ�.
 *
 * @warning �� �Լ��� ĳ�ø� ����ϱ� ������ �Է� ���� �ȷ�Ʈ�� ������ ����Ǹ�
 *          ����� ����ġ ���ϰ� �޶��� �� �����Ƿ� �����ؾ� �մϴ�.
 */
 // =============================================================================
std::vector<cv::Vec3b> ImageProcessor::convertPaletteToBGR(const std::vector<std::string>& hexPalette)
{
    // �̹� ��ȯ�� ����� ������ ĳ��
    static std::map<std::vector<std::string>, std::vector<cv::Vec3b>> cache; // std::map���� ����

    // �Է� ���Ϳ� ���� ĳ�� Ű ����
    std::vector<std::string> cacheKey = hexPalette;

    // ĳ�ÿ��� �̹� ��ȯ�� ����� ã�� ��ȯ
    if (cache.find(cacheKey) != cache.end())
    {
        return cache[cacheKey];
    }

    // ��ȯ �۾� ����
    std::vector<cv::Vec3b> bgrPalette;
    for (const std::string& hexColor : hexPalette)
    {
        cv::Vec3b bgrColor = hexStringToBGR(hexColor);
        bgrPalette.push_back(bgrColor);
    }

    // ��ȯ ����� ĳ�ÿ� ����
    cache[cacheKey] = bgrPalette;

    // ĳ�ÿ� ����� ���纻�� ��ȯ
    return cache[cacheKey];
}

// =============================================================================
cv::Vec3b ImageProcessor::findBrightestColor(const std::vector<cv::Vec3b>& colors)
{
    // ���� ���� ������ ������ ���� �� �ʱ�ȭ
    cv::Vec3b brightestColor = colors[0];

    // ���� ���� ������ ��� �� ��� (B + G + R)
    int maxBrightness = brightestColor[0] + brightestColor[1] + brightestColor[2];

    // ��� ������ �ݺ��ϸ鼭 ���� ���� ���� ã��
    for (const cv::Vec3b& color : colors)
    {
        // ���� ������ ��� ���
        int brightness = color[0] + color[1] + color[2];

        // ���� ���� ���󺸴� �� ������ ������Ʈ
        if (brightness > maxBrightness) 
        {
            maxBrightness = brightness;
            brightestColor = color;
        }
    }

    // ���� ���� ���� ��ȯ
    return brightestColor;
}

// =============================================================================
cv::Vec3b ImageProcessor::findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor)
{
    // ���� ����� ������ ������ ���� �� �ʱ�ȭ
    cv::Vec3b closestColor = colorPalette[0];

    // �ּ� �Ÿ� �ʱ�ȭ
    int minDistance = (int)cv::norm(targetColor - colorPalette[0]);

    // ��� �ȷ�Ʈ ������ �ݺ��ϸ鼭 ���� ����� ���� ã��
    for (const cv::Vec3b& color : colorPalette)
    {
        // ���� ����� ��� ���� ������ �Ÿ� ���
        int distance = (int)cv::norm(targetColor - color);

        // ��������� �ּ� �Ÿ����� �� ������ ������Ʈ
        if (distance < minDistance) {
            minDistance = distance;
            closestColor = color;
        }
    }

    // ���� ����� ���� ��ȯ
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

        Set16BitType(true); //8��Ʈ ����
        MainDlg->m_chEventsCheckBox.EnableWindow(FALSE); //8��Ʈ ����
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
// ROI ���� ��ȿ�� üũ
bool ImageProcessor::IsRoiValid(const cv::Rect& roi)
{
    return roi.width > 1 && roi.height > 1;
}

// =============================================================================
// �÷��� ������ �ɹ������� ����
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
//�̹��� ����ȭ
template <typename T>
cv::Mat ImageProcessor::NormalizeAndProcessImage(const T* data, int height, int width, int cvType)
{
    if (data == nullptr)
    {
        // data�� nullptr�� ��� ����ó�� �Ǵ� �⺻�� ������ ������ �� �ֽ��ϴ�.
        // ���⼭�� �⺻������ �� �̹����� ��ȯ�մϴ�.
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
// ROI ��ü �׸��� �Լ�
void ImageProcessor::DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels)
{
    std::lock_guard<std::mutex> lock(drawmtx);

    int markerSize = 15; // ��Ŀ�� ũ�� ����
    cv::Scalar redColor(0, 0, 0); // ������
    cv::Scalar blueColor(255, 255, 0); // �Ķ���
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

    // �ؽ�Ʈ�� �̹����� �����մϴ�.
    cstrValue.Format(_T(" [%d]"), m_MaxSpot.tempValue);
    strValue = std::string(CT2CA(cstrValue));
    cv::putText(imageMat, strValue, maxcrossCenter, cv::FONT_HERSHEY_PLAIN, 1.5, roiColor, 1, cv::LINE_AA);
    cv::drawMarker(imageMat, maxcrossCenter, roiColor, cv::MARKER_TRIANGLE_UP, markerSize, 2);
}

// =============================================================================
//�̹��� �����Ϳ� ���� �ķ�Ʈ ����
cv::Mat ImageProcessor::MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette)
{
    // Input �̹����� CV_16UC1 �̴�
    cv::Mat normalizedImage(inputImage.size(), CV_16UC1); // ����ȭ�� 16��Ʈ �̹���

    // input �̹��� ������ �ּҰ��� �ִ밪 �˻�.
    double minVal, maxVal;
    cv::minMaxLoc(inputImage, &minVal, &maxVal);

    // [0, 65535] ������ ����ȭ
    inputImage.convertTo(normalizedImage, CV_16UC1, 65535.0 / (maxVal - minVal), -65535.0 * minVal / (maxVal - minVal));

    // �÷� �� ������ �Ϲ������� 8��Ʈ �̹����� ���ǹǷ�,
    // ���⿡���� ����ȭ�� �̹����� �ӽ÷� 8��Ʈ�� ��ȯ�Ͽ� �÷� ���� ����.
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

    // ���̾� 1/2/1
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

    // UYVY ������ �ٽ� Mat���� ��ȯ (CV_8UC2 -> CV_8UC4)
    cv::Mat uyvyImage(height, width, CV_8UC2, input.data);
    cv::Mat bgrImage(height, width, CV_8UC3);  // BGR �������� ��� �̹��� ����

    // UYVY���� BGR�� ��ȯ
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

    cv::Mat bgrImage(height, width, CV_8UC3);  // BGR �������� ��� �̹��� ����

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
    // ��ü ������ ũ�� ���
    int imageSize = nWidth * nHeight;

    // imageSize ũ���� uint16_t �迭�� �������� �Ҵ��մϴ�.
    std::unique_ptr<uint16_t[]> imageArray = std::make_unique<uint16_t[]>(imageSize);
    for (int i = 0; i < imageSize; ++i)
    {
        uint16_t nRawdata = static_cast<uint16_t>(byteArr[i * 2] | (byteArr[i * 2 + 1] << 8));
        imageArray[i] = (nRawdata * GetScaleFactor()) - FAHRENHEIT;

    }
    //CString strLog;
    //OutputDebugString(strLog);

    return imageArray; // std::unique_ptr ��ȯ
}

// =============================================================================
// ������ ������ ������ ó��
std::unique_ptr<uint16_t[]> ImageProcessor::extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight)
{
    // roiWidth�� roiHeight ũ���� uint16_t �迭�� �������� �Ҵ��մϴ�.
    int nArraySize = roiWidth * roiHeight;
    std::unique_ptr<uint16_t[]> roiArray = std::make_unique<uint16_t[]>(nArraySize);

    int k = 0;

    for (int y = nStartY; y < nEndY; y++)
    {
        int rowOffset = y * nWidth;
        for (int x = nStartX; x < nEndX; x++)
        {
            int index = rowOffset + x;

            // �Է� �������� ��ȿ�� �˻�

            // �����͸� �а� ���� ���İ� ��ġ�ϴ��� Ȯ��
            uint16_t sample = static_cast<uint16_t>(byteArr[index * 2] | (byteArr[index * 2 + 1] << 8));
            roiArray[k++] = sample;
        }
    }
    //CString logMessage;
    //logMessage.Format(_T("roisize = %d"), k);
    //OutputDebugString(logMessage);

    return roiArray; // std::unique_ptr ��ȯ
}

// =============================================================================
// MinMax ���� �ʱ�ȭ
bool ImageProcessor::InitializeTemperatureThresholds()
{
    // min, max ������ reset
    m_Max = 0;
    m_Min = 65535;
    m_Max_X = 0;
    m_Max_Y = 0;
    m_Min_X = 0;
    m_Min_Y = 0;

    return true;
}

// =============================================================================
// ī�޶� ������ �����ϰ� ����
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
// ROI ������ ������ ����
void ImageProcessor::ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx)
{
    int absoluteX = nCurrentX + roi.x; // ROI ��� ��ǥ�� ���� ��ǥ�� ��ȯ
    int absoluteY = nCurrentY + roi.y; // ROI ��� ��ǥ�� ���� ��ǥ�� ��ȯ

    // �ִ� �ּ� �µ� üũ �� ���
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
// 16��Ʈ ������ �ּ�,�ִ밪 ����
void ImageProcessor::ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi)
{
    double dScale = GetScaleFactor();
    InitializeTemperatureThresholds();


    int nPointIdx = 0; // ����Ʈ �ε��� �ʱ�ȭ

    for (int y = 0; y < roi.height; y++)
    {
        for (int x = 0; x < roi.width; x++)
        {
            int dataIndex = y * roi.width + x; // ROI �������� �ε��� ���
            ushort tempValue = static_cast<ushort>(data[dataIndex]);

            ROIXYinBox(tempValue, dScale, x, y, roi, nPointIdx);

            nPointIdx++; // ����Ʈ �ε��� ����
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
            // �ȼ��� ��Ʈ �� (bpp)
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
            // �ȼ��� ��Ʈ �� (bpp)
            nbiClrUsed = 0;
        }

        BITMAPINFOHEADER& bmiHeader = m_BitmapInfo->bmiHeader;

        bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiHeader.biWidth = w;
        bmiHeader.biHeight = -h;  // �̹����� �Ʒ����� ���� ǥ��
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = bpp;
        bmiHeader.biCompression = BI_RGB;
        bmiHeader.biSizeImage = imageSize; // �̹��� ũ�� �ʱ�ȭ
        bmiHeader.biClrUsed = nbiClrUsed;   // ���� �ȷ�Ʈ ����

        return m_BitmapInfo;
    }
    catch (const std::bad_alloc&)
    {
        delete m_BitmapInfo; // �޸� ����
        m_BitmapInfo = nullptr;
        throw std::runtime_error("Failed to allocate memory for BITMAPINFO");
    }
}

// =============================================================================
// Mat data to BITMAPINFO
// ���̺� �̹��� �������� �ּ�ȭ�ϱ� ���Ͽ� ���� ���۸� ��� ���
void ImageProcessor::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CClientDC dc(MainDlg->GetDlgItem(nID));
    CRect rect;
    cv::Mat imageCopy = mImage.clone();
    MainDlg->GetDlgItem(nID)->GetClientRect(&rect);

    // �� ���� ����
    CBitmap backBufferBitmap;
    CDC backBufferDC;
    backBufferDC.CreateCompatibleDC(&dc);
    backBufferBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBackBufferBitmap = backBufferDC.SelectObject(&backBufferBitmap);

    // �� ���ۿ� �׸� �׸���
    backBufferDC.SetStretchBltMode(COLORONCOLOR);
    StretchDIBits(backBufferDC.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, imageCopy.cols, imageCopy.rows, imageCopy.data,
        BitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    // ����Ʈ ���۷� ����
    dc.BitBlt(0, 0, rect.Width(), rect.Height(), &backBufferDC, 0, 0, SRCCOPY);

    // ������ ��ü ����
    backBufferDC.SelectObject(pOldBackBufferBitmap);

    // ������ ��ü ����
    backBufferBitmap.DeleteObject();
    backBufferDC.DeleteDC();

    // CClientDC dc(MainDlg->GetDlgItem(nID)) ��ü�� �޸� ������ ����Ű�� �ʴ´ٰ� �����ϰ�, DeleteObject(dc)�� ����.
}

// =============================================================================
//ȭ�鿡 ����� ��Ʈ�� �����͸� ����
void ImageProcessor::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex)
{
    // ��Ʈ�� ���� ����
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
// A50 ī�޶��� ��� ���̸� ������Ʈ�ϴ� �Լ� FFF �����͸� ������ؼ� ������ ��� ������ �� ���;��Ѵ�.
int ImageProcessor::UpdateHeightForA50Camera(int& nHeight, int nWidth)
{
    if (nHeight <= 0 || nWidth <= 0)
    {
        // nHeight�� nWidth�� 0 ������ ��� ó��
        nHeight = 0;
        return nHeight;
    }

    if (m_CameraControl->m_Camlist == A50 && nHeight > 0)
    {
        int heightAdjustment = 1392 / nWidth + (1392 % nWidth ? 1 : 0);

        if (nHeight <= heightAdjustment)
        {
            // ���� ó��
            // nHeight�� heightAdjustment���� �۰ų� ���� ��� ó��  
            // ����� nHeight�� 0���� �����Ͽ� ���� �� ����.
            nHeight = 0;
        }
        else
        {
            nHeight -= heightAdjustment;
        }
    }

    return nHeight;
}