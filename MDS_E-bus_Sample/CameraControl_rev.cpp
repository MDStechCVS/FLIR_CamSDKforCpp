#include "CameraControl_rev.h"
#include "ImageProcessor.h"


CameraControl_rev::CameraControl_rev(int nIndex)
    : Manager(nullptr),
      m_nCamIndex(nIndex),
      m_Cam_Params(new Camera_Parameter)
{
    // 카메라 스레드 프로시저 객체를 생성하고 초기화
    m_Cam_Params->index = nIndex;
    m_Cam_Params->param = this;
    // 변수들을 초기화
    Initvariable();
    SetCamIndex(m_nCamIndex);

    // 카메라 스레드를 생성하고 시작
    StartThreadProc(nIndex);
}

// =============================================================================
CameraControl_rev::~CameraControl_rev()
{
    // 카메라를 종료 및 정리
    Camera_destroy();
}

// =============================================================================
void CameraControl_rev::CameraManagerLink(CameraManager* _link)
{
    Manager = _link;
}

void CameraControl_rev::CreateImageProcessor(int nIndex)
{
    ImgProc = new ImageProcessor(GetCamIndex(), this);
}

ImageProcessor* CameraControl_rev::GetImageProcessor() const 
{
    return ImgProc; // unique_ptr의 get() 메서드를 사용하여 포인터 반환
}

void CameraControl_rev::StartThreadProc(int nIndex)
{
    // 초기화 확인 및 스레드 시작
    pThread[nIndex] = AfxBeginThread(ThreadCam, m_Cam_Params, THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_NEW_PROCESS_GROUP);
    pThread[nIndex]->m_bAutoDelete = FALSE;
    pThread[nIndex]->ResumeThread();
    m_TStatus = ThreadStatus::THREAD_RUNNING;
}

// =============================================================================
void CameraControl_rev::Initvariable()
{
    CString strLog = _T("");

    // 스레드 관련 변수 초기화
    m_bThreadFlag = false;
    m_bStart = false;
    m_bCamRunning = true;
    m_bReStart = false;
    
    m_Device = nullptr;
    m_Stream = nullptr;
    m_Pipeline = nullptr;

    // 카메라 1의 FPS 초기화
    m_dCam_FPS = 0;
    // 버튼을 클릭하여 장치 연결 해제된경우
    bbtnDisconnectFlag = false; 

    strLog.Format(_T("---------Camera[%d] CameraParams Variable Initialize "), GetCamIndex() + 1);
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

    // 카메라 구조체 정리
    delete m_Cam_Params;
    m_Cam_Params = nullptr;

    if (ImgProc != nullptr)
    {
        delete ImgProc;
        ImgProc = nullptr;
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

    // 카메라 정지
    CameraStop(GetCamIndex());

    // 장치가 연결되어 있고 실행 중이 아닐 때
    if (m_Device != nullptr && GetCamRunningFlag() == FALSE)
    {
        // 장치의 이벤트를 등록 해제하고 연결 해제
        PvGenParameterArray* lGenDevice = m_Device->GetParameters();
        lGenDevice->Get(GetCamIndex())->UnregisterEventSink(0);
        SetStartFlag(false);
        m_Device->Disconnect();
        Camera_destroy();
    }
    bbtnDisconnectFlag = true;
    ImgProc->GetMainDialog()->gui_status = (GUI_STATUS)GUI_STEP_IDLE;
    
}

// =============================================================================
bool CameraControl_rev::CameraSequence(int nIndex)
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

    return bFlag;
}

