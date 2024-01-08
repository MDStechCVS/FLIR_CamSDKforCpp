
#include "CameraControl_rev.h"
#include "MDS_E-bus_SampleDlg.h"

CameraControl_rev::CameraControl_rev(int nIndex)
    : m_nCamIndex(nIndex), Manager(NULL), m_Cam_Params(new Camera_Parameter)
{
    // ī�޶� ������ ���ν��� ��ü�� �����ϰ� �ʱ�ȭ
    m_Cam_Params->index = nIndex;
    m_Cam_Params->param = this;

    // �������� �ʱ�ȭ
    Initvariable();
    SetCamIndex(m_nCamIndex);

    LoadCaminiFile(nIndex);
    SetPixelFormatParametertoGUI();

    //Rainbow Palette ����
    CreateRainbowPalette();

    // ī�޶� �����带 �����ϰ� ����
    pThread[nIndex] = AfxBeginThread(ThreadCam, m_Cam_Params,
        THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_NEW_PROCESS_GROUP);

    // �����带 �ڵ� �������� �ʵ��� ����
    pThread[nIndex]->m_bAutoDelete = FALSE;
    pThread[nIndex]->ResumeThread();

    m_TStatus = ThreadStatus::THREAD_RUNNING;
}

// =============================================================================
CameraControl_rev::~CameraControl_rev()
{
    // ī�޶� ���� �� ����
    Camera_destroy();
}

// =============================================================================
void CameraControl_rev::CameraManagerLink(CameraManager *_link)
{
    Manager = _link;
}

// =============================================================================
void CameraControl_rev::Initvariable()
{
    CString strLog = _T("");

    // ������ ���� ���� �ʱ�ȭ
    m_bThreadFlag = false;
    m_bStart = false;
    m_bRunning = true;
    m_bReStart = false;
    m_bYUVYFlag = false;

    m_Device = nullptr;
    m_Stream = nullptr;
    m_Pipeline = nullptr;

    m_bStartRecording = false;

    // ī�޶� 1�� FPS �ʱ�ȭ
    m_dCam_FPS = 0;


    m_nCsvFileCount = 1;
    m_colormapType = PaletteTypes::PALETTE_IRON; // �÷��� ����, �ʱⰪ�� JET���� ����

    m_TempData = cv::Mat();
    m_videoMat = cv::Mat();

    bbtnDisconnectFlag = false; // ��ư�� Ŭ���Ͽ� ��ġ ���� �����Ȱ��

    std::string strPath = CT2A(Common::GetInstance()->Getsetingfilepath());
    m_strRawdataPath = strPath + "Camera_" + std::to_string(GetCamIndex() + 1) + "\\";
    SetRawdataPath(m_strRawdataPath);

    Common::GetInstance()->CreateDirectoryRecursively(GetRawdataPath());
    Common::GetInstance()->CreateDirectoryRecursively(GetImageSavePath());
    Common::GetInstance()->CreateDirectoryRecursively(GetRawSavePath());
    Common::GetInstance()->CreateDirectoryRecursively(GetRecordingPath());

    strLog.Format(_T("---------Camera[%d] variable Initialize "), GetCamIndex() + 1);
    Common::GetInstance()->AddLog(0, strLog);

}

// =============================================================================
bool CameraControl_rev::Camera_destroy()
{
    // ī�޶� ��ġ ����
    if (m_Device != nullptr)
    {
        PvDevice::Free(m_Device);
        m_Device = nullptr;
    }

    // ���������� ����
    if (m_Pipeline != nullptr)
    {
        delete m_Pipeline;
        m_Pipeline = nullptr;
    }

    // ��Ʈ�� ����
    if (m_Stream != nullptr)
    {
        PvStream::Free(m_Stream);
        m_Stream = nullptr;
    }

    CString strLog = _T("");
    strLog.Format(_T("---------Camera[%d] destroy---------"), GetCamIndex()+1);
    Common::GetInstance()->AddLog(0, strLog);

    Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    
    return true;
}

// =============================================================================
void CameraControl_rev::CameraDisconnect()
{
    //��ȭ ����
    StopRecording();
    // ī�޶� ����
    CameraStop(GetCamIndex());

    // ��ġ�� ����Ǿ� �ְ� ���� ���� �ƴ� ��
    if (m_Device != nullptr && !m_bRunning)
    {
        // ��ġ�� �̺�Ʈ�� ��� �����ϰ� ���� ����
        PvGenParameterArray* lGenDevice = m_Device->GetParameters();
        lGenDevice->Get(GetCamIndex())->UnregisterEventSink(0);
        SetStartFlag(false);
        m_Device->Disconnect();
        Camera_destroy();
    }
    bbtnDisconnectFlag = true;
    GetMainDialog()->gui_status = GUI_STATUS::GUI_STEP_IDLE;
    
}

// =============================================================================
void CameraControl_rev::CameraSequence(int nIndex)
{
    bool bFlag = false;

    // ī�޶� ��ġ�� ����
    m_Device = ConnectToDevice(nIndex);

    // ī�޶� ��Ʈ�� ����
    m_Stream = OpenStream(nIndex);

    // ī�޶� ��Ʈ�� ����
    ConfigureStream(m_Device, m_Stream, nIndex);

    if (m_Stream != nullptr && m_Device != nullptr)
    {
        // ī�޶� ���������� ����
        m_Pipeline = CreatePipeline(m_Device, m_Stream, nIndex);

        if (m_Pipeline != nullptr || m_Device != nullptr)
        {
            // �Ķ���� ����
            bFlag = AcquireParameter(m_Device, m_Stream, m_Pipeline, nIndex);
        }
        else
        {
            // ���� ó��
        }
    }
}

