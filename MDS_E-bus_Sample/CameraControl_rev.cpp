
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
    LoadCaminiFile(nIndex);

    // 카메라 스레드를 생성하고 시작
    pThread[nIndex] = AfxBeginThread(ThreadCam, m_Cam_Params,
        THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_NEW_PROCESS_GROUP);

    // 스레드를 자동 삭제하지 않도록 설정
    pThread[nIndex]->m_bAutoDelete = FALSE;
    pThread[nIndex]->ResumeThread();

    m_TStatus = (ThreadStatus)THREAD_RUNNING;
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
    // 스레드 관련 변수 초기화
    m_bThreadFlag = false;
    m_bStart = false;
    m_bRunning = true;
    m_bReStart = false;
    m_Device = nullptr;
    m_Stream = nullptr;
    m_Pipeline = nullptr;

    // 카메라 1의 FPS 초기화
    m_dCam_FPS = 0;

    m_colormapType = (ColormapTypes)COLORMAP_JET; // 컬러맵 변수, 초기값은 JET으로 설정
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
    strLog.Format(_T("---------Camera[%d] destroy---------"), m_nCamIndex+1);
    Common::GetInstance()->AddLog(0, strLog);

    Common::GetInstance()->AddLog(0, _T("------------------------------------"));

    return true;
}