// =============================================================================
PvDevice* CameraControl_rev::ConnectToDevice(int nIndex)
{
    PvResult lResult = -1;
    CString strLog = _T("");
    PvDevice* device = NULL;

    if (GetStartFlag() == FALSE)
    {

        Common::GetInstance()->AddLog(0, _T("------------------------------------"));

        // GigE Vision 디바이스에 연결
        strLog.Format(_T("[Camera[%d]] Connecting.. to device"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        // 카메라 모델
        CString strModelName = Manager->m_strSetModelName.at(nIndex);
        strLog.Format(_T("[Camera[%d]] Model = [%s]"), nIndex + 1, static_cast<LPCTSTR>(strModelName));
        Common::GetInstance()->AddLog(0, strLog);

        PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
        strLog.Format(_T("[Camera[%d]] IP Address = [%s]"), nIndex + 1, static_cast<LPCTSTR>(Manager->m_strSetIPAddress.at(nIndex)));
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
            strLog.Format(_T("[Camera[%d]] Connect Success"), nIndex + 1);
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

    // 재시도 횟수를 5번으로 제한
    const int maxRetries = 5;

    // 재시도 루프
    while (nRtyCnt <= maxRetries)
    {
        strLog.Format(_T("Opening stream from device"));
        Common::GetInstance()->AddLog(0, strLog);

        PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
        lStream = PvStream::CreateAndOpen(ConnectionID, &lResult);

        if (lStream != nullptr)
        {
            strLog.Format(_T("[Camera[%d]] Stream opened successfully."), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);

            Common::GetInstance()->AddLog(0, _T("------------------------------------"));
            return lStream;
        }
        else
        {
            strLog.Format(_T("[Camera[%d]] Unable to stream from device. Retrying... %d"), nIndex + 1, nRtyCnt);
            Common::GetInstance()->AddLog(0, strLog);
            nRtyCnt++;

            if (nRtyCnt > maxRetries)
            {
                strLog.Format(_T("[Camera[%d]] Unable to stream from device after %d retries. ResultCode = %p"), nIndex + 1, maxRetries, &lResult);
                Common::GetInstance()->AddLog(0, strLog);
            }
        }
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
        strLog.Format(_T("[Camera[%d]] Negotiate packet size"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        // 장치 스트리밍 대상 설정
        lDeviceGEV->SetStreamDestination(lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort());
        strLog.Format(_T("[Camera[%d]] Configure device streaming destination"), nIndex + 1);
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
            strLog.Format(_T("[Camera[%d]] Pipeline %s"), nIndex + 1, lPipeline ? _T("Create Success") : _T("Create Fail"));
        }
        else if (aDevice != nullptr)
        {
            // 스트림이 없는 경우 이미 존재하는 m_Pipeline 활용
            m_Pipeline->SetBufferCount(BUFFER_COUNT);
            m_Pipeline->SetBufferSize(lSize);
            lPipeline = m_Pipeline;
            strLog.Format(_T("[Camera[%d]] Pipeline SetBufferCount, SetBufferSize Success"), nIndex + 1);
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
        strLog.Format(_T("[Camera[%d]] Streaming Parameters Set Success "), nIndex+1);
        Common::GetInstance()->AddLog(0, strLog);
    }


    if (bFlag[0] && bFlag[1] && bFlag[2])
    {
        SetCamRunningFlag(true);
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
            strLog.Format(_T("[Camera[%d]] Set IRFormat Fail code = %p"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            
            strLog.Format(_T("[Camera[%d]] Set IRFormat = %s"), nIndex + 1, (CString)strIRType);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;
    
    case Ax5:
    
        // Set TemperatureLinearMode (1 == on, 0 == off)
        result = lDeviceParams->SetEnumValue("TemperatureLinearMode", 1);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera[%d]] Set TemperatureLinearMode Fail code = [%p]"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            strLog.Format(_T("[Camera[%d]] Set TemperatureLinearMode on Success "), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }

        // Set TemperatureLinearResolution (low == 0, high == 1)
        result = lDeviceParams->SetEnumValue("TemperatureLinearResolution", 1);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera[%d]] Set TemperatureLinearResolution Fail code = %p"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
            return -1; 
        }
        else
        {
            strLog.Format(_T("[Camera[%d]] Set TemperatureLinearResolution high Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;
    case BlackFly:
        strLog.Format(_T("[Camera[%d]] Real image camera BlackFly"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        
        break;
    default:
        // 지원하지 않는 카메라 유형 처리
        strLog.Format(_T("[Camera[%d]] Unsupported Camera Type: %d"), nIndex + 1, m_Camlist);
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
void CameraControl_rev::DataProcessing(PvBuffer* aBuffer, int nIndex)
{

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
    strLog.Format(_T("[Camera[%d]] R = %d, Spot = %.2f B = %.2f, O = %.2f, J1 = %.2f"), nIndex + 1, RVaule, dSpot, dBValue, dOValue, dJ1Value);

    if (MainDlg->m_chGenICam_checkBox.GetCheck() && m_Camlist == Ax5)
        Common::GetInstance()->AddLog(0, strLog);
        */

    // 카메라 버퍼 데이터와 상태변수 체크
    if (IsInvalidState(nIndex, aBuffer))
    {
        return;
    }

    // aBuffer에서 이미지와 데이터 포인터를 가져온다
    PvImage* lImage = GetImageFromBuffer(aBuffer);

    //픽셀 포멧 가져오기
    SetupImagePixelType(lImage);
    byte* imageDataPtr = reinterpret_cast<byte*>(lImage->GetDataPointer());

    ImgProc->RenderDataSequence(lImage, imageDataPtr, nIndex);
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
    
    while (CamClass->m_TStatus == THREAD_RUNNING) // 쓰레드가 실행 중인 동안 반복
    {
        if (CamClass->GetThreadFlag())// 쓰레드 플래그 값 가져오기
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
                        // 러닝 플래그와 쓰레드 플레그를 중복 체크하여 이미지 처리 함수를 호출한다
                        if (CamClass->GetCamRunningFlag() && CamClass->GetThreadFlag())
                        {
                            CamClass->DataProcessing(lBuffer, nIndex);

                            CamClass->SetReStartFlag(false);
                            CamClass->SetCamRunningFlag(true);
                        }                         
                    }
                    else
                    {
                        // 버퍼 가져오기 실패
                        strLog.Format(_T("CamIndex[%d] Retrieve buffer Fail code = [%d]"), nIndex + 1, lResult.GetCode());
                        Common::GetInstance()->AddLog(0, strLog);

                        CamClass->SetCamRunningFlag(true);
                        CamClass->SetReStartFlag(true);
                    }
                    Sleep(50); 
                }              
            }
            else
            {
                // 재시작 시퀀스
                Sleep(1000);
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
    CMDS_Ebus_SampleDlg* MainDlg = ImgProc->GetMainDialog();
    CString strLog = _T("");
    strLog.Format(_T("---------Camera Start----------"));
    Common::GetInstance()->AddLog(0, strLog);

    CameraSequence(nIndex);

    bFlag = true;
    MainDlg->gui_status = GUI_STATUS::GUI_STEP_RUN;

    return bFlag;
}

// =============================================================================
bool CameraControl_rev::CameraStop(int nIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = ImgProc->GetMainDialog();
    CString strLog = _T("");
    bool bFlag[3] = { false, };
    bool bRtn = false;

    if (m_Device != NULL && m_Stream != NULL && m_Pipeline != NULL)
    {
        bFlag[0] = m_Device->StreamDisable();
        bFlag[1] = m_Device->GetParameters()->ExecuteCommand("AcquisitionStop");
        SetCamRunningFlag(false);
        SetThreadFlag(false);
        SetStartFlag(true);

        //녹화 중지
        if (ImgProc->GetStartRecordingFlag())
            ImgProc->StopRecording();
    }

    if (bFlag[1] == TRUE && bFlag[0] == TRUE)
    {
        bRtn = true;
        strLog.Format(_T("[Camera[%d]] Camera Stop success"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
    else
    {
        bRtn = false;
        strLog.Format(_T("[Camera[%d]] Camera Stop fail"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    MainDlg->gui_status = GUI_STATUS::GUI_STEP_STOP;
    return bRtn;
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
        strLog.Format(_T("[Camera[%d]][RC]Disconnect."), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }
   
    PvString ConnectionID = Manager->m_strSetIPAddress.at(nIndex);
    strLog.Format(_T("[Camera[%d]][RC]IP Address = [%s]"), nIndex + 1, Manager->m_strSetIPAddress.at(nIndex));
    Common::GetInstance()->AddLog(0, strLog);

    if(m_Device != nullptr)
        m_Device = nullptr;

    m_Device = m_Device->CreateAndConnect(ConnectionID, &lResult);

    if (lResult.IsOK())
    {
        strLog.Format(_T("[Camera[%d]] Connect Success"), nIndex + 1);
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
void CameraControl_rev::SetCamRunningFlag(bool bFlag)
{
    m_bCamRunning = bFlag;
}

// =============================================================================
//카메라 동작중 상태변수
bool CameraControl_rev::GetCamRunningFlag()
{
    return m_bCamRunning;
}

// =============================================================================
void CameraControl_rev::SetThreadFlag(bool bFlag)
{
    m_bThreadFlag = bFlag;
}

// =============================================================================
//쓰레드 동작중 상태변수
bool CameraControl_rev::GetThreadFlag()
{
    return m_bThreadFlag;
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
        strLog.Format(_T("[Camera[%d]] Camera Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
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
            strLog.Format(_T("[Camera[%d]] Set PixelFormat [%s] %s"), nIndex + 1, strPixelFormat, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;

    case Ax5:

        strLog.Format(_T("[Camera[%d]] Camera Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
        Common::GetInstance()->AddLog(0, strLog);

        //ax5
        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera[%d]] Set PixelFormat Fail code = %d"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera[%d]] Set PixelFormat %s"), nIndex + 1, strPixelFormat, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }
   
        //14bit
        result = lDeviceParams->SetEnumValue("DigitalOutput", 3);

        if (!result.IsOK())
        {
            strLog.Format(_T("[Camera[%d]] Set DigitalOutput Fail code = %d"), nIndex + 1, &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera[%d]] Set DigitalOutput Success"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;

    case BlackFly:
        strLog.Format(_T("[Camera[%d]] Camera Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
        Common::GetInstance()->AddLog(0, strLog);

        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);

        if (!result.IsOK())
        {
            strLog.Format(_T("Set PixelFormat Fail code = %d"), &result);
            Common::GetInstance()->AddLog(0, strLog);
        }
        else
        {
            strLog.Format(_T("[Camera[%d]] Set PixelFormat [%s] %s"), nIndex + 1, strPixelFormat, m_Cam_Params->strPixelFormat);
            Common::GetInstance()->AddLog(0, strLog);
        }
        break;

    default:
        result = lDeviceParams->SetEnumValue("PixelFormat", m_pixeltype);
        break;
    }

    return false;
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
void CameraControl_rev::SetCamIndex(int nIndex)
{
    m_nCamIndex = nIndex;
}

// =============================================================================
int CameraControl_rev::GetCamIndex()
{
    return m_nCamIndex;
}

