#include "ImageProcessor.h"

// =============================================================================
ImageProcessor::ImageProcessor(int nIndex, CameraControl_rev* pCameraControl)
    : m_nIndex(nIndex), m_CameraControl(pCameraControl)
{
    // ������
    
    // setting ini ���� �ҷ�����
    LoadCaminiFile(nIndex);

    // �⺻�� ����
    defalutCheckType();

    // �����ʱ�ȭ
    Initvariable_ImageParams();
    //�ȼ� ���信 ���� GUI Status ����
    SetPixelFormatParametertoGUI();

    m_CameraControl = pCameraControl;
}

// =============================================================================
ImageProcessor::~ImageProcessor() 
{
    // �Ҹ��� ���� (�ʿ��� ��� ����)
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

    // ��Ʈ���� �̹��� ���� ����
    int nWidth = lImage->GetWidth();
    int nHeight = lImage->GetHeight();
    int nArraysize = nWidth * nHeight;

    // ������ �̹����� ä�� �� �� ������ Ÿ�� ����
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
    
    // ��Ƽ ROI ���� ó�� �Լ� ȣ��
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

        cv::Mat tempCopy = fulldataMat.clone(); // fulldataMat�� ������ ���ο� ��� ����
        {
            if (m_TempData.empty())
            {
                m_TempData = cv::Mat::zeros(nHeight, nWidth, dataType);
                tempCopy.copyTo(m_TempData); // m_TempData�� �����͸� �����մϴ�.
            }
        }
    }

    // �̹��� ��ֶ�����
    cv::Mat processedImageMat = ProcessImageBasedOnSettings(imageDataPtr, nHeight, nWidth, MainDlg);

    //ROI ��ü �̹��� ������ ����
    SetImageSize(processedImageMat);

    //ROI �簢�� ������ ������ ��쿡�� �׸���
    //if(GetROIEnabled())
    //    DrawAllRoiRectangles(processedImageMat);

    // ī�޶� �𵨸� �»�ܿ� �׸���
    //putTextOnCamModel(processedImageMat);
    
    // ȭ�鿡 ���̺� �̹��� �� ROI �׸���
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

    m_colormapType = PALETTE_TYPE::PALETTE_IRON; // �÷��� ����, �ʱⰪ�� IRON���� ����

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
    GetCam()->m_Cam_Params->strPixelFormat = iniValue;

    iniKey.Format(_T("Save_interval"));
    m_nSaveInterval = GetPrivateProfileInt(iniSection, iniKey, (bool)false, filePath);

    LoadRoisFromIniFile(nIndex);

    // Palette ����
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
// ShapeType�� ���ڿ��� ��ȯ
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
    // YUVY ����ϰ�� üũ�ڽ� Ȱ��ȭ
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
// �� ���� ���̾�α� ������
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
        // CString�� std::string���� ��ȯ�մϴ�.m_CameraControl
        CString cstrText = GetCam()->Manager->m_strSetModelName.at(m_CameraControl->GetCamIndex());
        std::string strText = std::string(CT2CA(cstrText));

        // �ؽ�Ʈ�� �̹����� �����մϴ�.
        cv::putText(imageMat, strText, cv::Point(0, 25), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
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
        if (!GetCam()->GetCamRunningFlag())
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



    // ROI ó�� �Ϸ� ���
    std::unique_lock<std::mutex> lock(m_roiMutex);
    roiProcessedCond.wait(lock, [this]() { return !isProcessingROIs.load(); }); // ROI ó���� �Ϸ�� ������ ���

    if (GetStartRecordingFlag())
    {
        if (!m_isRecording && !processedImageMat.empty())
        {
            // ROI�� �ִٸ� ����
            if (!m_roiResults.empty())
            {
                m_roiResults.clear(); // Clear all stored ROIs
            }

            if (StartRecording(nWidth, nHeight, GetCam()->GetCameraFPS()))
            {
                GetCam()->ClearQueue();
                m_bSaveAsSEQ = true;
                // StartRecording open ����   
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
            // �̹��� �������� ��ȭ ���� �����ͷ� ����
            UpdateFrame(processedImageMat);
        }
    }
    else
    {

    }
}

// =============================================================================
// �̹��� ó�� �� ���� ���� ��ü �޸� ����
void ImageProcessor::CleanupAfterProcessing() 
{

    std::lock_guard<std::mutex> lock(m_bitmapMutex);
    if (m_BitmapInfo != nullptr)
    {
        delete[] reinterpret_cast<BYTE*>(m_BitmapInfo); // �޸� ����
        m_BitmapInfo = nullptr;
        m_BitmapInfoSize = 0;
    }
}

// =============================================================================
std::vector<std::string> ImageProcessor::CreateRainbowPalette()
{
    //int paletteSize = 128; // ���� ���� Ȯ��
    //int segmentLength = paletteSize / 3; // �� ���׸�Ʈ�� ����

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
    //int paletteSize = 1024; // ���� ���� Ȯ��
    //int segmentLength = paletteSize / 3; // �� ���׸�Ʈ�� ����

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
        // �� ������ B, G, R ���� �����ϸ�
        uchar newB = static_cast<uchar>(color[2] * scaleB);
        uchar newG = static_cast<uchar>(color[1] * scaleG);
        uchar newR = static_cast<uchar>(color[0] * scaleR);


        // ���� 255�� �ʰ����� �ʵ��� ����
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
    // �ȷ�Ʈ ��������
    std::vector<cv::Vec3b> Selectedpalette = paletteManager.GetPalette(palette);

    // �������� ������ ������ �ȷ�Ʈ ����
    std::vector<cv::Vec3b> adjusted_palette = adjustPaletteScale(Selectedpalette, scaleR, scaleG, scaleB);
   

    //��� ����
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

// =============================================================================
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
}

// =============================================================================
bool ImageProcessor::StartRecording(int frameWidth, int frameHeight, double frameRate)
{
    CString strLog = _T("");

    if (m_isRecording)
    {
        strLog.Format(_T("---------Camera[%d] Already recording"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        return false; // �̹� ��ȭ ���̹Ƿ� false ��ȯ
    }
    if (GetGrayType())
    {
        SetGrayType(FALSE);
        SetColorPaletteType(TRUE);
    }
    


    m_isRecording = true;

    // ��ȭ �̹��� ������ ���� ������ ����
    std::thread recordThread(&ImageProcessor::RecordThreadFunction, this, frameRate);
    recordThread.detach();  // ������ ������ ���� ����


    bool bSaveType = !GetGrayType(); // GetGrayType()�� true�� false, �ƴϸ� true�� ����

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
        return false; // ��ȭ ���� ��ȯ
    }
    else
    {
        strLog.Format(_T("---------Camera[%d] Video Writer open success"), m_CameraControl->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    */
    

    return true; // ��ȭ ���� ���� ��ȯ
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

    //// �̹� ��ȯ�� ����� ������ ĳ��
    //static std::map<std::vector<std::string>, std::vector<cv::Vec3b>> cache; // std::map���� ����

    //// �Է� ���Ϳ� ���� ĳ�� Ű ����
    //std::vector<std::string> cacheKey = hexPalette;

    //// ĳ�ÿ��� �̹� ��ȯ�� ����� ã�� ��ȯ
    //if (cache.find(cacheKey) != cache.end())
    //{
    //    return cache[cacheKey];
    //}

    //// ��ȯ �۾� ����
    //std::vector<cv::Vec3b> bgrPalette;
    //for (const std::string& hexColor : hexPalette)
    //{
    //    cv::Vec3b bgrColor = hexStringToBGR(hexColor);
    //    bgrPalette.push_back(bgrColor);
    //}

    //// ��ȯ ����� ĳ�ÿ� ����
    //cache[cacheKey] = bgrPalette;

    //// ĳ�ÿ� ����� ���纻�� ��ȯ
    //return cache[cacheKey];
    return std::vector<cv::Vec3b>();
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
        strLog.Format(_T("[Camera[%d]] YUV422_8_UYVY Mode"), GetCam()->GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        MainDlg->m_chUYVYCheckBox.SetCheck(TRUE);

        Set16BitType(true); //8��Ʈ ����
        MainDlg->m_chEventsCheckBox.EnableWindow(FALSE); //8��Ʈ ����
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
// ROI ��ü �׸��� �Լ�
void ImageProcessor::DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels, const ROIResults& roiResult)
{
    cv::Scalar roiColor(51, 255, 51); // ��� (BGR ������ ����)
    // ROI �簢�� �׸���
    cv::rectangle(imageMat, roiRect, roiColor, 2);

}

// =============================================================================
// �ּ� �� �ؽ�Ʈ�� ��Ŀ�� �׸��� ���� �Լ�
void ImageProcessor::DrawMinMarkerAndText(cv::Mat& imageMat, const MDSMeasureMinSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize)
{
    if (imageMat.empty()) 
        return;
    cv::Scalar TextColor(255, 255, 255); // ��� (BGR ������ ����)
    cv::Point center(spot.x, spot.y);
    cv::Point centerText(spot.x, spot.y);
    std::string text = label + " [" + std::to_string(static_cast<int>(spot.tempValue)) + "]";
    if (center.x >= 0 && center.x < imageMat.cols && center.y >= 0 && center.y < imageMat.rows)
    {
        // �ؽ�Ʈ �׸���
        cv::putText(imageMat, text, centerText, cv::FONT_HERSHEY_PLAIN, 1.5, TextColor, 1, cv::LINE_AA);
        // ��Ŀ �׸���
        cv::drawMarker(imageMat, center, color, cv::MARKER_TRIANGLE_DOWN, markerSize, 2);
    }
}

// =============================================================================
// �ִ� �� �ؽ�Ʈ�� ��Ŀ�� �׸��� ���� �Լ�
void ImageProcessor::DrawMaxMarkerAndText(cv::Mat& imageMat, const MDSMeasureMaxSpotValue& spot, const std::string& label, const cv::Scalar& color, int markerSize)
{
    if (imageMat.empty())
        return;

    cv::Scalar TextColor(255, 255, 255); // ��� (BGR ������ ����)
    cv::Point center(spot.x, spot.y);
    std::string text = label + " [" + std::to_string(static_cast<int>(spot.tempValue)) + "]";

    if (center.x >= 0 && center.x < imageMat.cols && center.y >= 0 && center.y < imageMat.rows)
    {
        // �ؽ�Ʈ �׸���
        cv::putText(imageMat, text, center, cv::FONT_HERSHEY_PLAIN, 1.5, TextColor, 1, cv::LINE_AA);
        // ��Ŀ �׸���
        cv::drawMarker(imageMat, center, color, cv::MARKER_TRIANGLE_UP, markerSize, 2);
    }
}

// =============================================================================
void ImageProcessor::DrawAllRoiRectangles(cv::Mat& image) 
{
    int num_channels = image.channels();

    std::lock_guard<std::mutex> roiDrawLock(roiDrawMtx);

    // ���� ROI�� �׸��ϴ�.
    {
        std::lock_guard<std::mutex> roiLock(m_roiMutex);  // Ȯ���� ROI �����͸� ��ȣ�ϱ� ���� ���ؽ��� ����մϴ�.
        cv::Scalar MinColor(255, 0, 0); // ��� (BGR ������ ����)
        cv::Scalar MaxColor(0, 0, 255); // ��� (BGR ������ ����)
        int markerSize = 20; // ��Ŀ�� ũ�� ����

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

    // �ӽ� ROI�� �׸��ϴ�.
    {
        std::lock_guard<std::mutex> tempLock(m_temporaryRoiMutex);  // �ӽ� ROI �����͸� ��ȣ�ϱ� ���� ���ؽ��� ����մϴ�.
        for (const auto& tempRoi : m_temporaryRois)
        {
            DrawRoiShape(image, *tempRoi, num_channels, tempRoi->shapeType);
        }
    }
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
    // �޸� ����
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

    // ���̾� 1/2/1
    colorMappedImage = applyIronColorMap(normalizedImage3channel, palette, dScaleR, dScaleG, dScaleB);



    // �޸� ����
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
        if (m_CameraControl->GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
            imageArray[i] = (nRawdata * GetScaleFactor()) - FAHRENHEIT;
        else
            imageArray[i] = nRawdata;

    }

    CString strLog;
    OutputDebugString(strLog);

    return imageArray; // std::unique_ptr ��ȯ
}

// =============================================================================
// ������ ������ ������ ó��
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
// ROI ������ �̹��� ������ ����
std::unique_ptr<uint16_t[]> ImageProcessor::extractImageData(const cv::Mat& roiMat)
{
    size_t dataSize = roiMat.total();
    auto data = std::make_unique<uint16_t[]>(dataSize);
    memcpy(data.get(), roiMat.data, dataSize * sizeof(uint16_t));
    return data;
}

// =============================================================================
// MinMax ���� �ʱ�ȭ
bool ImageProcessor::InitializeTemperatureThresholds()
{
    return true;
}

// =============================================================================
// ���� ROI ��� �ʱ�ȭ �Լ�
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
// ī�޶� ������ �����ϰ� ����
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
// ROI ������ ������ ����
void ImageProcessor::ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx, ROIResults& roiResult)
{
    int absoluteX = nCurrentX + roi.x;
    int absoluteY = nCurrentY + roi.y;
    CTemperature T;

    // �ּ� �µ� üũ �� ������Ʈ
    if (uTempValue <= roiResult.min_temp)
    {
        roiResult.min_temp = uTempValue;
        roiResult.min_x = absoluteX;
        roiResult.min_y = absoluteY;
        roiResult.minSpot.x = roiResult.min_x;  // ������ X ��ǥ ������Ʈ
        roiResult.minSpot.y = roiResult.min_y;  // ������ Y ��ǥ ������Ʈ
        roiResult.minSpot.pointIdx = nPointIdx; // ����Ʈ �ε��� ����
        roiResult.minSpot.updated = true;

        if (GetCam()->GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
            roiResult.minSpot.tempValue = static_cast<float>(uTempValue) * dScale - FAHRENHEIT;
        else
        {
            T = GetCam()->imgToTemp(uTempValue);
            roiResult.minSpot.tempValue = T.Value(CTemperature::Celsius);

            
        }
    }
    // �ִ� �µ� üũ �� ������Ʈ
    if (uTempValue >= roiResult.max_temp)
    {
        roiResult.max_temp = uTempValue;
        roiResult.max_x = absoluteX;
        roiResult.max_y = absoluteY;
        roiResult.maxSpot.x = roiResult.max_x;  // ������ X ��ǥ ������Ʈ
        roiResult.maxSpot.y = roiResult.max_y;  // ������ Y ��ǥ ������Ʈ
        roiResult.maxSpot.pointIdx = nPointIdx; // ����Ʈ �ε��� ����
        roiResult.maxSpot.updated = true;

        if (GetCam()->GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
            roiResult.maxSpot.tempValue = static_cast<float>(uTempValue) * dScale - FAHRENHEIT;
        else
        {
            T = GetCam()->imgToTemp(uTempValue);
            roiResult.maxSpot.tempValue = T.Value(CTemperature::Celsius);
        }
    }

    // ���� �� ���� ���
    roiResult.span = roiResult.max_temp - roiResult.min_temp;
    roiResult.level = (roiResult.max_temp + roiResult.min_temp) / 2.0;
}



// =============================================================================
// 16��Ʈ ������ �ּ�,�ִ밪 ����
void ImageProcessor::ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, std::unique_ptr<ROIResults>& roiResult, double dScale)
{
    int nPointIdx = 0; // ����Ʈ �ε��� �ʱ�ȭ

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
        bmiHeader.biHeight = -h;  // �̹����� �Ʒ����� ���� ǥ��
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = bpp;
        bmiHeader.biCompression = BI_RGB;
        bmiHeader.biSizeImage = imageSize; // �̹��� ũ�� �ʱ�ȭ
        bmiHeader.biClrUsed = nbiClrUsed;   // ���� �ȷ�Ʈ ����

        // �ʱ�ȭ ���� ���
        std::cout << "BitmapInfo initialized: biSize = " << bmiHeader.biSize << std::endl;

        return bitmapInfo;
    }
    catch (const std::bad_alloc&)
    {
        delete[] reinterpret_cast<BYTE*>(bitmapInfo); // �޸� ����
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
// ���̺� �̹��� �������� �ּ�ȭ�ϱ� ���Ͽ� ���� ���۸� ��� ���
void ImageProcessor::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
{
    // �Է� �̹��� �Ǵ� BITMAPINFO�� ��ȿ���� ���� ��� �Լ��� �����մϴ�.
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

    // ���縦 ���� �̹��� ����
    cv::Mat imageCopy;
    {
        std::lock_guard<std::mutex> lock(drawmtx);
        mImage.copyTo(imageCopy);
    }

    // ���� ���۸��� ���� �غ�
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

    // �̹��� ��Ʈ��Ī
    backBufferDC.SetStretchBltMode(COLORONCOLOR);
    if (GDI_ERROR == StretchDIBits(backBufferDC.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, imageCopy.cols, imageCopy.rows, imageCopy.data, BitmapInfo, DIB_RGB_COLORS, SRCCOPY)) {
        Common::GetInstance()->AddLog(1, _T("StretchDIBits failed."));
    }

    // ���� ���۸��� ����Ͽ� ȭ�鿡 �׸���
    if (!dc.BitBlt(0, 0, rect.Width(), rect.Height(), &backBufferDC, 0, 0, SRCCOPY)) {
        Common::GetInstance()->AddLog(1, _T("BitBlt failed."));
    }

    // ����
    backBufferDC.SelectObject(pOldBackBufferBitmap);
    backBufferBitmap.DeleteObject();
    backBufferDC.DeleteDC();
}

// =============================================================================
//ȭ�鿡 ����� ��Ʈ�� �����͸� ����
void ImageProcessor::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex)
{
    // ������ �Ҵ�� �޸� ����
    CleanupAfterProcessing();

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
bool ImageProcessor::StartSEQRecording(CString strfilePath)
{

    if (m_SEQFile.m_hFile != CFile::hFileNull) 
    {
        m_SEQFile.Close();  // �̹� ���� �ִ� ������ �ִٸ� �ݱ�
    }

    m_SEQfilePath = CT2A(strfilePath);  // ���� ��� ����
    CFileException e;
    // Try to create new file
    // ���� ���� �õ�
    // ���� ���� �õ�
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

    // ����� ���Ե� ���������� ��ȿ�� üũ
    PvBuffer* pBuf = nullptr;
    pBuf = GetCam()->GetBufferFromQueue();
    int Height = 0;
    PvImage* lImage = pBuf->GetImage();
    Height = lImage->GetHeight();

    // ��� �����Ͱ� ���Ե��� ������ �ٷ� �����Ѵ�
    if (Height == 480 || Height == 348)
    {
        StopRecording();
        return;
    }
        
    // ���� ù ���� �� ���� ����� 
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

    // buffer to SEQ ����
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
            Common::GetInstance()->AddLog(0, _T("������ ������ ���� �߻�"));
            return;
        }

        try
        {
            if (!pHeader || !pIndex) {
                Common::GetInstance()->AddLog(0, _T("pHeader, pIndex NULL"));
                return;
            }

            // Write to file
            if (m_SEQFile.m_hFile != CFile::hFileNull)  // ������ ���� �ִ��� Ȯ��
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

                    if (m_SEQFile.m_hFile != CFile::hFileNull)  // ������ ���� �ִ��� Ȯ��
                    {
                        // Write the modified Basic Image data block to file
                        m_SEQFile.Write(pData, pIndex[i].dwDataSize);
                    }
                }
            }
            if (m_SEQFile.m_hFile != CFile::hFileNull)  // ������ ���� �ִ��� Ȯ��
            {
                m_SEQFile.Write(&geom, sizeof(GEOMETRIC_INFO_T));
                m_SEQFile.Write(pSrc, nPixelSize);
                m_iCurrentSEQImage++;
            }
        }
        catch (CFileException* e)
        {
            // ���� �߻� �� �α� �����
            //CString errorMessage;
            //errorMessage.Format(_T("SEQ File Write Fail: %d"), e->m_cause);
            //Common::GetInstance()->AddLog(0, errorMessage);
            return;
        }
    }
}

//=============================================================================
// �ȷ�Ʈ ������ 256���� ���߾� ��������
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
// �۾��ʿ�
void ImageProcessor::DetectAndDisplayPeople(cv::Mat& image, int& personCount, double& detectionRate)
{
    cv::CascadeClassifier fullbody_cascade;

    // ���� ���� ��� ��������
    CString programDir = Common::GetInstance()->GetProgramDirectory();
    CString cascadePath = programDir + _T("\\haarcascade_fullbody.xml");

    // CString�� std::string���� ��ȯ
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
    //cv::equalizeHist(image, image); // ������׷� ��Ȱȭ

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
//�۾� �ʿ�
void ImageProcessor::DisplayPersonCountAndRate(cv::Mat& image, int personCount, double detectionRate)
{
    std::string text = "Count: " + std::to_string(personCount) + " Rate: " + std::to_string(detectionRate) + "%";
    cv::putText(image, text, cv::Point(0, 200), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
}

//=============================================================================
void ImageProcessor::addROI(const cv::Rect& roi)
{
    //std::lock_guard<std::mutex> lock(m_roiMutex);  // m_roiResults�� ���� ������ ����ȭ
    //auto roiResult = std::make_shared<ROIResults>();
    //roiResult->roi = roi;

    //roiResult->nIndex = m_roiResults.size()+1;  // �ε����� �߰��Ǵ� ������� �Ҵ�
    //m_roiResults.push_back(roiResult);


    //CString strLog;
    //strLog.Format(_T("Added ROI: %d [%d, %d, %d, %d]"), roiResult->nIndex, roiResult->roi.x, roiResult->roi.y, roiResult->roi.width, roiResult->roi.height);
    //Common::GetInstance()->AddLog(0, strLog);
}

//=============================================================================
// ROI�� �����ϴ� �Լ�
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
            m_roiResults[nIndex]->needsRedraw = true;  // ���� �÷��� ����
        }
    }
}

//=============================================================================
bool ImageProcessor::canAddROI() const 
{
    return m_roiResults.size() < 4;  // �ִ� 4���� ROI ���
}

//=============================================================================
// �ӽ� ROI ������Ʈ �Լ�
void ImageProcessor::temporaryUpdateROI(const cv::Rect& roi, ShapeType shapeType)
{
    std::lock_guard<std::mutex> lock(m_temporaryRoiMutex);  // ����ȭ�� ���� ���� �ɾ��ݴϴ�.
    if (m_isAddingNewRoi)
    {
        if (m_temporaryRois.empty())
        {
            auto tempRoiResult = std::make_shared<ROIResults>();
            tempRoiResult->roi = roi;
            tempRoiResult->shapeType = shapeType; // ���� ���� ����
            m_temporaryRois.push_back(tempRoiResult);
        }
        else
        {
            m_temporaryRois.back()->roi = roi;
            m_temporaryRois.back()->shapeType = shapeType; // ���� �ӽ� ROI�� ���� ���� ������Ʈ
        }
    }
}

//=============================================================================
// ���� ROI�� Ȯ���ϴ� �Լ�
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
    // �ε����� �߰��Ǵ� ������� �Ҵ�
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
// �̹��� ũ�� ��ȿ�� �˻� �߰�
bool ImageProcessor::IsRoiValid(const cv::Rect& roi, int imageWidth, int imageHeight)
{
    // ROI�� �̹��� ��踦 ����� �ʴ��� �˻�
    return roi.x >= 0 && roi.y >= 0 && (roi.x + roi.width) <= imageWidth && (roi.y + roi.height) <= imageHeight;
}

//=============================================================================
void ImageProcessor::ProcessROIsConcurrently(const byte* imageDataPtr, int imageWidth, int imageHeight)
{
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<ROIResults>> localCopies;

    // ���� ���纻�� �����ϱ� ���� ����� �ɾ� ����ȭ ������ ����
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
                std::lock_guard<std::mutex> lock(m_roiMutex); // ��� ������Ʈ �� ���
                updatedResults.push_back(processedResult);
            }
            });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    // ����� ���� ����Ʈ�� �ݿ�
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
                // ������ ROIResult ��ü�� ������Ʈ�ϵ�, ShapeType�� ����
                updatedResult->shapeType = (*it)->shapeType;
                *it = updatedResult;
            }
        }
        isProcessingROIs = false;
        roiProcessedCond.notify_all();  // ROI ó�� �Ϸ� �˸�
    }
}




//=============================================================================
// Ŭ���� ��ġ�� roi �ε��� �˻�
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

    return -1; // Point�� ���Ե� ROI�� ã�� ���� ���
}

//=============================================================================
bool ImageProcessor::GetROIEnabled()
{
    return m_ROIEnabled;
}

//=============================================================================
void ImageProcessor::DrawRoiShape(cv::Mat& imageMat, const ROIResults& roiResult, int num_channels, ShapeType shapeType, const cv::Scalar& color)
{
    // ����ü�� �ִ� ���� �⺻������ 
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
    // �α׳� ���� ������Ʈ�� ���� �߰� ó��
    CString logMessage;
    logMessage.Format(_T("Shape type changed to: %d"), static_cast<int>(type));
    Common::GetInstance()->AddLog(0, logMessage);

    // ���� ������ ����Ǿ����� �ٸ� �޼ҵ忡 ������ �� �ֽ��ϴ�.
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
    const int maxROIs = 4;  // �ִ� ROI ����

    // ���� ROI ������ �ִ� ������ �ʰ��ϴ��� Ȯ��
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
            // ���̶���Ʈ �������� ����
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
        // ���� �������� �ǵ���
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