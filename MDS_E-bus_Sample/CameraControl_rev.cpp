
#include "CameraControl_rev.h"
#include "MDS_E-bus_SampleDlg.h"

CameraControl_rev::CameraControl_rev(int nIndex)
    : m_nCamIndex(nIndex), Manager(NULL), m_Cam_Params(new Camera_Parameter)
{
    // 카메라 스레드 프로시저 객체를 생성하고 초기화
    m_Cam_Params->index = nIndex;
    m_Cam_Params->param = this;

    // 변수들을 초기화
    Initvariable();
    SetCamIndex(m_nCamIndex);

    LoadCaminiFile(nIndex);
    SetPixelFormatParametertoGUI();

    //Rainbow Palette 생성
    CreateRainbowPalette();

    // 카메라 스레드를 생성하고 시작
    pThread[nIndex] = AfxBeginThread(ThreadCam, m_Cam_Params,
        THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_NEW_PROCESS_GROUP);

    // 스레드를 자동 삭제하지 않도록 설정
    pThread[nIndex]->m_bAutoDelete = FALSE;
    pThread[nIndex]->ResumeThread();

    m_TStatus = ThreadStatus::THREAD_RUNNING;
}

// =============================================================================
CameraControl_rev::~CameraControl_rev()
{
    // 카메라를 종료 및 정리
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

    // 스레드 관련 변수 초기화
    m_bThreadFlag = false;
    m_bStart = false;
    m_bRunning = true;
    m_bReStart = false;
    m_bYUVYFlag = false;

    m_Device = nullptr;
    m_Stream = nullptr;
    m_Pipeline = nullptr;

    m_bStartRecording = false;

    // 카메라 1의 FPS 초기화
    m_dCam_FPS = 0;


    m_nCsvFileCount = 1;
    m_colormapType = PaletteTypes::PALETTE_IRON; // 컬러맵 변수, 초기값은 JET으로 설정

    m_TempData = cv::Mat();
    m_videoMat = cv::Mat();

    bbtnDisconnectFlag = false; // 버튼을 클릭하여 장치 연결 해제된경우

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
    // 카메라 장치 정리
    if (m_Device != nullptr)
    {
        PvDevice::Free(m_Device);
        m_Device = nullptr;
    }

    // 파이프라인 정리
    if (m_Pipeline != nullptr)
    {
        delete m_Pipeline;
        m_Pipeline = nullptr;
    }

    // 스트림 정리
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
    //녹화 중지
    StopRecording();
    // 카메라 정지
    CameraStop(GetCamIndex());

    // 장치가 연결되어 있고 실행 중이 아닐 때
    if (m_Device != nullptr && !m_bRunning)
    {
        // 장치의 이벤트를 등록 해제하고 연결 해제
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

    // 카메라 장치에 연결
    m_Device = ConnectToDevice(nIndex);

    // 카메라 스트림 오픈
    m_Stream = OpenStream(nIndex);

    // 카메라 스트림 설정
    ConfigureStream(m_Device, m_Stream, nIndex);

    if (m_Stream != nullptr && m_Device != nullptr)
    {
        // 카메라 파이프라인 생성
        m_Pipeline = CreatePipeline(m_Device, m_Stream, nIndex);

        if (m_Pipeline != nullptr || m_Device != nullptr)
        {
            // 파라미터 설정
            bFlag = AcquireParameter(m_Device, m_Stream, m_Pipeline, nIndex);
        }
        else
        {
            // 실패 처리
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

        // GigE Vision 디바이스에 연결
        strLog.Format(_T("[Camera_%d] Connecting.. to device"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        // 카메라 모델
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
            // 장치가 이미 연결되어 있는 경우
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

    // 재시도 횟수를 3번으로 제한
    const int maxRetries = 3;

    // 재시도 루프
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
    // GigE Vision 장치인 경우, GigE Vision 특정 스트리밍 파라미터를 설정
    CString strLog = _T("");
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV*>(aDevice);
    if (lDeviceGEV != nullptr)
    {
        PvStreamGEV* lStreamGEV = static_cast<PvStreamGEV*>(aStream);

        // 패킷 크기 협상
        lDeviceGEV->NegotiatePacketSize();
        strLog.Format(_T("[Camera_%d] Negotiate packet size"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        // 장치 스트리밍 대상 설정
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
    PvPipeline* lPipeline = nullptr; // PvPipeline 포인터 초기화
    if (aDevice != nullptr)
    {
        uint32_t lSize = aDevice->GetPayloadSize(); // 장치의 페이로드 크기 획득

        if (aStream != nullptr)
        {
            // 스트림이 존재하는 경우 새로운 PvPipeline 생성
            lPipeline = new PvPipeline(aStream);
            strLog.Format(_T("[Camera_%d] Pipeline %s"), nIndex + 1, lPipeline ? _T("Create Success") : _T("Create Fail"));
        }
        else if (aDevice != nullptr)
        {
            // 스트림이 없는 경우 이미 존재하는 m_Pipeline 활용
            m_Pipeline->SetBufferCount(BUFFER_COUNT);
            m_Pipeline->SetBufferSize(lSize);
            lPipeline = m_Pipeline;
            strLog.Format(_T("[Camera_%d] Pipeline SetBufferCount, SetBufferSize Success"), nIndex + 1);
        }

        if (lPipeline != nullptr)
        {
            // Pipeline 설정 (BufferCount, BufferSize)
            lPipeline->SetBufferCount(BUFFER_COUNT);
            lPipeline->SetBufferSize(lSize);
            Common::GetInstance()->AddLog(0, strLog); // 로그 기록
        }
    }

    Common::GetInstance()->AddLog(0, _T("------------------------------------")); // 분리 구분을 위한 로그 추가

    return lPipeline; // 생성된 또는 설정된 PvPipeline 포인터 반환
}

// =============================================================================
bool CameraControl_rev::AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex)
{
    // 스트리밍을 제어하는 데 필요한 장치 매개변수 가져오기
    if (aDevice == nullptr || aStream == nullptr || aPipeline == nullptr)
        return false;

    PvGenParameterArray* lDeviceParams = aDevice->GetParameters();
    CString strLog = _T("");
    PvResult result = -1;
    
    // 카메라 매개변수 설정
    CameraParamSetting(nIndex, aDevice);

    // GenICam AcquisitionStart 및 AcquisitionStop 명령 
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
        // 설정 실패 처리
        strLog.Format(_T("Error code = [%p][device] Streaming Camera Parameters Set fail "), &result);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        // 설정 성공 처리
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
// 카메라 매개변수를 설정하는 함수
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
        // 지원하지 않는 카메라 유형 처리
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

    // CString에서 '0x' 또는 '0X' 접두사를 제거
    if (strWithoutPrefix.Left(2).CompareNoCase(_T("0x")) == 0)
        strWithoutPrefix = strWithoutPrefix.Mid(2);

    // 16진수 문자열을 숫자로 변환
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
//            std::wcout << L"디렉터리 생성 성공: " << wideString << std::endl;
//            return true;
//        }
//        else {
//            std::wcerr << L"디렉터리 생성 실패: " << wideString << std::endl;
//            return false;
//        }
//    }
//}

// =============================================================================
/// 카메라 버퍼 데이터를 가져와서 이미지 후가공 
void CameraControl_rev::ImageProcessing(PvBuffer* aBuffer, int nIndex)
{
    // 카메라 버퍼 데이터와 상태변수 체크
    if (IsInvalidState(nIndex, aBuffer))
    {
        return;
    }

    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();

    // aBuffer에서 이미지와 데이터 포인터를 가져온다
    PvImage* lImage = GetImageFromBuffer(aBuffer);

    //픽셀 포멧 가져오기
    SetupImagePixelType(lImage);

    // 이미지 초기화 및 데이터 포인터 가져오기
    unsigned char* ptr = GetImageDataPointer(lImage);
    // 이미지 가로 세로
    int nWidth = lImage->GetWidth();
    int nHeight = lImage->GetHeight();

    //a50 카메라 헤더에서 FFF영역 합쳐서 받아오는 방법
    //UpdateHeightForA50Camera(nHeight, nWidth);

    int nArraysize = nWidth * nHeight;
    uint16_t* pixelArray = ConvertToUInt16Pointer(ptr);

    if (!pixelArray)
        return;

    // 라디오매트릭 정보
    /*
    PvGenParameterArray* lDeviceParams = m_Device->GetParameters();// 카메라 매개변수 획득
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

        // ROI 이미지를 cv::Mat으로 변환

    }
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
                // StartRecording open 성공   
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
            // 이미지 프레임을 녹화 전용 데이터로 복사
            UpdateFrame(processedImageMat);
        }
    }
}

// =============================================================================
void CameraControl_rev::UpdateFrame(Mat newFrame) 
{
    std::lock_guard<std::mutex> lock(videomtx);
    m_videoMat = newFrame.clone(); // 새 프레임으로 업데이트
}

// =============================================================================
void CameraControl_rev::putTextOnCamModel(cv::Mat& imageMat)
{
    if (!imageMat.empty()) 
    {
        // CString을 std::string으로 변환합니다.
        CString cstrText = Manager->m_strSetModelName.at(GetCamIndex());
        std::string strText = std::string(CT2CA(cstrText));

        // 텍스트를 이미지에 삽입합니다.
        cv::putText(imageMat, strText, cv::Point(0, 25), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
    }
}

// =============================================================================
std::unique_ptr<uint16_t[]> CameraControl_rev::extractWholeImage(const uint8_t* byteArr, int byteArrLength, int nWidth, int nHeight)
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
std::unique_ptr<uint16_t[]> CameraControl_rev::extractROI(const uint8_t* byteArr, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight)
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
  
    //    
    //logMessage.Format(_T("roisize = %d"), k);
    //OutputDebugString(logMessage);
    
    return roiArray; // std::unique_ptr 반환
}

// =============================================================================
// 카메라 종류별 스케일값 적용
bool CameraControl_rev::InitializeTemperatureThresholds()
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
// ROI 내부의 데이터 연산
void CameraControl_rev::ROIXYinBox(ushort uTempValue, double dScale, int nCurrentX, int nCurrentY, cv::Rect roi, int nPointIdx)
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
void CameraControl_rev::ProcessImageData(std::unique_ptr<uint16_t[]>&& data, int size, int nImageWidth, int nImageHeight, cv::Rect roi)
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
// 쓰레드 폴링 함수
UINT CameraControl_rev::ThreadCam(LPVOID _mothod)
{
    Camera_Parameter* item = static_cast<Camera_Parameter*>(_mothod); // 쓰레드 매개변수 가져오기
    int nIndex = item->index; // 카메라 인덱스
    CameraControl_rev* CamClass = static_cast<CameraControl_rev*>(item->param); // 카메라 컨트롤 클래스 인스턴스
    PvBuffer* lBuffer; // PvBuffer 객체
    double lFrameRateVal = 0.0; // 카메라 프레임 레이트 값을 저장하는 변수

    CString strLog = _T(""); // 로그 출력을 위한 문자열
    PvResult Result = -1; // PvResult 객체 초기화

    PvResult lOperationResult = -1; // 작업 결과 변수 초기화
    CMDS_Ebus_SampleDlg* MainDlg = (CMDS_Ebus_SampleDlg*)item->param; // 메인 대화상자 인스턴스
    
    bool bFlag = false; // 쓰레드 실행 플래그 초기화

    while (CamClass->m_TStatus == THREAD_RUNNING) // 쓰레드가 실행 중인 동안 반복
    {
        bFlag = CamClass->m_bThreadFlag; // 쓰레드 플래그 값 가져오기
        if (bFlag)
        {
            if (!CamClass->GetReStartFlag()) // 재시작 플래그 확인
            {
                if (CamClass->m_Stream &&CamClass->m_Stream->IsOpen()) // 스트림이 열려있는지 확인
                {
                    lBuffer = nullptr; // PvBuffer 객체 초기화

                    // 다음 버퍼를 가져오기
                    PvResult lResult = CamClass->m_Pipeline->RetrieveNextBuffer(&lBuffer, 1000, &lOperationResult);
                    if (lResult.IsOK())
                    {
                        if (lBuffer->GetOperationResult() == (PvResult::Code::CodeEnum::OK))
                        {
                            // 가져온 버퍼의 작업 결과 확인 및 처리
                            // strLog.Format(_T("BlockID : %d "), lBuffer->GetBlockID());
                            // Common::GetInstance()->AddLog(0, strLog);
                        }
                        else
                        {
                            // 작업 결과가 OK가 아닐 경우 처리
                            // cout << lDoodle[lDoodleIndex] << " " << lOperationResult.GetCodeString().GetAscii() << "\r";
                        }

                        // 버퍼를 파이프라인에 반환
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
                        // 버퍼 가져오기 실패
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
                // 재시작 시퀀스
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
// 쓰레드 종료
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
//화면에 출력한 비트맵 데이터를 생성
void CameraControl_rev::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex)
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
            // 픽셀당 비트 수 (bpp)
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
void CameraControl_rev::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
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
// 카메라 연결 끊김 발생 시 재 연결 시퀀스
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
//재 시작 시퀀스 상태변수
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
//카메라 동작중 상태변수
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
//시퀀스 시작 상태변수
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
//쓰레드 동작중 상태변수
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
//입력된 카메라 모델명이 구조체에 있는지 확인
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

    return A50; // 기본값
}

// =============================================================================
//카메라별 파라미터 설정하기
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

        // 카메라 모델이름 받아오기
        strLog.Format(_T("[Camera_%d] Cam Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
        Common::GetInstance()->AddLog(0, strLog);
        //a50 pixel format 파라미터 설정
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
//설정 파일 불러오기
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

    // YUVY 경우일경우 체크박스 활성화
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
//이미지 정규화
template <typename T>
cv::Mat CameraControl_rev::NormalizeAndProcessImage(const T* data, int height, int width, int cvType)
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
void CameraControl_rev::DrawRoiRectangle(cv::Mat& imageMat, const cv::Rect& roiRect, int num_channels) 
{
    std::lock_guard<std::mutex> lock(drawmtx);
    
    int markerSize = 30; // 마커의 크기 설정
    cv::Scalar redColor(0, 0, 0); // 빨간색
    cv::Scalar blueColor(255, 255, 0); // 파란색
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
    

    // 텍스트를 이미지에 삽입합니다.
    cstrValue.Format(_T("[%d]"), m_MaxSpot.tempValue);
    strValue = std::string(CT2CA(cstrValue));
    cv::putText(imageMat, strValue, maxcrossCenter, cv::FONT_HERSHEY_PLAIN, 1.5, roiColor, 2, cv::LINE_AA);
    cv::drawMarker(imageMat, maxcrossCenter, roiColor, cv::MARKER_TRIANGLE_UP, markerSize);
 }

// =============================================================================
//이미지 데이터에 따라 파레트 설정
cv::Mat CameraControl_rev::MapColorsToPalette(const cv::Mat& inputImage, PaletteTypes palette)
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
        dScaleG = 2;
    }

    // 아이언 1/2/1
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

    // UYVY 형식을 다시 Mat으로 변환 (CV_8UC2 -> CV_8UC4)
    cv::Mat uyvyImage(height, width, CV_8UC2, input.data);
    cv::Mat bgrImage(height, width, CV_8UC3);  // BGR 형식으로 결과 이미지 생성

    // UYVY에서 BGR로 변환
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
// PvBuffer -> PvImage를 얻는 함수
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
// 이미지의 픽셀 유형을 설정하는 함수
void CameraControl_rev::SetupImagePixelType(PvImage* lImage)
{
    DWORD pixeltypeValue = ConvertHexValue(m_Cam_Params->strPixelFormat);
    m_pixeltype = (PvPixelType)pixeltypeValue;
    lImage->Alloc(lImage->GetWidth(), lImage->GetHeight(), PV_PIXEL_WIN_RGB32);
}

// =============================================================================
// A50 카메라의 경우 높이를 업데이트하는 함수 FFF 데이터를 얻기위해서 지정된 헤더 영역을 더 얻어와야한다.
int CameraControl_rev::UpdateHeightForA50Camera(int& nHeight, int nWidth)
{
    if (nHeight <= 0 || nWidth <= 0)
    {
        // nHeight나 nWidth가 0 이하인 경우 처리
        nHeight = 0;
        return nHeight;
    }

    if (m_Camlist == A50 && nHeight > 0)
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

// =============================================================================
// Main GUI 라이브 이미지출력
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
// 이미지 처리 중 동적 생성 객체 메모리 해제
void CameraControl_rev::CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex)
{
    if (m_BitmapInfo != nullptr)
    {
        delete[] m_BitmapInfo;
        m_BitmapInfo = nullptr;
    }
}

// =============================================================================
// 이미지 처리 함수
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
// PvBuffer 유효성 체크
bool CameraControl_rev::IsValidBuffer(PvBuffer* aBuffer)
{
    return aBuffer && aBuffer->GetImage();
}

// =============================================================================
// 이미지 가공 전 유효성 체크
bool CameraControl_rev::IsInvalidState(int nIndex, PvBuffer* buffer)
{
    return nIndex < 0 || m_TStatus != THREAD_RUNNING || !IsValidBuffer(buffer);
}

// =============================================================================
// 주 메인 다이얼로그 포인터
CMDS_Ebus_SampleDlg* CameraControl_rev::GetMainDialog()
{
    return (CMDS_Ebus_SampleDlg*)AfxGetApp()->GetMainWnd();
}

// =============================================================================
// 이미지 데이터 포인터
unsigned char* CameraControl_rev::GetImageDataPointer(PvImage* image)
{
    return image ? image->GetDataPointer() : nullptr;
}

// =============================================================================
// unsigned char 포인터를 uint16_t 포인터로 변환
uint16_t* CameraControl_rev::ConvertToUInt16Pointer(unsigned char* ptr)
{
    return reinterpret_cast<uint16_t*>(ptr);
}

// =============================================================================
// 8비트를 16비트로 변환
std::unique_ptr<uint16_t[]> CameraControl_rev::Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length)
{

    // roiWidth와 roiHeight 크기의 uint16_t 배열을 동적으로 할당합니다.
    std::unique_ptr<uint16_t[]> roiArray = std::make_unique<uint16_t[]>(length);
    for (int i = 0; i < length; i++)
    {
        if (!m_bRunning)
            return nullptr;

        // 데이터를 읽고 예상 형식과 일치하는지 확인
        uint16_t sample = static_cast<uint16_t>(src[i * 2] | (src[i * 2 + 1] << 8));

        roiArray[i] = sample;
    }

    return roiArray;
}

// =============================================================================
// ROI 존재 유효성 체크
bool CameraControl_rev::IsRoiValid(const cv::Rect& roi)
{
    return roi.width > 1 && roi.height > 1;
}

// =============================================================================
// 컬러맵 정보를 맴버변수에 저장
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

        Set16BitType(true); //8비트 고정
        MainDlg->m_chEventsCheckBox.EnableWindow(FALSE); //8비트 고정


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
std::vector<cv::Vec3b> CameraControl_rev::convertPaletteToBGR(const std::vector<std::string>& hexPalette)
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
cv::Vec3b CameraControl_rev::findBrightestColor(const std::vector<cv::Vec3b>& colors)
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
        if (brightness > maxBrightness) {
            maxBrightness = brightness;
            brightestColor = color;
        }
    }

    // 가장 밝은 색상 반환
    return brightestColor;
}

// =============================================================================
cv::Vec3b CameraControl_rev::findClosestColor(const std::vector<cv::Vec3b>& colorPalette, const cv::Vec3b& targetColor)
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
std::vector<cv::Vec3b> CameraControl_rev::adjustPaletteScale(const std::vector<cv::Vec3b>& originalPalette, double scaleR, double scaleG, double scaleB)
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
cv::Mat CameraControl_rev::applyIronColorMap(cv::Mat& im_gray, PaletteTypes palette, double scaleR, double scaleG, double scaleB)
{
    std::vector<cv::Vec3b> bgr_palette = convertPaletteToBGR(GetPalette(palette));

    // 스케일을 적용한 조정된 팔레트 생성
    std::vector<cv::Vec3b> adjusted_palette = adjustPaletteScale(bgr_palette, scaleR, scaleG, scaleB);

    m_Markcolor = findBrightestColor(adjusted_palette);
    cv::Vec3b greenColor(0, 0, 255);
    m_findClosestColor = findClosestColor(adjusted_palette, greenColor);

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
std::vector<std::string> CameraControl_rev::CreateRainbowPalette() 
{
    // 256가지 레인보우 색상 팔레트 생성
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
        // 기본값 또는 오류 처리를 수행할 수 있음
        return std::vector<std::string>();
    }
}

std::string CameraControl_rev::GenerateFileNameWithTimestamp(const std::string& basePath, const std::string& prefix, const std::string& extension)
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

bool CameraControl_rev::SaveImageWithTimestamp(const cv::Mat& image)
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
    strLog.Format(_T("Completed writeback to image file: %s"), filePathW.c_str());
    Common::GetInstance()->AddLog(0, strLog);

    return true;
}

bool CameraControl_rev::SaveRawDataWithTimestamp(const cv::Mat& rawData)
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

    if (elapsedSeconds >= seconds) // 300초 = 5분
    {
        // 추가: Mat 객체를 JPEG, tmp 파일로 저장

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

        return false; // 이미 녹화 중이므로 false 반환
    }

    // 녹화 이미지 생성을 위한 쓰레드 시작
    std::thread recordThread(&CameraControl_rev::RecordThreadFunction,this, frameRate);
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
        strLog.Format(_T("---------Camera[%d] VideoWriter for video recording cannot be opened"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
        return false; // 녹화 실패 반환
    }
    else
    {
        strLog.Format(_T("---------Camera[%d] Video Writer open success"), GetCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    m_isRecording = true;

    return true; // 녹화 시작 성공 반환
}

// =============================================================================
void CameraControl_rev::RecordThreadFunction(double frameRate) 
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

