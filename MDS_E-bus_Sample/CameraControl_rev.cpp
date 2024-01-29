#include "CameraControl_rev.h"
#include "ImageProcessor.h"


CameraControl_rev::CameraControl_rev(int nIndex)
    : Manager(nullptr),
      m_nCamIndex(nIndex),
      m_Cam_Params(new Camera_Parameter)
{
    // ī�޶� ������ ���ν��� ��ü�� �����ϰ� �ʱ�ȭ
    m_Cam_Params->index = nIndex;
    m_Cam_Params->param = this;
    // �������� �ʱ�ȭ
    Initvariable();
    SetCamIndex(m_nCamIndex);

    // ī�޶� �����带 �����ϰ� ����
    StartThreadProc(nIndex);
}

// =============================================================================
CameraControl_rev::~CameraControl_rev()
{
    // ī�޶� ���� �� ����
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
    return ImgProc; // unique_ptr�� get() �޼��带 ����Ͽ� ������ ��ȯ
}

void CameraControl_rev::StartThreadProc(int nIndex)
{
    // �ʱ�ȭ Ȯ�� �� ������ ����
    pThread[nIndex] = AfxBeginThread(ThreadCam, m_Cam_Params, THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_NEW_PROCESS_GROUP);
    pThread[nIndex]->m_bAutoDelete = FALSE;
    pThread[nIndex]->ResumeThread();
    m_TStatus = ThreadStatus::THREAD_RUNNING;
}

// =============================================================================
void CameraControl_rev::Initvariable()
{
    CString strLog = _T("");

    // ������ ���� ���� �ʱ�ȭ
    m_bThreadFlag = false;
    m_bStart = false;
    m_bCamRunning = true;
    m_bReStart = false;
    
    m_Device = nullptr;
    m_Stream = nullptr;
    m_Pipeline = nullptr;

    // ī�޶� 1�� FPS �ʱ�ȭ
    m_dCam_FPS = 0;
    // ��ư�� Ŭ���Ͽ� ��ġ ���� �����Ȱ��
    bbtnDisconnectFlag = false; 

    strLog.Format(_T("---------Camera[%d] CameraParams Variable Initialize "), GetCamIndex() + 1);
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

    // ī�޶� ����ü ����
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

    // ī�޶� ����
    CameraStop(GetCamIndex());

    // ��ġ�� ����Ǿ� �ְ� ���� ���� �ƴ� ��
    if (m_Device != nullptr && GetCamRunningFlag() == FALSE)
    {
        // ��ġ�� �̺�Ʈ�� ��� �����ϰ� ���� ����
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

        // GigE Vision ����̽��� ����
        strLog.Format(_T("[Camera[%d]] Connecting.. to device"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        // ī�޶� ��
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
            // ��ġ�� �̹� ����Ǿ� �ִ� ���
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

    // ��õ� Ƚ���� 5������ ����
    const int maxRetries = 5;

    // ��õ� ����
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
    // GigE Vision ��ġ�� ���, GigE Vision Ư�� ��Ʈ���� �Ķ���͸� ����
    CString strLog = _T("");
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV*>(aDevice);
    if (lDeviceGEV != nullptr)
    {
        PvStreamGEV* lStreamGEV = static_cast<PvStreamGEV*>(aStream);

        // ��Ŷ ũ�� ����
        lDeviceGEV->NegotiatePacketSize();
        strLog.Format(_T("[Camera[%d]] Negotiate packet size"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);

        // ��ġ ��Ʈ���� ��� ����
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
    PvPipeline* lPipeline = nullptr; // PvPipeline ������ �ʱ�ȭ
    if (aDevice != nullptr)
    {
        uint32_t lSize = aDevice->GetPayloadSize(); // ��ġ�� ���̷ε� ũ�� ȹ��

        if (aStream != nullptr)
        {
            // ��Ʈ���� �����ϴ� ��� ���ο� PvPipeline ����
            lPipeline = new PvPipeline(aStream);
            strLog.Format(_T("[Camera[%d]] Pipeline %s"), nIndex + 1, lPipeline ? _T("Create Success") : _T("Create Fail"));
        }
        else if (aDevice != nullptr)
        {
            // ��Ʈ���� ���� ��� �̹� �����ϴ� m_Pipeline Ȱ��
            m_Pipeline->SetBufferCount(BUFFER_COUNT);
            m_Pipeline->SetBufferSize(lSize);
            lPipeline = m_Pipeline;
            strLog.Format(_T("[Camera[%d]] Pipeline SetBufferCount, SetBufferSize Success"), nIndex + 1);
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
        // �������� �ʴ� ī�޶� ���� ó��
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

    // CString���� '0x' �Ǵ� '0X' ���λ縦 ����
    if (strWithoutPrefix.Left(2).CompareNoCase(_T("0x")) == 0)
        strWithoutPrefix = strWithoutPrefix.Mid(2);

    // 16���� ���ڿ��� ���ڷ� ��ȯ
    _stscanf_s(strWithoutPrefix, _T("%X"), &hexValue);

    return hexValue;
}


// =============================================================================
/// ī�޶� ���� �����͸� �����ͼ� �̹��� �İ��� 
void CameraControl_rev::DataProcessing(PvBuffer* aBuffer, int nIndex)
{

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
    strLog.Format(_T("[Camera[%d]] R = %d, Spot = %.2f B = %.2f, O = %.2f, J1 = %.2f"), nIndex + 1, RVaule, dSpot, dBValue, dOValue, dJ1Value);

    if (MainDlg->m_chGenICam_checkBox.GetCheck() && m_Camlist == Ax5)
        Common::GetInstance()->AddLog(0, strLog);
        */

    // ī�޶� ���� �����Ϳ� ���º��� üũ
    if (IsInvalidState(nIndex, aBuffer))
    {
        return;
    }

    // aBuffer���� �̹����� ������ �����͸� �����´�
    PvImage* lImage = GetImageFromBuffer(aBuffer);

    //�ȼ� ���� ��������
    SetupImagePixelType(lImage);
    byte* imageDataPtr = reinterpret_cast<byte*>(lImage->GetDataPointer());

    ImgProc->RenderDataSequence(lImage, imageDataPtr, nIndex);
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
    
    while (CamClass->m_TStatus == THREAD_RUNNING) // �����尡 ���� ���� ���� �ݺ�
    {
        if (CamClass->GetThreadFlag())// ������ �÷��� �� ��������
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
                        // ���� �÷��׿� ������ �÷��׸� �ߺ� üũ�Ͽ� �̹��� ó�� �Լ��� ȣ���Ѵ�
                        if (CamClass->GetCamRunningFlag() && CamClass->GetThreadFlag())
                        {
                            CamClass->DataProcessing(lBuffer, nIndex);

                            CamClass->SetReStartFlag(false);
                            CamClass->SetCamRunningFlag(true);
                        }                         
                    }
                    else
                    {
                        // ���� �������� ����
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
                // ����� ������
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

        //��ȭ ����
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
//�� ���� ������ ���º���
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
//ī�޶� ������ ���º���
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
//������ ������ ���º���
bool CameraControl_rev::GetThreadFlag()
{
    return m_bThreadFlag;
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
        strLog.Format(_T("[Camera[%d]] Camera Model = %s"), nIndex + 1, Manager->m_strSetModelName.at(nIndex));
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
void CameraControl_rev::SetCamIndex(int nIndex)
{
    m_nCamIndex = nIndex;
}

// =============================================================================
int CameraControl_rev::GetCamIndex()
{
    return m_nCamIndex;
}