// =============================================================================
void CameraControl_rev::CameraDisconnect()
{
    // 카메라 정지
    CameraStop(m_nCamIndex);

    // 장치가 연결되어 있고 실행 중이 아닐 때
    if (m_Device != nullptr && !m_bRunning)
    {
        // 장치의 이벤트를 등록 해제하고 연결 해제
        PvGenParameterArray* lGenDevice = m_Device->GetParameters();
        lGenDevice->Get(m_nCamIndex)->UnregisterEventSink(0);
        SetStartFlag(false);
        m_Device->Disconnect();
        Camera_destroy();
    }

    GetMainDialog()->gui_status = (GUI_STATUS)GUI_STEP_IDLE;
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
        strLog.Format(_T("[Cam_Index_%d] Connecting.. to device"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        // 카메라 모델
        CString strModelName = Manager->m_strSetModelName.at(nIndex);
        strLog.Format(_T("[Cam_Index_%d]Model = [%s]"), nIndex + 1, static_cast<LPCTSTR>(strModelName));
        Common::GetInstance()->AddLog(0, strLog);

        PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
        strLog.Format(_T("[Cam_Index_%d] IP Address = [%s]"), nIndex + 1, static_cast<LPCTSTR>(Manager->m_strSetIPAddress.at(nIndex)));
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
            strLog.Format(_T("[Cam_Index_%d] Connect Success"), nIndex + 1);
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
            strLog.Format(_T("[Cam_Index_%d] Stream opened successfully."), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);

            Common::GetInstance()->AddLog(0, _T("------------------------------------"));
            return lStream;
        }
        else
        {
            strLog.Format(_T("[Cam_Index_%d] Unable to stream from device. Retrying... %d"), nIndex + 1, nRtyCnt);
            Common::GetInstance()->AddLog(0, strLog);
            nRtyCnt++;

            if (nRtyCnt > maxRetries)
            {
                strLog.Format(_T("[Cam_Index_%d] Unable to stream from device after %d retries. ResultCode = %p"), nIndex + 1, maxRetries, &lResult);
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
        strLog.Format(_T("[Cam_Index_%d] Negotiate packet size"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        // 장치 스트리밍 대상 설정
        lDeviceGEV->SetStreamDestination(lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort());
        strLog.Format(_T("[Cam_Index_%d] Configure device streaming destination"), nIndex + 1);
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
            strLog.Format(_T("[Cam_Index_%d] Pipeline %s"), nIndex + 1, lPipeline ? _T("Create Success") : _T("Create Fail"));
        }
        else if (aDevice != nullptr)
        {
            // 스트림이 없는 경우 이미 존재하는 m_Pipeline 활용
            m_Pipeline->SetBufferCount(BUFFER_COUNT);
            m_Pipeline->SetBufferSize(lSize);
            lPipeline = m_Pipeline;
            strLog.Format(_T("[Cam_Index_%d] Pipeline SetBufferCount, SetBufferSize Success"), nIndex + 1);
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


    int rt = SetStreamingCameraParameters(lDeviceParams, nIndex, m_Camlist);
    if (rt != 0)
    {
        // 설정 실패 처리
        strLog.Format(_T("code = [%p] device StreamingCameraParameters fail "), &result);
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
        SetThreadFlag(true);
        SetStartFlag(true);
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

    //*--Streaming enabled --*/
   // 0 = Radiometric
   // 1 = TemperatureLinear100mK
   // 2 =TemperatureLinear10mK

    switch (Camlist)
    {
    case A50:
    {
        int nIRType = 2;
        CString strIRType = _T("");
        result = lDeviceParams->SetEnumValue("IRFormat", nIRType);
        if (nIRType == 2)
        {
            strIRType.Format(_T("TemperatureLinear10mK"));
        }

        if (!result.IsOK())
        {
            strLog.Format(_T("[Cam_Index_%d] Set IRFormat Fail code = %p"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            
            strLog.Format(_T("[Cam_Index_%d] Set IRFormat = %s"), nIndex + 1, strIRType);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;
    }
    case Ax5:
    {
        // Set TemperatureLinearMode (1 == on, 0 == off)
        result = lDeviceParams->SetEnumValue("TemperatureLinearMode", 1);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Cam_Index_%d] Set TemperatureLinearMode Fail code = [%p]"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            strLog.Format(_T("[Cam_Index_%d] Set TemperatureLinearMode on Success "), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }

        // Set TemperatureLinearResolution (low == 0, high == 1)
        result = lDeviceParams->SetEnumValue("TemperatureLinearResolution", 1);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Cam_Index_%d] Set TemperatureLinearResolution Fail code = %p"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            strLog.Format(_T("[Cam_Index_%d] Set TemperatureLinearResolution high Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;
    }
    default:
        // 지원하지 않는 카메라 유형 처리
        strLog.Format(_T("[Cam_Index_%d] Unsupported Camera Type: %d"), nIndex + 1, m_Camlist);
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

// =============================================================================
/// 카메라 버퍼 데이터를 가져와서 이미지 후가공 
void CameraControl_rev::ImageProcessing(PvBuffer* aBuffer, int nIndex)
{
    // 카메라 버퍼 데이터와 상태변수 체크
    if (IsInvalidState(nIndex, aBuffer))
    {
        return;
    }

    CString strLog = _T("");
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

    int nArray = nWidth * nHeight;
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
    strLog.Format(_T("[Cam_Index_%d] R = %d, Spot = %.2f B = %.2f, O = %.2f, J1 = %.2f"), nIndex + 1, RVaule, dSpot, dBValue, dOValue, dJ1Value);

    if (MainDlg->m_chGenICam_checkBox.GetCheck() && m_Camlist == Ax5)
        Common::GetInstance()->AddLog(0, strLog);
        */

    byte* imageDataPtr = reinterpret_cast<byte*>(lImage->GetDataPointer());
    if (MainDlg->m_chEventsCheckBox.GetCheck() && ptr != NULL)
    {
        //8bit data를 16bit로 컨버트한다
        Convert8BitTo16Bit(imageDataPtr, pixelArray, nArray);
    }

    // 생성할 이미지의 채널 수 및 데이터 타입 선택
    int num_channels = (MainDlg->m_chEventsCheckBox.GetCheck()) ? 16 : 8;
    int dataType = (MainDlg->m_chEventsCheckBox.GetCheck()) ? CV_16UC1 : CV_8UC1;

    // 마지막 ROI 좌표 대입
    cv::Rect roi = m_Select_rect;
    uint16_t* roiData = nullptr;

    if (IsRoiValid(roi))
    {
        // ROI 데이터 저장
        roiData = extractROI(imageDataPtr, nArray, nWidth, roi.x, roi.x + roi.width, roi.y, roi.y + roi.height, roi.width, roi.height);

        // ROI 이미지 데이터 처리 및 최소/최대값 업데이트
        ProcessImageData(roiData, roi.width * roi.height);

        // ROI 이미지를 cv::Mat으로 변환
        cv::Mat roiMat(roi.height, roi.width, dataType, roiData);
    }

    // 이미지 노멀라이즈
    cv::Mat processedImageMat = ProcessImageBasedOnSettings(imageDataPtr, nHeight, nWidth, MainDlg);
   
    //ROI 객체 이미지 사이즈 저장
    SetImageSize(processedImageMat);

    //ROI 사각형 영역이 생겼을 경우에만 그린다
    if (IsRoiValid(roi))
    {
        // ROI 사각형 그리기
        DrawRoiRectangle(processedImageMat, m_Select_rect, num_channels);
        //WriteCSV("D:\\Test.csv", processedImageMat);
    }

    int nType = processedImageMat.type();
    if (processedImageMat.empty())
        return;

    // 화면에 라이브 이미지 그리기
    DisplayLiveImage(MainDlg, processedImageMat, nIndex);

    //  작업이 완료된 비트맵 정보를 삭제합니다, 동적으로 할당한 메모리를 정리
    CleanupAfterProcessing(MainDlg, nIndex, roiData);
}

// =============================================================================
// 지정된 영역의 데이터 처리
uint16_t* CameraControl_rev::extractROI(const uint8_t* byteArr, int byteArrLength, int nWidth, int nStartX, int nEndX, int nStartY, int nEndY, int roiWidth, int roiHeight)
{
    uint16_t* roiArr = new uint16_t[roiWidth * roiHeight];
    int k = 0;

    for (int y = nStartY; y < nEndY; y++) {
        int rowOffset = y * nWidth;
        for (int x = nStartX; x < nEndX; x++) {
            int index = rowOffset + x;

            if (index >= 0 && index < nWidth * nEndY && (index * 2 + 1) < byteArrLength) 
            {
                uint16_t sample = static_cast<uint16_t>(byteArr[index * 2] | (byteArr[index * 2 + 1] << 8));
                roiArr[k++] = sample;
            }
        }
    }

    return roiArr;
}

// =============================================================================
// 16비트 데이터 최소,최대값 산출, simd 기법 사용
void CameraControl_rev::ProcessImageData(const uint16_t* data, int size)
{

    __m128 min16_simd = _mm_set1_ps(65535.0f); // 초기값 설정
    __m128 max16_simd = _mm_setzero_ps();      // 초기값 설정

    __m128 scale_vector;

    if (m_Camlist == A50)
    {
        scale_vector = _mm_set1_ps(0.01f);
    }
    else if (m_Camlist == Ax5)
    {
        scale_vector = _mm_set1_ps(0.04f);
    }

    for (int i = 0; i < size / 4; i++)
    {
        // 4개의 데이터를 한 번에 처리
        __m128i sample_simd = _mm_loadu_si128((__m128i*) & data[i * 4]);

        // 16비트 정수를 32비트 부동소수점으로 변환
        __m128 sampleTemp_simd = _mm_cvtepi32_ps(_mm_cvtepu16_epi32(sample_simd));

        // 스케일 벡터를 사용하여 데이터를 변환
        sampleTemp_simd = _mm_mul_ps(sampleTemp_simd, scale_vector);
        sampleTemp_simd = _mm_sub_ps(sampleTemp_simd, _mm_set1_ps(273.15f));

        // 최소/최대 값을 업데이트
        min16_simd = _mm_min_ps(sampleTemp_simd, min16_simd);
        max16_simd = _mm_max_ps(sampleTemp_simd, max16_simd);
    }

    // SIMD 결과값을 추출하여 최소/최대 값 및 인덱스 계산
    float min16_values[4];
    float max16_values[4];
    int min16_indices[4];
    int max16_indices[4];

    _mm_store_ps(min16_values, min16_simd);
    _mm_store_ps(max16_values, max16_simd);

    for (int i = 0; i < 4; i++)
    {
        min16_indices[i] = (size / 4) * i;
        max16_indices[i] = (size / 4) * i;
    }

    // 최종 최소/최대 값 및 인덱스 결정
    float min16_value = min16_values[0];
    float max16_value = max16_values[0];
    int min16_idx = min16_indices[0];
    int max16_idx = max16_indices[0];

    for (int i = 1; i < 4; i++)
    {
        if (min16_values[i] < min16_value)
        {
            min16_value = min16_values[i];
            min16_idx = min16_indices[i];
        }
        if (max16_values[i] > max16_value)
        {
            max16_value = max16_values[i];
            max16_idx = max16_indices[i];
        }
    }

    // m_MinSpot 및 m_MaxSpot 업데이트
    m_MinSpot.pointIdx = min16_idx;
    m_MinSpot.tempValue = (ushort)min16_value;
    m_MaxSpot.pointIdx = max16_idx;
    m_MaxSpot.tempValue = (ushort)max16_value;

}

// =============================================================================
// mat data *.csv format save
void CameraControl_rev::WriteCSV(string filename, Mat m)
{
    ofstream myfile;
    myfile.open(filename.c_str());
    myfile << cv::format(m, cv::Formatter::FMT_CSV) << std::endl;
    myfile.close();

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
                        if (lBuffer->GetOperationResult() == 0)
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
   if(m_TStatus == THREAD_RUNNING)
        m_TStatus = THREAD_STOP;

    CString strLog = _T("");
    strLog.Format(_T("Camera[%d] Thread Status = THREAD_STOP"), m_nCamIndex);
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

    MainDlg->gui_status = GUI_STEP_RUN;

    return bFlag;
}

// =============================================================================
bool CameraControl_rev::CameraStop(int nIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CString strLog = _T("");
    bool bFlag[3] = {false, };
    bool bRtn = false;

    if (m_Device != NULL && m_Stream != NULL && m_Pipeline != NULL)
    {
        bFlag[0] = m_Device->StreamDisable();
        bFlag[1] = m_Device->GetParameters()->ExecuteCommand("AcquisitionStop");
        SetThreadFlag(false);
        SetStartFlag(true);
    }

    if (bFlag[1] == true && bFlag[0] == true)
    {
        bRtn = true;
        strLog.Format(_T("[Cam_Index_%d] Camera Stop success"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        bRtn = false;
        strLog.Format(_T("[Cam_Index_%d] Camera Stop fail"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    MainDlg->gui_status = GUI_STEP_STOP;
    return bRtn;
}

// =============================================================================
// BITMAPINFO, BITMAPINFOHEADER  setting
BITMAPINFO* CameraControl_rev::CreateBitmapInfo(int w, int h, int bpp, int nIndex)
{
    BITMAPINFO* m_BitmapInfo = nullptr;
    int imageSize = -1;
    int nbiClrUsed = 0;

    // Free previously allocated memory if any
    if (m_BitmapInfo != nullptr)
    {
        delete[] reinterpret_cast<BYTE*>(m_BitmapInfo);
        m_BitmapInfo = nullptr;
    }

    try
    {
        if (bpp == 8)
        {
            m_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)]);
            imageSize = w * h;
            nbiClrUsed = 256;
        }
        else if (bpp == 16) // 16 bit
        {
            m_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new BYTE[sizeof(BITMAPINFO) + 65535 * sizeof(RGBQUAD)]);
            imageSize = w * h * 2;
            nbiClrUsed = 65536;
        }
    }
    catch (const std::bad_alloc&)
    {
        throw std::runtime_error("Failed to allocate memory for BITMAPINFO");
    }

    m_BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_BitmapInfo->bmiHeader.biWidth = w;
    m_BitmapInfo->bmiHeader.biHeight = -h;
    m_BitmapInfo->bmiHeader.biPlanes = 1;
    m_BitmapInfo->bmiHeader.biBitCount = bpp;
    m_BitmapInfo->bmiHeader.biCompression = BI_RGB;
    m_BitmapInfo->bmiHeader.biSizeImage = imageSize;
    m_BitmapInfo->bmiHeader.biClrUsed = nbiClrUsed;

    if (bpp == 8)
    {
        std::vector<RGBQUAD> GrayPalette(256);
        std::copy(GrayPalette.begin(), GrayPalette.end(), m_BitmapInfo->bmiColors);
    }
    else
    {
    }

    return m_BitmapInfo;
}

// =============================================================================
// Mat data to BITMAPINFO
// 라이브 이미지 깜박임을 최소화하기 위하여 더블 버퍼링 기법 사용
void CameraControl_rev::DrawImage(Mat mImage, int nID, BITMAPINFO* BitmapInfo)
{     
    CMDS_Ebus_SampleDlg* MainDlg = GetMainDialog();
    CClientDC dc(MainDlg->GetDlgItem(nID));
    CRect rect;

    MainDlg->GetDlgItem(nID)->GetClientRect(&rect);

    // 백 버퍼 생성
    CBitmap backBufferBitmap;
    CDC backBufferDC;
    backBufferDC.CreateCompatibleDC(&dc);
    backBufferBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBackBufferBitmap = backBufferDC.SelectObject(&backBufferBitmap);

    // 백 버퍼에 그림 그리기
    backBufferDC.SetStretchBltMode(COLORONCOLOR);
    StretchDIBits(backBufferDC.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, mImage.cols, mImage.rows, mImage.data, BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    
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
    bool bRtn = false;
    CString strLog = _T("");
    PvResult lResult = -1;
    Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    
    if (m_Device != NULL)
    {
        CameraDisconnect();
        strLog.Format(_T("[Cam_Index_%d][RC]Disconnect."), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
   
    PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
    strLog.Format(_T("[Cam_Index_%d][RC]IP Address = [%s]"), nIndex + 1, Manager->m_strSetIPAddress.at(nIndex));
    Common::GetInstance()->AddLog(0, strLog);

    m_Device = NULL;
    m_Device = m_Device->CreateAndConnect(ConnectionID, &lResult);

    if (lResult.IsOK())
    {
        strLog.Format(_T("[Cam_Index_%d] Connect Success"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
        bRtn = false;

    m_Stream = OpenStream(nIndex);
    if (m_Stream == NULL) bRtn = false;
    ConfigureStream(m_Device, m_Stream, nIndex);
    m_Pipeline = CreatePipeline(m_Device, m_Stream, nIndex);
    if (m_Pipeline == NULL) bRtn = false;
           
    Sleep(1000);

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
//입력된 카메라 모델명이 구조체에 있는지 확인
bool CameraControl_rev::FindCameraModelName(int nCamIndex, CString strModel)
{
    CString strModelID = _T("");
    bool SearchFlag = false;
    strModelID.Format(_T("%s"), Manager->m_strSetModelName.at(nCamIndex));

    CString sContentsLower = strModelID.MakeLower();

    if (-1 != sContentsLower.Find(strModel.MakeLower()))
        SearchFlag = true;

    return SearchFlag;
}

// =============================================================================
//카메라별 설정값 불러오기
bool CameraControl_rev::CameraParamSetting(int nIndex, PvDevice* aDevice)
{
    
    PvGenParameterArray* lDeviceParams = aDevice->GetParameters();
    PvResult result = -1;
    CString strLog = _T("");

    bool bFindFlag = FindCameraModelName(nIndex, _T("a50"));
    
    if (bFindFlag)
        m_Camlist = A50;

    bFindFlag = FindCameraModelName(nIndex, _T("ax5"));

    if (bFindFlag)
        m_Camlist = Ax5;

    bFindFlag = FindCameraModelName(nIndex, _T("ft1000"));

    if (bFindFlag)
        m_Camlist = FT1000;

    DWORD pixeltypeValue = ConvertHexValue(m_Cam_Params->strPixelFormat);
    m_pixeltype = (PvPixelType)pixeltypeValue;

    switch (m_Camlist)
    {
    case A50:

        //a50 

        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("Set PixelFormat Fail code = %d"), &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Cam_Index_%d]Set PixelFormat %s"), nIndex + 1, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }

        break;

    case Ax5:

        //ax5
        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Cam_Index_%d] Set PixelFormat Fail code = %d"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Cam_Index_%d] Set PixelFormat %s"), nIndex + 1, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }

       
        //14bit
        result = lDeviceParams->SetEnumValue("DigitalOutput", 3);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Cam_Index_%d] Set DigitalOutput Fail code = %d"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Cam_Index_%d] Set DigitalOutput Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }

        break;

    case FT1000:
        break;

    default:
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
    cv::Scalar redColor(0, 0, 255);
    cv::Scalar roiColor = (num_channels == 16) ? redColor : redColor;
    cv::rectangle(imageMat, roiRect, redColor, 2, 8, 0);
}

// =============================================================================
//화면에 출력한 비트맵 데이터를 생성
void CameraControl_rev::CreateAndDrawBitmap(CMDS_Ebus_SampleDlg* MainDlg, const cv::Mat& imageMat, int num_channels, int nIndex) 
{
    //int channelBit = num_channels * (imageMat.elemSize() * 8);
    m_BitmapInfo = CreateBitmapInfo(imageMat.cols, imageMat.rows, num_channels, nIndex);

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
//이미지 데이터에 따라 파레트 설정
cv::Mat CameraControl_rev::MapColorsToPalette(const cv::Mat& inputImage, cv::ColormapTypes colormap)
{
    cv::Mat colorMappedImage;
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

    // applyColorMap 함수의 세 번째 인자로 colormap 변수를 사용
    cv::applyColorMap(normalizedImage8bit, colorMappedImage, colormap);


    // colorMappedImage를 다시 16비트 이미지로 변환.
    cv::Mat outputImage(inputImage.size(), CV_16UC1);
    for (int y = 0; y < colorMappedImage.rows; ++y)
    {
        for (int x = 0; x < colorMappedImage.cols; ++x)
        {
            cv::Vec3b mappedColor = colorMappedImage.at<cv::Vec3b>(y, x);
            ushort pixelValue = static_cast<ushort>(mappedColor[0]) << 8 | static_cast<ushort>(mappedColor[1]); // 예: 첫 번째와 두 번째 채널을 사용
            outputImage.at<ushort>(y, x) = pixelValue;
        }
    }

    return outputImage;
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
    lImage->Alloc(lImage->GetWidth(), lImage->GetHeight(), m_pixeltype);
}

// =============================================================================
// A50 카메라의 경우 높이를 업데이트하는 함수 FFF 데이터를 얻기위해서 헤더 영역을 더 얻어와야한다.
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
void CameraControl_rev::DisplayLiveImage(CMDS_Ebus_SampleDlg* MainDlg, cv::Mat& processedImageMat, int nIndex)
{
    if (processedImageMat.empty()) return;

    int num_channels = MainDlg->m_chEventsCheckBox.GetCheck() ? 16 : 8;
    if (MainDlg->m_chEventsCheckBox.GetCheck())
    {
        cv::Mat colorMappedImage = MapColorsToPalette(processedImageMat, GetColormapType());
        CreateAndDrawBitmap(MainDlg, colorMappedImage, num_channels, nIndex);
    }
    else
    {
        CreateAndDrawBitmap(MainDlg, processedImageMat, num_channels, nIndex);
    }
}

// =============================================================================
// 이미지 처리 중 동적 생성 객체 메모리 해제
void CameraControl_rev::CleanupAfterProcessing(CMDS_Ebus_SampleDlg* MainDlg, int nIndex, uint16_t* roiArr)
{
    if (m_BitmapInfo != nullptr)
    {
        delete[] m_BitmapInfo;
        m_BitmapInfo = nullptr;
    }
    free(roiArr);
    //delete[] roiArr;
}

// =============================================================================
// 이미지 처리 함수
cv::Mat CameraControl_rev::ProcessImageBasedOnSettings(byte* imageDataPtr, int nHeight, int nWidth, CMDS_Ebus_SampleDlg* MainDlg)
{
    int dataType = MainDlg->m_chEventsCheckBox.GetCheck() ? CV_16UC1 : CV_8UC1;
    if (MainDlg->m_chEventsCheckBox.GetCheck())
    {
        return NormalizeAndProcessImage(imageDataPtr, nHeight, nWidth, dataType);
    }
    else
    {
        return NormalizeAndProcessImage(imageDataPtr, nHeight, nWidth, CV_8UC1);
    }
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
void CameraControl_rev::Convert8BitTo16Bit(uint8_t* src, ushort*& dest, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (!m_bRunning)
            return;

        dest[i] = (ushort)(src[i * 2] | (src[i * 2 + 1] << 8));
    }
}

// =============================================================================
// ROI 존재 유효성 체크
bool CameraControl_rev::IsRoiValid(const cv::Rect& roi)
{
    return roi.width > 1 && roi.height > 1;
}

// =============================================================================
// 컬러맵 정보를 맴버변수에 저장
void CameraControl_rev::SetColormapType(ColormapTypes type)
{
    m_colormapType = type;
}

// =============================================================================
ColormapTypes CameraControl_rev::GetColormapType()
{
    return m_colormapType;
}