// =============================================================================
PvDevice* CameraControl_rev::ConnectToDevice(int nIndex)
{
    PvResult lResult = -1;
    CString strLog = _T("");
    PvDevice* device = NULL;

    if (!GetStartFlag())
    {

        Common::GetInstance()->AddLog(0, _T("------------------------------------"));

        // GigE Vision ����̽��� ����
        strLog.Format(_T("[Camera_%d] Connecting.. to device"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        // ī�޶� ��
        CString strModelName = Manager->m_strSetModelName.at(nIndex);
        strLog.Format(_T("[Camera_%d]Model = [%s]"), nIndex + 1, static_cast<LPCTSTR>(strModelName));
        Common::GetInstance()->AddLog(0, strLog);

        PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
        strLog.Format(_T("[Camera_%d] IP Address = [%s]"), nIndex + 1, static_cast<LPCTSTR>(Manager->m_strSetIPAddress.at(nIndex)));
        Common::GetInstance()->AddLog(0, strLog);
        
        device = PvDevice::CreateAndConnect(ConnectionID, &lResult);
    }
    else
    {
        if (device == NULL && GetStartFlag())
        {
            // ��ġ�� �̹� ����Ǿ� �ִ� ���
            return m_Device;
        }
        else
        {
            strLog.Format(_T("[Camera_%d] Connect Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
    }
   
    Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    return device;
}

// =============================================================================
PvStream* CameraControl_rev::OpenStream(int nIndex)
{
    PvStream* lStream = NULL;
    PvResult lResult = -1;
    CString strLog = _T("");
    int nRtyCnt = 0;

    // ��õ� Ƚ���� 3������ ����
    const int maxRetries = 3;

    // ��õ� ����
    while (nRtyCnt <= maxRetries)
    {
        strLog.Format(_T("Opening stream from device"));
        Common::GetInstance()->AddLog(0, strLog);

        PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
        lStream = PvStream::CreateAndOpen(ConnectionID, &lResult);

        if (lStream != nullptr)
        {
            strLog.Format(_T("[Camera_%d] Stream opened successfully."), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);

            Common::GetInstance()->AddLog(0, _T("------------------------------------"));
            return lStream;
        }
        else
        {
            strLog.Format(_T("[Camera_%d] Unable to stream from device. Retrying... %d"), nIndex + 1, nRtyCnt);
            Common::GetInstance()->AddLog(0, strLog);
            nRtyCnt++;

            if (nRtyCnt > maxRetries)
            {
                strLog.Format(_T("[Camera_%d] Unable to stream from device after %d retries. ResultCode = %p"), nIndex + 1, maxRetries, &lResult);
                Common::GetInstance()->AddLog(0, strLog);
            }
        }

        Sleep(500);
    }

    Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    return lStream;
}

// =============================================================================
void CameraControl_rev::ConfigureStream(PvDevice* aDevice, PvStream* aStream, int nIndex)
{
    // GigE Vision ��ġ�� ���, GigE Vision Ư�� ��Ʈ���� �Ķ���͸� ����
    CString strLog = _T("");
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV*>(aDevice);
    if (lDeviceGEV != nullptr)
    {
        PvStreamGEV* lStreamGEV = static_cast<PvStreamGEV*>(aStream);

        // ��Ŷ ũ�� ����
        lDeviceGEV->NegotiatePacketSize();
        strLog.Format(_T("[Camera_%d] Negotiate packet size"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        // ��ġ ��Ʈ���� ��� ����
        lDeviceGEV->SetStreamDestination(lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort());
        strLog.Format(_T("[Camera_%d] Configure device streaming destination"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    }
}

// =============================================================================
PvPipeline* CameraControl_rev::CreatePipeline(PvDevice* aDevice, PvStream* aStream, int nIndex)
{
    CString strLog = _T("");
    PvPipeline* lPipeline = nullptr; // PvPipeline ������ �ʱ�ȭ
    if (aDevice != nullptr)
    {
        uint32_t lSize = aDevice->GetPayloadSize(); // ��ġ�� ���̷ε� ũ�� ȹ��

        if (aStream != nullptr)
        {
            // ��Ʈ���� �����ϴ� ��� ���ο� PvPipeline ����
            lPipeline = new PvPipeline(aStream);
            strLog.Format(_T("[Camera_%d] Pipeline %s"), nIndex + 1, lPipeline ? _T("Create Success") : _T("Create Fail"));
        }
        else if (aDevice != nullptr)
        {
            // ��Ʈ���� ���� ��� �̹� �����ϴ� m_Pipeline Ȱ��
            m_Pipeline->SetBufferCount(BUFFER_COUNT);
            m_Pipeline->SetBufferSize(lSize);
            lPipeline = m_Pipeline;
            strLog.Format(_T("[Camera_%d] Pipeline SetBufferCount, SetBufferSize Success"), nIndex + 1);
        }

        if (lPipeline != nullptr)
        {
            // Pipeline ���� (BufferCount, BufferSize)
            lPipeline->SetBufferCount(BUFFER_COUNT);
            lPipeline->SetBufferSize(lSize);
            Common::GetInstance()->AddLog(0, strLog); // �α� ���
        }
    }

    Common::GetInstance()->AddLog(0, _T("------------------------------------")); // �и� ������ ���� �α� �߰�

    return lPipeline; // ������ �Ǵ� ������ PvPipeline ������ ��ȯ
}

// =============================================================================
bool CameraControl_rev::AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex)
{
    // ��Ʈ������ �����ϴ� �� �ʿ��� ��ġ �Ű����� ��������
    if (aDevice == nullptr || aStream == nullptr || aPipeline == nullptr)
        return false;

    PvGenParameterArray* lDeviceParams = aDevice->GetParameters();
    CString strLog = _T("");
    PvResult result = -1;
    
    // ī�޶� �Ű����� ����
    CameraParamSetting(nIndex, aDevice);

    // GenICam AcquisitionStart �� AcquisitionStop ��� 
    PvGenCommand* lStart = dynamic_cast<PvGenCommand*>(lDeviceParams->Get("AcquisitionStart"));

    result = aPipeline->Reset();

    bool bFlag[5] = { false, };
    result = aPipeline->Start();

    if (!result.IsOK())
    {
        strLog.Format(_T("code = [%p] Pipeline start fail "), &result);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        bFlag[0] = true;
    }

    result = aDevice->StreamEnable();

    if (!result.IsOK())
    {
        strLog.Format(_T("code = [%p] device StreamEnable fail "), &result);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        bFlag[1] = true;
    }

    result = lStart->Execute();

    if (!result.IsOK())
    {
        strLog.Format(_T("code = [%p] device Execute fail "), &result);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        bFlag[2] = true;
    }

    int rt = SetStreamingCameraParameters(lDeviceParams, nIndex, (CameraModelList)m_Camlist);
    if (rt != 0)
    {
        // ���� ���� ó��
        strLog.Format(_T("Error code = [%p][device] Streaming Camera Parameters Set fail "), &result);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        // ���� ���� ó��
        strLog.Format(_T("Streaming Camera Parameters Set Success "));
        Common::GetInstance()->AddLog(0, strLog);
    }


    if (bFlag[0] && bFlag[1] && bFlag[2])
    {
        SetRunningFlag(true);
        SetThreadFlag(true);
        SetStartFlag(true);
        bbtnDisconnectFlag = false;

        return true;
    }

    return false;
    
}

// =============================================================================
// ī�޶� �Ű������� �����ϴ� �Լ�
int CameraControl_rev::SetStreamingCameraParameters(PvGenParameterArray * lDeviceParams, int nIndex, CameraModelList Camlist)
{
    PvResult result = -1;
    CString strLog = _T("");
    CString strIRType = _T("");
    int nIRType = 0;
    //*--Streaming enabled --*/
   // 0 = Radiometric
   // 1 = TemperatureLinear100mK
   // 2 =TemperatureLinear10mK

    switch (Camlist)
    {
    case FT1000:
    case A400:
    case A500:
    case A600:
    case A700:
    case A615:
    case A50:
    
        nIRType = 2;
        result = lDeviceParams->SetEnumValue("IRFormat", nIRType);
        if (nIRType == TEMPERATURELINEAR10MK)
        {
            strIRType.Format(_T("TemperatureLinear10mK"));
        }

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera_%d] Set IRFormat Fail code = %p"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            
            strLog.Format(_T("[Camera_%d] Set IRFormat = %s"), nIndex + 1, (CString)strIRType);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;
    
    case Ax5:
    
        // Set TemperatureLinearMode (1 == on, 0 == off)
        result = lDeviceParams->SetEnumValue("TemperatureLinearMode", 1);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera_%d] Set TemperatureLinearMode Fail code = [%p]"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            strLog.Format(_T("[Camera_%d] Set TemperatureLinearMode on Success "), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }

        // Set TemperatureLinearResolution (low == 0, high == 1)
        result = lDeviceParams->SetEnumValue("TemperatureLinearResolution", 1);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera_%d] Set TemperatureLinearResolution Fail code = %p"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            strLog.Format(_T("[Camera_%d] Set TemperatureLinearResolution high Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;
    case BlackFly:
        strLog.Format(_T("[Camera_%d] Real image camera BlackFly"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        
        break;
    default:
        // �������� �ʴ� ī�޶� ���� ó��
        strLog.Format(_T("[Camera_%d] Unsupported Camera Type: %d"), nIndex + 1, m_Camlist);
        Common::GetInstance()->AddLog(0, strLog);
        return -1; 
    }

    return 0;
}

// =============================================================================
// string to hex
DWORD CameraControl_rev::ConvertHexValue(const CString& hexString)
{
    DWORD hexValue = 0;
    CString strWithoutPrefix = hexString;

    // CString���� '0x' �Ǵ� '0X' ���λ縦 ����
    if (strWithoutPrefix.Left(2).CompareNoCase(_T("0x")) == 0)
        strWithoutPrefix = strWithoutPrefix.Mid(2);

    // 16���� ���ڿ��� ���ڷ� ��ȯ
    _stscanf_s(strWithoutPrefix, _T("%X"), &hexValue);

    return hexValue;
}


//bool CameraControl_rev::CreateDirectoryIfNotExists(const std::string& directoryPath)
//{
//    std::wstring wideString(directoryPath.begin(), directoryPath.end());
//
//    if (!PathIsDirectory(wideString.c_str()))
//    {
//        if (SHCreateDirectoryEx(NULL, wideString.c_str(), NULL) == ERROR_SUCCESS)
//        {
//            std::wcout << L"���͸� ���� ����: " << wideString << std::endl;
//            return true;
//        }
//        else {
//            std::wcerr << L"���͸� ���� ����: " << wideString << std::endl;
//            return false;
//        }
//    }
//}

// =============================================================================
/// ī�޶� ���� �����͸� �����ͼ� �̹��� �İ��� 
void CameraControl_rev::ImageProcessing(PvBuffer* aBuffer, int nIndex)
{
    // ī�޶� ���� �����Ϳ� ���º��� üũ
    if (IsInvalidState(nIndex, aBuffer))
    {
        return;
    }

    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();

    // aBuffer���� �̹����� ������ �����͸� �����´�
    PvImage* lImage = GetImageFromBuffer(aBuffer);

    //�ȼ� ���� ��������
    SetupImagePixelType(lImage);

    // �̹��� �ʱ�ȭ �� ������ ������ ��������
    unsigned char* ptr = GetImageDataPointer(lImage);
    // �̹��� ���� ����
    int nWidth = lImage->GetWidth();
    int nHeight = lImage->GetHeight();

    //a50 ī�޶� ������� FFF���� ���ļ� �޾ƿ��� ���
    //UpdateHeightForA50Camera(nHeight, nWidth);

    int nArraysize = nWidth * nHeight;
    uint16_t* pixelArray = ConvertToUInt16Pointer(ptr);

    if (!pixelArray)
        return;

    // ������Ʈ�� ����
    /*
    PvGenParameterArray* lDeviceParams = m_Device->GetParameters();// ī�޶� �Ű����� ȹ��
    *int64_t RVaule = 0;
    double dBValue = 0, dOValue = 0, dJ1Value = 0, dSpot = 0;
    lDeviceParams->GetIntegerValue("R", RVaule);
    lDeviceParams->GetFloatValue("B", dBValue);
    lDeviceParams->GetFloatValue("O", dOValue);
    lDeviceParams->GetFloatValue("J1", dJ1Value);
    lDeviceParams->GetFloatValue("Spot", dSpot);
    strLog.Format(_T("[Camera_%d] R = %d, Spot = %.2f B = %.2f, O = %.2f, J1 = %.2f"), nIndex + 1, RVaule, dSpot, dBValue, dOValue, dJ1Value);

    if (MainDlg->m_chGenICam_checkBox.GetCheck() && m_Camlist == Ax5)
        Common::GetInstance()->AddLog(0, strLog);
        */


    byte* imageDataPtr = reinterpret_cast<byte*>(lImage->GetDataPointer());

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

        // ROI �̹����� cv::Mat���� ��ȯ

    }
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
    CleanupAfterProcessing(MainDlg, nIndex);
}

// =============================================================================
void CameraControl_rev::ProcessAndRecordFrame(const Mat &processedImageMat, int nWidth, int nHeight)
{
    CString strLog = _T("");

    if (GetStartRecordingFlag())
    {
        if (!m_isRecording && !processedImageMat.empty())
        {
            if (StartRecording(nWidth, nHeight, GetCameraFPS()))
            {
                // StartRecording open ����   
                strLog.Format(_T("---------Camera[%d] Video Start Recording"), GetCamIndex() + 1);
                Common::GetInstance()->AddLog(0, strLog);
            }
            else
            {
                strLog.Format(_T("---------Camera[%d] Video Open Fail"), GetCamIndex() + 1);
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
void CameraControl_rev::UpdateFrame(Mat newFrame) 
{
    std::lock_guard<std::mutex> lock(videomtx);
    m_videoMat = newFrame.clone(); // �� ���������� ������Ʈ
}

// =============================================================================
void CameraControl_rev::putTextOnCamModel(cv::Mat& imageMat)
{
    if (!imageMat.empty()) 
    {
        // CString�� std::string���� ��ȯ�մϴ�.
        CString cstrText = Manager->m_strSetModelName.at(GetCamIndex());
        std::string strText = std::string(CT2CA(cstrText));

        // �ؽ�Ʈ�� �̹����� �����մϴ�.
        cv::putText(imageMat, strText, cv::Point(0, 25), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
    }
}

// =============================================================================
std::unique_ptr<uint16_t[]> CameraControl_rev::extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight)
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
std::unique_ptr<uint16_t[]> CameraControl_rev::extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight)
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
  
    //    
    //logMessage.Format(_T("roisize = %d"), k);
    //OutputDebugString(logMessage);
    
    return roiArray; // std::unique_ptr ��ȯ
}

// =============================================================================
// ī�޶� ������ �����ϰ� ����
bool CameraControl_rev::InitializeTemperatureThresholds()
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
double CameraControl_rev::GetScaleFactor()
{
    double dScale = 0;

    switch (m_Camlist)
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
        case BlackFly :
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
void CameraControl_rev::ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx)
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
void CameraControl_rev::ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi)
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
// mat data *.csv format save
bool CameraControl_rev::WriteCSV(string strPath, Mat mData)
{  
    ofstream myfile;
    myfile.open(strPath.c_str());
    myfile << cv::format(mData, cv::Formatter::FMT_CSV) << std::endl;
    myfile.close();

    return true;
}

// =============================================================================
// ������ ���� �Լ�
UINT CameraControl_rev::ThreadCam(LPVOID _mothod)
{
    Camera_Parameter* item = static_cast<Camera_Parameter*>(_mothod); // ������ �Ű����� ��������
    int nIndex = item->index; // ī�޶� �ε���
    CameraControl_rev* CamClass = static_cast<CameraControl_rev*>(item->param); // ī�޶� ��Ʈ�� Ŭ���� �ν��Ͻ�
    PvBuffer* lBuffer; // PvBuffer ��ü
    double lFrameRateVal = 0.0; // ī�޶� ������ ����Ʈ ���� �����ϴ� ����

    CString strLog = _T(""); // �α� ����� ���� ���ڿ�
    PvResult Result = -1; // PvResult ��ü �ʱ�ȭ

    PvResult lOperationResult = -1; // �۾� ��� ���� �ʱ�ȭ
    CMDS_Ebus_SampleDlg* MainDlg = (CMDS_Ebus_SampleDlg*)item->param; // ���� ��ȭ���� �ν��Ͻ�
    
    bool bFlag = false; // ������ ���� �÷��� �ʱ�ȭ

    while (CamClass->m_TStatus == THREAD_RUNNING) // �����尡 ���� ���� ���� �ݺ�
    {
        bFlag = CamClass->m_bThreadFlag; // ������ �÷��� �� ��������
        if (bFlag)
        {
            if (!CamClass->GetReStartFlag()) // ����� �÷��� Ȯ��
            {
                if (CamClass->m_Stream &&CamClass->m_Stream->IsOpen()) // ��Ʈ���� �����ִ��� Ȯ��
                {
                    lBuffer = nullptr; // PvBuffer ��ü �ʱ�ȭ

                    // ���� ���۸� ��������
                    PvResult lResult = CamClass->m_Pipeline->RetrieveNextBuffer(&lBuffer, 1000, &lOperationResult);
                    if (lResult.IsOK())
                    {
                        if (lBuffer->GetOperationResult() == (PvResult::Code::CodeEnum::OK))
                        {
                            // ������ ������ �۾� ��� Ȯ�� �� ó��
                            // strLog.Format(_T("BlockID : %d "), lBuffer->GetBlockID());
                            // Common::GetInstance()->AddLog(0, strLog);
                        }
                        else
                        {
                            // �۾� ����� OK�� �ƴ� ��� ó��
                            // cout << lDoodle[lDoodleIndex] << " " << lOperationResult.GetCodeString().GetAscii() << "\r";
                        }

                        // ���۸� ���������ο� ��ȯ
                        PvGenParameterArray* lStreamParams = CamClass->m_Stream->GetParameters();
                        PvGenFloat* lFrameRate = dynamic_cast<PvGenFloat*>(lStreamParams->Get("AcquisitionRate"));
                        lFrameRate->GetValue(lFrameRateVal);
                        CamClass->SetCameraFPS(lFrameRateVal);

                        CamClass->m_Pipeline->ReleaseBuffer(lBuffer);

                        if (CamClass->GetRunningFlag() && CamClass->m_TStatus == THREAD_RUNNING)
                            CamClass->ImageProcessing(lBuffer, nIndex);

                        CamClass->SetReStartFlag(false);
                        CamClass->SetRunningFlag(true);
                    }
                    else
                    {
                        // ���� �������� ����
                        strLog.Format(_T("CamIndex[%d] Retrieve buffer Fail code = [%d]"), nIndex + 1, lResult.GetCode());
                        Common::GetInstance()->AddLog(0, strLog);
                        CamClass->SetRunningFlag(true);
                        CamClass->SetReStartFlag(true);
                    }
                    Sleep(100); 
                }              
            }
            else
            {
                // ����� ������
                Sleep(3000);
                strLog.Format(_T("CamIndex[%d] ReStart Sequence"), nIndex + 1);
                Common::GetInstance()->AddLog(0, strLog);
         
                CamClass->ReStartSequence(nIndex);
            }
        }
    }

    return 0;
}

// =============================================================================
// ������ ����
bool CameraControl_rev::DestroyThread(void)
{
    if (m_TStatus == THREAD_RUNNING)
    {
        m_TStatus = THREAD_STOP;
    }

    CString strLog = _T("");
    strLog.Format(_T("Camera[%d] Thread Status = THREAD_STOP"), GetCamIndex());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

// =============================================================================
double CameraControl_rev::GetCameraFPS()
{
    return m_dCam_FPS;
}

// =============================================================================
void CameraControl_rev::SetCameraFPS(double dValue)
{
    m_dCam_FPS = dValue;
}

// =============================================================================
bool CameraControl_rev::CameraStart(int nIndex)
{
    bool bFlag = false;
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CString strLog = _T("");
    strLog.Format(_T("-----------Camera Start------------"));
    Common::GetInstance()->AddLog(0, strLog);

    CameraSequence(nIndex);

    bFlag = true;
    MainDlg->gui_status = GUI_STATUS::GUI_STEP_RUN;

    return bFlag;
}

// =============================================================================
bool CameraControl_rev::CameraStop(int nIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CString strLog = _T("");
    bool bFlag[3] = { false, };
    bool bRtn = false;

    if (m_Device != NULL && m_Stream != NULL && m_Pipeline != NULL)
    {
        bFlag[0] = m_Device->StreamDisable();
        bFlag[1] = m_Device->GetParameters()->ExecuteCommand("AcquisitionStop");
        SetRunningFlag(false);
        SetThreadFlag(false);
        SetStartFlag(true);
    }

    if (bFlag[1] == true && bFlag[0] == true)
    {
        bRtn = true;
        strLog.Format(_T("[Camera_%d] Camera Stop success"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        bRtn = false;
        strLog.Format(_T("[Camera_%d] Camera Stop fail"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    MainDlg->gui_status = GUI_STATUS::GUI_STEP_STOP;
    return bRtn;
}

// =============================================================================
//ȭ�鿡 ����� ��Ʈ�� �����͸� ����
void CameraControl_rev::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex)
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
// BITMAPINFO, BITMAPINFOHEADER  setting
BITMAPINFO* CameraControl_rev::CreateBitmapInfo(const cv::Mat& imageMat, int w, int h, int num_channel)
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
        else if(GetGrayType())
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
void CameraControl_rev::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
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
// ī�޶� ���� ���� �߻� �� �� ���� ������
int CameraControl_rev::ReStartSequence(int nIndex)
{
    if (bbtnDisconnectFlag)
        return -1;

    bool bRtn = false;
    CString strLog = _T("");
    PvResult lResult = -1;
    Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    
    if (m_Device != NULL)
    {
        CameraDisconnect();
        strLog.Format(_T("[Camera_%d][RC]Disconnect."), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
   
    PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
    strLog.Format(_T("[Camera_%d][RC]IP Address = [%s]"), nIndex + 1, Manager->m_strSetIPAddress.at(nIndex));
    Common::GetInstance()->AddLog(0, strLog);

    m_Device = NULL;
    m_Device = m_Device->CreateAndConnect(ConnectionID, &lResult);

    if (lResult.IsOK())
    {
        strLog.Format(_T("[Camera_%d] Connect Success"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
        bRtn = false;

    m_Stream = OpenStream(nIndex);
    if (m_Stream == NULL)
    {
        bRtn = false;
    }

    ConfigureStream(m_Device, m_Stream, nIndex);
    m_Pipeline = CreatePipeline(m_Device, m_Stream, nIndex);

    if (m_Pipeline == NULL)
    {
        bRtn = false;
    }
           
    bRtn = AcquireParameter(m_Device, m_Stream, m_Pipeline, nIndex);
    strLog.Format(_T("Cam[%d][RC] AcquireImages"), nIndex + 1);
    Common::GetInstance()->AddLog(0, strLog);

    if (bRtn)
    {
        SetReStartFlag(false);
        m_TStatus = THREAD_RUNNING;
        strLog.Format(_T("Cam[%d] ReStartSequence success"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        strLog.Format(_T("Cam[%d] ReStartSequence fail"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    
    m_bThreadFlag = true;
  
    strLog.Format(_T("Cam[%d] [RC]m_bThreadFlag = true"), nIndex + 1);
    Common::GetInstance()->AddLog(0, strLog);
   
    Common::GetInstance()->AddLog(0, _T("------------------------------------"));

    return 0;
}

// =============================================================================
void CameraControl_rev::SetReStartFlag(bool bFlag)
{
    m_bReStart = bFlag;
}

// =============================================================================
//�� ���� ������ ���º���
bool CameraControl_rev::GetReStartFlag()
{
    return m_bReStart;
}

// =============================================================================
void CameraControl_rev::SetRunningFlag(bool bFlag)
{
    m_bRunning = bFlag;
}

// =============================================================================
//ī�޶� ������ ���º���
bool CameraControl_rev::GetRunningFlag()
{
    return m_bRunning;
}

// =============================================================================
void CameraControl_rev::SetThreadFlag(bool bFlag)
{
    m_bThreadFlag = bFlag;
}

// =============================================================================
//������ ���� ���º���
bool CameraControl_rev::GetStartFlag()
{
    return m_bStart;
}

// =============================================================================
void CameraControl_rev::SetStartFlag(bool bFlag)
{
    m_bStart = bFlag;
}

// =============================================================================
//������ ������ ���º���
bool CameraControl_rev::GetThreadFlag()
{
    return m_bRunning;
}

// =============================================================================
void CameraControl_rev::SetYUVYType(bool bFlag)
{
    m_bYUVYFlag = bFlag;
}

// =============================================================================
bool CameraControl_rev::GetYUVYType()
{
    return m_bYUVYFlag;
}

// =============================================================================
void CameraControl_rev::SetRGBType(bool bFlag)
{
    m_bRGBFlag = bFlag;
}

// =============================================================================
bool CameraControl_rev::GetRGBType()
{
    return m_bRGBFlag;
}

// =============================================================================
bool CameraControl_rev::GetGrayType()
{
    return m_bGrayFlag;
}

// =============================================================================
void CameraControl_rev::SetGrayType(bool bFlag)
{
    m_bGrayFlag = bFlag;
}

// =============================================================================
bool CameraControl_rev::Get16BitType()
{
    return m_b16BitFlag;
}

// =============================================================================
void CameraControl_rev::Set16BitType(bool bFlag)
{
    m_b16BitFlag = bFlag;
}

// =============================================================================
bool CameraControl_rev::GetColorPaletteType()
{
    return m_bColorFlag;
}

// =============================================================================
void CameraControl_rev::SetColorPaletteType(bool bFlag)
{
    m_bColorFlag = bFlag;
}

// =============================================================================
void CameraControl_rev::SetRawdataPath(std::string path)
{
    m_strRawdataPath = path;
}

// =============================================================================
std::string CameraControl_rev::GetRawdataPath()
{
    return m_strRawdataPath;
}

// =============================================================================
std::string CameraControl_rev::GetImageSavePath()
{
    return GetRawdataPath() + "Image\\";
}

// =============================================================================
std::string CameraControl_rev::GetRawSavePath()
{
    return GetRawdataPath() + "raw\\";
}

// =============================================================================
std::string CameraControl_rev::GetRecordingPath()
{
    return GetRawdataPath() + "recording\\";
}

// =============================================================================
//�Էµ� ī�޶� �𵨸��� ����ü�� �ִ��� Ȯ��
CameraModelList CameraControl_rev::FindCameraModel(int nCamIndex)
{
    CString strModelID = _T("");
    strModelID.Format(_T("%s"), Manager->m_strSetModelName.at(nCamIndex));

    CString sContentsLower = strModelID.MakeLower();

    if (sContentsLower.Find(_T("a50")) != -1)
        return A50;
    else if (sContentsLower.Find(_T("ax5")) != -1)
        return Ax5;
    else if (sContentsLower.Find(_T("ft1000")) != -1)
        return FT1000;
    else if (sContentsLower.Find(_T("blackfly")) != -1)
        return BlackFly;

    return A50; // �⺻��
}

// =============================================================================
//ī�޶� �Ķ���� �����ϱ�
bool CameraControl_rev::CameraParamSetting(int nIndex, PvDevice* aDevice)
{
    PvGenParameterArray* lDeviceParams = aDevice->GetParameters();
    PvResult result = -1;
    CString strLog = _T("");
    bool bFindFlag = false;

    m_Camlist = FindCameraModel(nIndex);

    DWORD pixeltypeValue = ConvertHexValue(m_Cam_Params->strPixelFormat);
    m_pixeltype = (PvPixelType)pixeltypeValue;

    CString strPixelFormat = _T("");
    if (m_Cam_Params->strPixelFormat == MONO8)
    {
        strPixelFormat = "Mono8";
    }
    else if (m_Cam_Params->strPixelFormat == MONO16)
    {
        strPixelFormat = "Mono16";
    }
    else if (m_Cam_Params->strPixelFormat == MONO14)
    {
        strPixelFormat = "Mono14";
    }
    else if (m_Cam_Params->strPixelFormat == YUV422_8_UYVY)
    {
        strPixelFormat = "YUV422_8_UYVY";
    }
    else if (m_Cam_Params->strPixelFormat == RGB8PACK)
    {
        strPixelFormat = "RGB_8_Pack";
    }
    else
    {
        strPixelFormat = "Mono8";
    }

    switch (m_Camlist)
    {
    case FT1000:
    case A400:
    case A500:
    case A600:
    case A700:
    case A615:
    case A50:

        // ī�޶� ���̸� �޾ƿ���
        strLog.Format(_T("[Camera_%d] Cam Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
        Common::GetInstance()->AddLog(0, strLog);
        //a50 pixel format �Ķ���� ����
        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("Set PixelFormat Fail code = %d"), &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera_%d]Set PixelFormat [%s] %s"), nIndex + 1, strPixelFormat, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;

    case Ax5:

        strLog.Format(_T("[Camera_%d] Cam Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
        Common::GetInstance()->AddLog(0, strLog);

        //ax5
        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera_%d] Set PixelFormat Fail code = %d"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera_%d] Set PixelFormat %s"), nIndex + 1, strPixelFormat, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }
   
        //14bit
        result = lDeviceParams->SetEnumValue("DigitalOutput", 3);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera_%d] Set DigitalOutput Fail code = %d"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera_%d] Set DigitalOutput Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;

    case BlackFly:
        strLog.Format(_T("[Camera_%d] Cam Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
        Common::GetInstance()->AddLog(0, strLog);

        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("Set PixelFormat Fail code = %d"), &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera_%d]Set PixelFormat [%s] %s"), nIndex + 1, strPixelFormat, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;


        break;

    default:
        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);
        break;
    }

    return false;
}

// =============================================================================
//���� ���� �ҷ�����
void CameraControl_rev::LoadCaminiFile(int nIndex)
{
    bool bFlag = false;
    
    CString filePath = _T(""), strSetfilepath = _T("");
    CString iniSection = _T(""), iniKey = _T(""), iniValue = _T("");
    TCHAR cbuf[MAX_PATH] = { 0, };
    CString strFileName; 

    strFileName.Format(_T("CameraParams_%d.ini"), nIndex+1);
    filePath.Format(Common::GetInstance()->SetProgramPath(strFileName));

    iniSection.Format(_T("Params"));
    iniKey.Format(_T("PixelType"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    m_Cam_Params->strPixelFormat = iniValue;

    iniKey.Format(_T("Save_interval"));
    m_nSaveInterval = GetPrivateProfileInt(iniSection, iniKey, (bool)false, filePath);

    // YUVY ����ϰ�� üũ�ڽ� Ȱ��ȭ
    if(m_Cam_Params->strPixelFormat == YUV422_8_UYVY)
    {
        SetYUVYType(TRUE);  
        Set16BitType(FALSE);
        SetGrayType(FALSE);
        SetRGBType(FALSE);
    }
    else if (m_Cam_Params->strPixelFormat == MONO8)
    {
        SetGrayType(TRUE);
        SetYUVYType(FALSE);
        Set16BitType(FALSE);
        SetRGBType(FALSE);
    }
    else if (m_Cam_Params->strPixelFormat == MONO16 || m_Cam_Params->strPixelFormat == MONO14)
    {
        Set16BitType(TRUE);
        SetGrayType(TRUE);
        SetYUVYType(FALSE);
        SetRGBType(FALSE);
    }
    else if (m_Cam_Params->strPixelFormat == RGB8PACK)
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
void CameraControl_rev::SetImageSize(Mat Img)
{
    OriMat.copySize(Img);
}

// =============================================================================
cv::Size CameraControl_rev::GetImageSize()
{
    return OriMat.size();
}

// =============================================================================
//�̹��� ����ȭ
template <typename T>
cv::Mat CameraControl_rev::NormalizeAndProcessImage(const T* data, int height, int width, int cvType)
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
void CameraControl_rev::DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels) 
{
    std::lock_guard<std::mutex> lock(drawmtx);
    
    int markerSize = 30; // ��Ŀ�� ũ�� ����
    cv::Scalar redColor(0, 0, 0); // ������
    cv::Scalar blueColor(255, 255, 0); // �Ķ���
    cv::Scalar roiColor = (255, 255, 255);
    cv::rectangle(imageMat, roiRect, blueColor, 2, 8, 0);

    cv::Point mincrossCenter(m_MinSpot.x, m_MinSpot.y);
    cv::Point maxcrossCenter(m_MaxSpot.x, m_MaxSpot.y);
    CString cstrValue;
    std::string strValue;
    cstrValue.Format(_T("[%d]"), m_MinSpot.tempValue);
    strValue = std::string(CT2CA(cstrValue));
    cv::putText(imageMat, strValue, mincrossCenter, cv::FONT_HERSHEY_PLAIN, 1.5, roiColor, 2, cv::LINE_AA);
    cv::drawMarker(imageMat, mincrossCenter, roiColor, cv::MARKER_TRIANGLE_DOWN, markerSize);
    

    // �ؽ�Ʈ�� �̹����� �����մϴ�.
    cstrValue.Format(_T("[%d]"), m_MaxSpot.tempValue);
    strValue = std::string(CT2CA(cstrValue));
    cv::putText(imageMat, strValue, maxcrossCenter, cv::FONT_HERSHEY_PLAIN, 1.5, roiColor, 2, cv::LINE_AA);
    cv::drawMarker(imageMat, maxcrossCenter, roiColor, cv::MARKER_TRIANGLE_UP, markerSize);
 }

// =============================================================================
//�̹��� �����Ϳ� ���� �ķ�Ʈ ����
cv::Mat CameraControl_rev::MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette)
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
        dScaleG = 2;
    }

    // ���̾� 1/2/1
    colorMappedImage = applyIronColorMap(normalizedImage3channel, palette, dScaleR, dScaleG, dScaleB);
    //cv::applyColorMap(colorMappedImage, colorMappedImage, colormap);
    //cv::GaussianBlur(colorMappedImage, smoothedImage, cv::Size(5, 5), 0, 0);
    
    return colorMappedImage;

}
// =============================================================================


cv::Mat CameraControl_rev::ConvertUYVYToBGR8(const cv::Mat& input)
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
cv::Mat CameraControl_rev::Convert16UC1ToBGR8(const cv::Mat& input)
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
// PvBuffer -> PvImage�� ��� �Լ�
PvImage* CameraControl_rev::GetImageFromBuffer(PvBuffer* aBuffer)
{
    PvImage* lImage = aBuffer ? aBuffer->GetImage() : nullptr;
    if (!lImage)
    {
        Common::GetInstance()->AddLog(0, _T("invalid image data"));
    }
    return lImage;
}

// =============================================================================
// �̹����� �ȼ� ������ �����ϴ� �Լ�
void CameraControl_rev::SetupImagePixelType(PvImage* lImage)
{
    DWORD pixeltypeValue = ConvertHexValue(m_Cam_Params->strPixelFormat);
    m_pixeltype = (PvPixelType)pixeltypeValue;
    lImage->Alloc(lImage->GetWidth(), lImage->GetHeight(), PV_PIXEL_WIN_RGB32);
}

// =============================================================================
// A50 ī�޶��� ��� ���̸� ������Ʈ�ϴ� �Լ� FFF �����͸� ������ؼ� ������ ��� ������ �� ���;��Ѵ�.
int CameraControl_rev::UpdateHeightForA50Camera(int& nHeight, int nWidth)
{
    if (nHeight <= 0 || nWidth <= 0)
    {
        // nHeight�� nWidth�� 0 ������ ��� ó��
        nHeight = 0;
        return nHeight;
    }

    if (m_Camlist == A50 && nHeight > 0)
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

// =============================================================================
// Main GUI ���̺� �̹������
cv::Mat CameraControl_rev::DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex)
{
    if (processedImageMat.empty()) 
        return processedImageMat;

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
// �̹��� ó�� �� ���� ���� ��ü �޸� ����
void CameraControl_rev::CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex)
{
    if (m_BitmapInfo != nullptr)
    {
        delete[] m_BitmapInfo;
        m_BitmapInfo = nullptr;
    }
}

// =============================================================================
// �̹��� ó�� �Լ�
cv::Mat CameraControl_rev::ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg)
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
// PvBuffer ��ȿ�� üũ
bool CameraControl_rev::IsValidBuffer(PvBuffer* aBuffer)
{
    return aBuffer && aBuffer->GetImage();
}

// =============================================================================
// �̹��� ���� �� ��ȿ�� üũ
bool CameraControl_rev::IsInvalidState(int nIndex, PvBuffer* buffer)
{
    return nIndex < 0 || m_TStatus != THREAD_RUNNING || !IsValidBuffer(buffer);
}

// =============================================================================
// �� ���� ���̾�α� ������
CMDS_Ebus_SampleDlg* CameraControl_rev::GetMainDialog()
{
    return (CMDS_Ebus_SampleDlg*)AfxGetApp()->GetMainWnd();
}

// =============================================================================
// �̹��� ������ ������
unsigned char* CameraControl_rev::GetImageDataPointer(PvImage* image)
{
    return image ? image->GetDataPointer() : nullptr;
}

// =============================================================================
// unsigned char �����͸� uint16_t �����ͷ� ��ȯ
uint16_t* CameraControl_rev::ConvertToUInt16Pointer(unsigned char* ptr)
{
    return reinterpret_cast<uint16_t*>(ptr);
}

// =============================================================================
// 8��Ʈ�� 16��Ʈ�� ��ȯ
std::unique_ptr<uint16_t[]> CameraControl_rev::Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length)
{

    // roiWidth�� roiHeight ũ���� uint16_t �迭�� �������� �Ҵ��մϴ�.
    std::unique_ptr<uint16_t[]> roiArray = std::make_unique<uint16_t[]>(length);
    for (int i = 0; i < length; i++)
    {
        if (!m_bRunning)
            return nullptr;

        // �����͸� �а� ���� ���İ� ��ġ�ϴ��� Ȯ��
        uint16_t sample = static_cast<uint16_t>(src[i * 2] | (src[i * 2 + 1] << 8));

        roiArray[i] = sample;
    }

    return roiArray;
}

// =============================================================================
// ROI ���� ��ȿ�� üũ
bool CameraControl_rev::IsRoiValid(const cv::Rect& roi)
{
    return roi.width > 1 && roi.height > 1;
}

// =============================================================================
// �÷��� ������ �ɹ������� ����
void CameraControl_rev::SetPaletteType(PaletteTypes type)
{
    m_colormapType = type;
}

// =============================================================================
PaletteTypes CameraControl_rev::GetPaletteType()
{
    return m_colormapType;
}

// =============================================================================
void CameraControl_rev::SetPixelFormatParametertoGUI()
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CString strLog = _T("");
    
    // YUVY
    if(GetYUVYType())
    {
        strLog.Format(_T("[Camera_%d] YUV422_8_UYVY Mode"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        MainDlg->m_chUYVYCheckBox.SetCheck(TRUE);

        Set16BitType(true); //8��Ʈ ����
        MainDlg->m_chEventsCheckBox.EnableWindow(FALSE); //8��Ʈ ����


        MainDlg->m_chColorMapCheckBox.EnableWindow(FALSE);
        MainDlg->m_chMonoCheckBox.EnableWindow(FALSE);

        strLog.Format(_T("[Camera_%d] UYVY CheckBox Checked"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
        strLog.Format(_T("[Camera_%d] 16Bit CheckBox Checked, Disable Windows"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("[Camera_%d] ColorMap  Disable Windows"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("[Camera_%d] Mono  Disable Windows"), GetCamIndex() + 1);
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
    
    //MainDlg->m_chUYVYCheckBox.EnableWindow(FALSE);
    //MainDlg->m_chEventsCheckBox.EnableWindow(FALSE);
}

// =============================================================================
BITMAPINFO* CameraControl_rev::ConvertImageTo32BitWithBitmapInfo(const cv::Mat& imageMat, int w, int h)
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
cv::Vec3b CameraControl_rev::hexStringToBGR(const std::string& hexColor)
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
std::vector<cv::Vec3b> CameraControl_rev::convertPaletteToBGR(const std::vector<std::string>& hexPalette)
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
cv::Vec3b CameraControl_rev::findBrightestColor(const std::vector<cv::Vec3b>& colors)
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
        if (brightness > maxBrightness) {
            maxBrightness = brightness;
            brightestColor = color;
        }
    }

    // ���� ���� ���� ��ȯ
    return brightestColor;
}

// =============================================================================
cv::Vec3b CameraControl_rev::findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor)
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
std::vector<cv::Vec3b> CameraControl_rev::adjustPaletteScale(const std::vector<cv::Vec3b>& originalPalette, double scaleR, double scaleG, double scaleB)
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
cv::Mat CameraControl_rev::applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB)
{
    std::vector<cv::Vec3b> bgr_palette = convertPaletteToBGR(GetPalette(palette));

    // �������� ������ ������ �ȷ�Ʈ ����
    std::vector<cv::Vec3b> adjusted_palette = adjustPaletteScale(bgr_palette, scaleR, scaleG, scaleB);

    m_Markcolor = findBrightestColor(adjusted_palette);
    cv::Vec3b greenColor(0, 0, 255);
    m_findClosestColor = findClosestColor(adjusted_palette, greenColor);

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
std::vector<std::string> CameraControl_rev::CreateRainbowPalette() 
{
    // 256���� ���κ��� ���� �ȷ�Ʈ ����
    for (int i = 0; i < 256; ++i) {
        int r, g, b;

        if (i < 85) {
            r = 255 - i * 3;
            g = 0;
            b = i * 3;
        }
        else if (i < 170) {
            r = 0;
            g = (i - 85) * 3;
            b = 255 - (i - 85) * 3;
        }
        else {
            r = (i - 170) * 3;
            g = 255 - (i - 170) * 3;
            b = 0;
        }

        char color[8];
        snprintf(color, sizeof(color), "#%02X%02X%02X", r, g, b);
        Rainbow_palette.push_back(color);
    }

    return Rainbow_palette;
}

// =============================================================================
std::vector<std::string> CameraControl_rev::GetPalette(PaletteTypes paletteType)
{
    switch (paletteType)
    {
    case PALETTE_IRON:
        return iron_palette;
    case PALETTE_RAINBOW:
        return Rainbow_palette;
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

std::string CameraControl_rev::GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension)
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

bool CameraControl_rev::SaveImageWithTimestamp(const cv::Mat& image)
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
    strLog.Format(_T("Completed writeback to image file: %s"), filePathW.c_str());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

bool CameraControl_rev::SaveRawDataWithTimestamp(const cv::Mat& rawData)
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
    strLog.Format(_T("Completed writeback to raw data file: %s"), filePathW.c_str());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

// =============================================================================
void CameraControl_rev::SaveFilePeriodically(cv::Mat& rawdata, cv::Mat& imagedata)
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

    if (elapsedSeconds >= seconds) // 300�� = 5��
    {
        // �߰�: Mat ��ü�� JPEG, tmp ���Ϸ� ����

        SaveRawDataWithTimestamp(rawdata);
        SaveImageWithTimestamp(imagedata);

        m_nCsvFileCount++;
        lastCallTime = currentTime;
    }
}

// =============================================================================
bool CameraControl_rev::StartRecording(int frameWidth, int frameHeight, double frameRate)
{
    std::lock_guard<std::mutex> lock(videomtx);
    CString strLog = _T("");

    if (m_isRecording)
    {
        strLog.Format(_T("---------Camera[%d] Already recording"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);

        return false; // �̹� ��ȭ ���̹Ƿ� false ��ȯ
    }

    // ��ȭ �̹��� ������ ���� ������ ����
    std::thread recordThread(&CameraControl_rev::RecordThreadFunction,this, frameRate);
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
        strLog.Format(_T("---------Camera[%d] VideoWriter for video recording cannot be opened"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
        return false; // ��ȭ ���� ��ȯ
    }
    else
    {
        strLog.Format(_T("---------Camera[%d] Video Writer open success"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    m_isRecording = true;

    return true; // ��ȭ ���� ���� ��ȯ
}

// =============================================================================
void CameraControl_rev::RecordThreadFunction(double frameRate) 
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
void CameraControl_rev::StopRecording() 
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
    strLog.Format(_T("---------Camera[%d] Video Stop Recording"), GetCamIndex() + 1);
    Common::GetInstance()->AddLog(0, strLog);
}

// =============================================================================
void CameraControl_rev::SetStartRecordingFlag(bool bFlag)
{
    m_bStartRecording = bFlag;
}

// =============================================================================
bool CameraControl_rev::GetStartRecordingFlag()
{
    return m_bStartRecording;
}

// =============================================================================
void CameraControl_rev::SetMouseImageSaveFlag(bool bFlag)
{
    m_bMouseImageSave = bFlag;
}
// =============================================================================
bool CameraControl_rev::GetMouseImageSaveFlag()
{
    return m_bMouseImageSave;
}

// =============================================================================
void CameraControl_rev::SetCamIndex(int nIndex)
{
    m_nCamIndex = nIndex;
}

// =============================================================================
int CameraControl_rev::GetCamIndex()
{
    return m_nCamIndex;
}

