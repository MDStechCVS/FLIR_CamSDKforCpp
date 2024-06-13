#include "CameraControl_rev.h"
#include "ImageProcessor.h"


const TCHAR* IRFormatArray::IRFormatStrings[] =
{
    _T("RADIOMETRIC"),
    _T("TEMPERATURELINEAR10MK"),
    _T("TEMPERATURELINEAR100MK"),
    nullptr
};

CameraControl_rev::CameraControl_rev(int nIndex)
    : Manager(nullptr),
      m_nCamIndex(nIndex),
      m_Cam_Params(new Camera_Parameter),
      m_tauPlanckConstants(new TPConstants),
      m_objectParams(new ObjParams),
      m_spectralResponseParams(new stRParams)
{
    // ī�޶� ������ ���ν��� ��ü�� �����ϰ� �ʱ�ȭ
    m_Cam_Params->index = nIndex;
    m_Cam_Params->param = this;
    // �������� �ʱ�ȭ
    Initvariable();
    SetCamIndex(m_nCamIndex);

    // ī�޶� �����带 �����ϰ� ����
    StartThreadProc(nIndex);

    StopImageProcessingThread();
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

// =============================================================================
void CameraControl_rev::CreateImageProcessor(int nIndex)
{
    ImgProc = new ImageProcessor(GetCamIndex(), this);
}

// =============================================================================
ImageProcessor* CameraControl_rev::GetImageProcessor() const 
{
    return ImgProc; // unique_ptr�� get() �޼��带 ����Ͽ� ������ ��ȯ
}

// =============================================================================
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
    m_bCamRunning = false;
    m_bReStart = false;
    
    m_Device = nullptr;
    m_Stream = nullptr;
    m_Pipeline = nullptr;

    // ī�޶� 1�� FPS �ʱ�ȭ
    m_dCam_FPS = 0;
    // ��ư�� Ŭ���Ͽ� ��ġ ���� �����Ȱ��
    bbtnDisconnectFlag = false; 

    m_bMeasCapable = false;



    m_objectParams->AtmTemp = m_objectParams->ExtOptTemp = m_objectParams->AmbTemp = 293.15f;
    m_objectParams->ExtOptTransm = 1.0f; // default

    // For TAU cameras we need GUI code for editing these values
    m_objectParams->ObjectDistance = 2.0; // Default
    m_objectParams->RelHum = 0.5; // Default
    m_objectParams->Emissivity = 1.0; // Default

    m_tauPlanckConstants->J1 = 1;
    m_tauPlanckConstants->J0 = 0;
    // Initiate spectral response
    m_spectralResponseParams->X = 1.9;
    m_spectralResponseParams->alpha1 = 0.006569;
    m_spectralResponseParams->beta1 = -0.002276;
    m_spectralResponseParams->alpha2 = 0.01262;
    m_spectralResponseParams->beta2 = -0.00667;

    ClearQueue();

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

    if (GetReStartFlag() == FALSE)
    {
        // ī�޶� ����ü ����
        delete m_Cam_Params;
        m_Cam_Params = nullptr;

        delete m_tauPlanckConstants;
        m_tauPlanckConstants = nullptr;

        delete m_objectParams;
        m_objectParams = nullptr;

        delete m_spectralResponseParams;
        m_spectralResponseParams = nullptr;

        if (ImgProc != nullptr)
        {
            delete ImgProc;
            ImgProc = nullptr;
        }

        StopImageProcessingThread();

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
    ImgProc->GetMainDialog()->gui_status = GUI_STATUS::GUI_STEP_IDLE;
    
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
            lStream->GetParameters()->ExecuteCommand(" Reset ");

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

    if(lPipeline != nullptr)
        lPipeline->Reset();

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

    FFF_HeightSummary(lDeviceParams);

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
        strLog.Format(_T("[Camera[%d]] Streaming Parameters Set Success "), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    TempRangeSearch(nIndex, lDeviceParams);

    if (bFlag[0] && bFlag[1] && bFlag[2])
    {
        if (GetCamRunningFlag() == FALSE)
        {
            SetCamRunningFlag(true);
            SetThreadFlag(true);
            SetStartFlag(true);
            bbtnDisconnectFlag = false;
            if (m_Camlist != CAM_MODEL::BlackFly)
            {
                int64_t nValue = 0;
                lDeviceParams->GetEnumValue("IRFormat", nValue);
                strLog.Format(_T("[Camera[%d]] Get IRFormat = %d"), nIndex + 1, nValue);
                Common::GetInstance()->AddLog(0, strLog);
            }

            return true;
        }
    }

    return false;

}

// =============================================================================
// IR ���� �α��� ���� �Լ�
void CameraControl_rev::LogIRFormat(int nIndex, PvGenParameterArray* lDeviceParams)
{
    if (m_Camlist != CAM_MODEL::BlackFly)
    {
        int64_t nValue = 0;
        lDeviceParams->GetEnumValue("IRFormat", nValue);
        CString strLog;
        strLog.Format(_T("[Camera[%d]] Get IRFormat = %d"), nIndex + 1, nValue);
        Common::GetInstance()->AddLog(0, strLog);
    }
}

// =============================================================================
// ī�޶� �Ű������� �����ϴ� �Լ�
int CameraControl_rev::SetStreamingCameraParameters(PvGenParameterArray * lDeviceParams, int nIndex, CameraModelList Camlist)
{
    PvResult result = -1;
    CString strLog = _T("");
    CString strIRType = _T("");
    Camera_IRFormat nIRType;
    int64_t nValue = 0;

    //*--Streaming enabled --*/
   // 0 = Radiometric
   // 1 = TemperatureLinear100mK
   // 2 =TemperatureLinear10mK

    nIRType = GetIRFormat();

    switch (Camlist)
    {
    case CAM_MODEL::FT1000:
    case CAM_MODEL::XSC:
    case CAM_MODEL::A300:
    case CAM_MODEL::A400:
    case CAM_MODEL::A500:
    case CAM_MODEL::A700:
    case CAM_MODEL::A615:
    case CAM_MODEL::A50:
        
        //nIRType = RADIOMETRIC;
        result = lDeviceParams->SetEnumValue("IRFormat", (int)nIRType);
        if (nIRType == Camera_IRFormat::TEMPERATURELINEAR10MK)
        {
            strIRType.Format(_T("TemperatureLinear10mK"));
        }
        else if (nIRType == Camera_IRFormat::TEMPERATURELINEAR100MK)
        {
            strIRType.Format(_T("TemperatureLinear100mK"));
        }
        else if (nIRType == Camera_IRFormat::RADIOMETRIC)
        {
            strIRType.Format(_T("Radiometric"));
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

        lDeviceParams->GetEnumValue("IRFormat", nValue);
        strLog.Format(_T("[Camera[%d]] Get IRFormat = %d"), nIndex + 1, nValue);
        Common::GetInstance()->AddLog(0, strLog);

        break;
    
    case CAM_MODEL::Ax5:
        
        if (GetIRFormat() != Camera_IRFormat::RADIOMETRIC)
        {
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
        }
        break;
    case CAM_MODEL::BlackFly:
        strLog.Format(_T("[Camera[%d]] Real image camera BlackFly"), nIndex + 1);
        Common::GetInstance()->AddLog(0, strLog);
        
        break;
    default:
        // �������� �ʴ� ī�޶� ���� ó��
        strLog.Format(_T("[Camera[%d]] Unsupported Camera Type: %d"), nIndex + 1, m_Camlist);
        Common::GetInstance()->AddLog(0, strLog);
        return -1; 
    }

    if (nIRType == Camera_IRFormat::RADIOMETRIC)
    {
        PvGenInteger* lR = dynamic_cast<PvGenInteger*>(lDeviceParams->Get("R"));
        PvGenFloat* lB = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("B"));
        PvGenFloat* lF = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("F"));
        PvGenFloat* lO = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("O"));
        if (lR && lB && lF && lO)
        {
            int64_t tmpR = 0;
            lR->GetValue(tmpR);
            m_tauPlanckConstants->R = (int)tmpR;
            lB->GetValue(m_tauPlanckConstants->B);
            lF->GetValue(m_tauPlanckConstants->F);
            lO->GetValue(m_tauPlanckConstants->O);
            m_bMeasCapable = true;
        }
        // Get gain (J1) and offset (J0)
        PvGenInteger* lJ0 = dynamic_cast<PvGenInteger*>(lDeviceParams->Get("J0"));
        PvGenFloat* lJ1 = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("J1"));
        if (lJ0 && lJ1)
        {
            int64_t tmp;
            PvResult lr;
            lr = lJ1->GetValue(m_tauPlanckConstants->J1);
            lJ0->GetValue(tmp);
            m_tauPlanckConstants->J0 = (ULONG)tmp;
            if (lr.IsOK())
                m_bMeasCapable = true;
        }
        // Get spectral response
        PvGenFloat* lX = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("X"));
        PvGenFloat* la1 = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("alpha1"));
        PvGenFloat* la2 = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("alpha2"));
        PvGenFloat* lb1 = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("beta1"));
        PvGenFloat* lb2 = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("beta2"));
        if (lX && la1 && la2 && lb1 && lb2) 
        {
            PvResult lr;
            lr = lX->GetValue(m_spectralResponseParams->X);
            la1->GetValue(m_spectralResponseParams->alpha1);
            la2->GetValue(m_spectralResponseParams->alpha2);
            lb1->GetValue(m_spectralResponseParams->beta1);
            lb2->GetValue(m_spectralResponseParams->beta2);
            if (lr.IsOK())
                m_bMeasCapable = true;
        }

        if (m_bMeasCapable)
            doUpdateCalcConst();
    }
    else
    {
        // Get current object parameters
        PvGenFloat* lEmissivity = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("ObjectEmissivity"));
        PvGenFloat* lAmb = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("ReflectedTemperature"));
        PvGenFloat* lAtm = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("AtmosphericTemperature"));
        PvGenFloat* lEOT = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("WindowTemperature"));
        PvGenFloat* lExtOptTransm = dynamic_cast<PvGenFloat*>(lDeviceParams->Get("WindowTransmission"));

        CString strEditText;
        ImgProc->GetMainDialog()->m_edExtOptTemp.GetWindowText(strEditText);
        m_objectParams->ExtOptTemp = _ttof(strEditText);

        if (lEmissivity && lAmb && lAtm && lEOT && lExtOptTransm)
        {
            lEmissivity->GetValue(m_objectParams->Emissivity);
            lAmb->GetValue(m_objectParams->AmbTemp);
            lAtm->GetValue(m_objectParams->AtmTemp);
            lEOT->GetValue(m_objectParams->ExtOptTemp);
            lExtOptTransm->GetValue(m_objectParams->ExtOptTransm);
        }

        // Initiate spectral response
        m_spectralResponseParams->X = 1.9;
        m_spectralResponseParams->alpha1 = 0.006569;
        m_spectralResponseParams->beta1 = -0.002276;
        m_spectralResponseParams->alpha2 = 0.01262;
        m_spectralResponseParams->beta2 = -0.00667;

        if (m_bMeasCapable)
            doUpdateCalcConst();
        
    }
    return 0;
}

// =============================================================================
void CameraControl_rev::doUpdateCalcConst(void)
{
    m_objectParams->AtmTao = doCalcAtmTao();

    m_tauPlanckConstants->K1 = doCalcK1();

    m_tauPlanckConstants->K2 = doCalcK2(tempToObjSig(m_objectParams->AmbTemp),
        tempToObjSig(m_objectParams->AtmTemp),
        tempToObjSig(m_objectParams->ExtOptTemp));
}

// =============================================================================
// A50 ī�޶��� ��� ���̸� ������Ʈ�ϴ� �Լ� FFF �����͸� ������ؼ� ������ ��� ������ �� ���;��Ѵ�.
// ��� ������ ���Ե� ������ ũ�⸦ ��ȯ
int CameraControl_rev::UpdateHeightForA50Camera(int& nHeight, int nWidth)
{
    if (nHeight <= 0 || nWidth <= 0)
    {
        return 0;
    }

    int heightAdjustment = 1392 / nWidth + (1392 % nWidth ? 1 : 0);

    if (0 < heightAdjustment)
    {
        nHeight += heightAdjustment;
    }

    return heightAdjustment;
}

double CameraControl_rev::doCalcAtmTao(void)
{
    double tao, dtao;
    double H, T, sqrtD, X, a1, b1, a2, b2;
    double sqrtH2O;
    double TT;
    double a1b1sqH2O, a2b2sqH2O, exp1, exp2;
    CTemperature C(CTemperature::Celsius);

#define H2O_K1 +1.5587e+0
#define H2O_K2 +6.9390e-2
#define H2O_K3 -2.7816e-4
#define H2O_K4 +6.8455e-7
#define TAO_TATM_MIN -30.0
#define TAO_TATM_MAX  90.0
#define TAO_SQRTH2OMAX 6.2365
#define TAO_COMP_MIN 0.400
#define TAO_COMP_MAX 1.000

    H = m_objectParams->RelHum;
    C = m_objectParams->AtmTemp;
    T = C.Value();        // We need Celsius to use constants defined above
    sqrtD = sqrt(m_objectParams->ObjectDistance);
    X = m_spectralResponseParams->X;
    a1 = m_spectralResponseParams->alpha1;
    b1 = m_spectralResponseParams->beta1;
    a2 = m_spectralResponseParams->alpha2;
    b2 = m_spectralResponseParams->beta2;

    if (T < TAO_TATM_MIN)
        T = TAO_TATM_MIN;
    else if (T > TAO_TATM_MAX)
        T = TAO_TATM_MAX;

    TT = T * T;

    sqrtH2O = sqrt(H * exp(H2O_K1 + H2O_K2 * T + H2O_K3 * TT + H2O_K4 * TT * T));

    if (sqrtH2O > TAO_SQRTH2OMAX)
        sqrtH2O = TAO_SQRTH2OMAX;

    a1b1sqH2O = (a1 + b1 * sqrtH2O);
    a2b2sqH2O = (a2 + b2 * sqrtH2O);
    exp1 = exp(-sqrtD * a1b1sqH2O);
    exp2 = exp(-sqrtD * a2b2sqH2O);

    tao = X * exp1 + (1 - X) * exp2;
    dtao = -(a1b1sqH2O * X * exp1 + a2b2sqH2O * (1 - X) * exp2);
    // The real D-derivative is also divided by 2 and sqrtD.
    // Here we only want the sign of the slope! */

    if (tao < TAO_COMP_MIN)
        tao = TAO_COMP_MIN;      // below min value, clip

    else if (tao > TAO_COMP_MAX)
    {
        // check tao at 1 000 000 m dist
        tao = X * exp(-(1.0E3) * a1b1sqH2O) + (1.0 - X) * exp(-(1.0E3) * a2b2sqH2O);

        if (tao > 1.0)    // above max, staying up, assume \/-shape
            tao = TAO_COMP_MIN;
        else
            tao = TAO_COMP_MAX; // above max, going down, assume /\-shape
    }
    else if (dtao > 0.0 && m_objectParams->ObjectDistance > 0.0)
        tao = TAO_COMP_MIN;   // beween max & min, going up, assume \/

    // else between max & min, going down => OK as it is, ;-)

    return(tao);
}

// =============================================================================
double CameraControl_rev::doCalcK1(void)
{
    double dblVal = 1.0;

    dblVal = m_objectParams->AtmTao * m_objectParams->Emissivity * m_objectParams->ExtOptTransm;

    if (dblVal > 0.0)
        dblVal = 1 / dblVal;

    return (dblVal);
}

// =============================================================================
double CameraControl_rev::doCalcK2(double dAmbObjSig,
    double dAtmObjSig,
    double dExtOptTempObjSig)
{
    double emi;
    double temp1 = 0.0;
    double temp2 = 0.0;
    double temp3 = 0.0;

    emi = m_objectParams->Emissivity;

    if (emi > 0.0)
    {
        temp1 = (1.0 - emi) / emi * dAmbObjSig;

        if (m_objectParams->AtmTao > 0.0) {
            temp2 = (1.0 - m_objectParams->AtmTao) / (emi * m_objectParams->AtmTao) * dAtmObjSig;

            if (m_objectParams->ExtOptTransm > 0.0 && m_objectParams->ExtOptTransm < 1.0) {
                temp3 = (1.0 - m_objectParams->ExtOptTransm) /
                    (emi * m_objectParams->AtmTao * m_objectParams->ExtOptTransm) * dExtOptTempObjSig;
            }
        }
    }

    return (temp1 + temp2 + temp3);
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
    PvBuffer* lBuffer = aBuffer;

    // ī�޶� ���� �����Ϳ� ���º��� üũ
    if (IsInvalidState(nIndex, aBuffer))
    {
        return;
    }

    // aBuffer���� �̹����� ������ �����͸� �����´�
    PvImage* lImage = GetImageFromBuffer(aBuffer);

    //image Alloc
    SetupImagePixelType(lImage);

    byte* imageDataPtr = reinterpret_cast<byte*>(lImage->GetDataPointer());

    // �̹��� ó���� ���� ������ ����
    StartImageProcessingThread(aBuffer, nIndex);
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

    
    while (CamClass->m_TStatus == THREAD_STATUS::THREAD_RUNNING) // �����尡 ���� ���� ���� �ݺ�
    {
        if (CamClass->GetThreadFlag())// ������ �÷��� �� ��������
        {
            if (!CamClass->GetReStartFlag()) // ����� �÷��� Ȯ��
            {
                if (CamClass->m_Stream &&CamClass->m_Stream->IsOpen()) // ��Ʈ���� �����ִ��� Ȯ��
                {
                    lBuffer = nullptr; // PvBuffer ��ü �ʱ�ȭ
                
                    // ���� ���۸� ��������
                    PvResult lResult = CamClass->m_Pipeline->RetrieveNextBuffer(&lBuffer, 5000, &lOperationResult);
                    if (lResult.IsOK())
                    {
                        if (lOperationResult.IsOK())
                        {
                            // ������ ������ �۾� ��� Ȯ�� �� ó��
                            // strLog.Format(_T("BlockID : %d "), lBuffer->GetBlockID());
                            // Common::GetInstance()->AddLog(0, strLog);
                                                     
                            PvGenParameterArray* lStreamParams = CamClass->m_Stream->GetParameters();                                         
                            PvGenFloat* lFrameRate = dynamic_cast<PvGenFloat*>(lStreamParams->Get("AcquisitionRate"));
                            lFrameRate->GetValue(lFrameRateVal);
                            CamClass->SetCameraFPS(lFrameRateVal);
                                                 
                            // ���� �÷��׿� ������ �÷��׸� �ߺ� üũ�Ͽ� �̹��� ó�� �Լ��� ȣ���Ѵ�
                            if (CamClass->GetCamRunningFlag() && CamClass->GetThreadFlag())
                            {          
                                if (CamClass->GetImageProcessor()->GetStartRecordingFlag())
                                {
                                    CamClass->mm_timer.GetUTCTime(CamClass->GetImageProcessor()->m_ts,
                                        CamClass->GetImageProcessor()->m_ms, CamClass->GetImageProcessor()->m_tzBias);

                                    CamClass->AddBufferToQueue(lBuffer);
                                }

                                CamClass->DataProcessing(lBuffer, nIndex);
                                CamClass->SetReStartFlag(false);
                                CamClass->SetCamRunningFlag(true);                                                                                 
                            }
                            CamClass->m_Pipeline->ReleaseBuffer(lBuffer);
                        }
                        else
                        {
                            // �۾� ����� OK�� �ƴ� ��� ó��
                            // cout << lDoodle[lDoodleIndex] << " " << lOperationResult.GetCodeString().GetAscii() << "\r";
                            if(lBuffer)
                                CamClass->m_Pipeline->ReleaseBuffer(lBuffer);
                        }   
                        std::this_thread::sleep_for(std::chrono::milliseconds(0));
                    }           
                    else
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(0));
                        // ���� �������� ����
                        strLog.Format(_T("CamIndex[%d] Retrieve buffer Fail code = [%d]"), nIndex + 1, lResult.GetCode());
                        Common::GetInstance()->AddLog(0, strLog);

                        CamClass->SetCamRunningFlag(true);
                        CamClass->SetReStartFlag(true);
                    }
                }              
            }
            else
            {
                // ����� ������
                Sleep(500);
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
    if (m_TStatus == THREAD_STATUS::THREAD_RUNNING)
    {
        m_TStatus = THREAD_STATUS::THREAD_STOP;
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
    Sleep(500);
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
        if (GetCamRunningFlag())
        {
            SetCamRunningFlag(false);
            SetThreadFlag(false);
            SetStartFlag(true);
        }

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
    {
        bRtn = false;
    }

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
        m_TStatus = THREAD_STATUS::THREAD_RUNNING;
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
CameraModelList CameraControl_rev::FindCameraModel(int nCamIndex, PvGenParameterArray* lDeviceParams)
{
    CString strModelID = _T("");
    strModelID.Format(_T("%s"), Manager->m_strSetModelName.at(nCamIndex));
    CString sContentsLower = strModelID.MakeLower();

    PvString lInfoStr = "N/A";
    CString devInfo = _T("");
    PvGenString* lManufacturerInfo = dynamic_cast<PvGenString*>(lDeviceParams->Get("DeviceManufacturerInfo"));
    //�Ķ���Ͱ� �������
    if (lManufacturerInfo == nullptr)
        return  CAM_MODEL::None;
    lManufacturerInfo->GetValue(lInfoStr);
    devInfo = lInfoStr.GetUnicode();
    CameraModelList CamList;

    if (sContentsLower.Find(_T("a50")) != -1)
        CamList = CAM_MODEL::A50;
    else if (sContentsLower.Find(_T("a70")) != -1)
        CamList = CAM_MODEL::A70;
    else if (sContentsLower.Find(_T("a320")) != -1)
        CamList = CAM_MODEL::A300;
    else if (sContentsLower.Find(_T("a615")) != -1)
        CamList = CAM_MODEL::A615;
    else if (sContentsLower.Find(_T("ax5")) != -1)
        CamList = CAM_MODEL::Ax5;
    else if (sContentsLower.Find(_T("ft1000")) != -1)
        CamList = CAM_MODEL::FT1000;
    else if (sContentsLower.Find(_T("pt1000")) != -1)
        CamList = CAM_MODEL::FT1000;
    else if (sContentsLower.Find(_T("xsc")) != -1)
        CamList = CAM_MODEL::XSC;
    else if (sContentsLower.Find(_T("blackfly")) != -1)
        CamList = CAM_MODEL::BlackFly;

    //FT1000 �ϰ�쿡���� �߰� ���̽� ó��
    if (m_Camlist == CAM_MODEL::FT1000)
    {
        if ((devInfo.Find(L"A615G") >= 0))
        {
            CamList = CAM_MODEL::A615;
        }
    }
    //if (devInfo.Find(L"A320G") >= 0)
    
     return CamList; // �⺻��
}

// =============================================================================
// description : A645, A615, A700 ī�޶� �� ���
// Height ���̸� 480 ���� 483���� ��ȯ ���ش�.
//������ 480 ���� �Ǿ� ������, ī�޶󿡼� �޾ƿ��� �̹������� FFF ������� ������ �����Ե� ���۰� ���ŵȴ�.
bool CameraControl_rev::FFF_HeightSummary(PvGenParameterArray* lDeviceParams)
{
    PvResult result;
    //TRACE("Info: %S\r\n", devInfo);
    //OutputDebugString(devInfo);
    PvGenInteger* lHeight = dynamic_cast<PvGenInteger*>(lDeviceParams->Get("Height"));
    PvGenInteger* lWidth = dynamic_cast<PvGenInteger*>(lDeviceParams->Get("Width"));
    int A400_Height = 240;
    int A700_Height = 480;
    int A70_Height = 480;
    int A645_Height = 480;
    int A50_Height = 348;
    int Ax5_Height = 256;

    lHeight->GetValue(m_Cam_Params->nHeight);
    lWidth->GetValue(m_Cam_Params->nWidth);

    int nH = m_Cam_Params->nHeight;
    int nW = m_Cam_Params->nWidth;
    int nFFF_HeaderValue = UpdateHeightForA50Camera(nH, nW);

    //�̹��� ��Ʈ���� �� ���Ǵ� ����Ʈ��
    m_Cam_Params->nHeight;

    int64_t nHeight = m_Cam_Params->nHeight;
    int nFinalHeight = 0;

    switch (m_Camlist)
    {
        case CAM_MODEL::A300:
            lHeight->SetValue(246);
            //m_bTLUTCapable = true;
        break;

        case CAM_MODEL::A50:
            m_Cam_Params->nHeight = A50_Height;
            nFinalHeight = nFFF_HeaderValue + A50_Height;
            result = lHeight->SetValue(nFinalHeight);
        break;

        case CAM_MODEL::A400:
            m_Cam_Params->nHeight = A400_Height;
            nFinalHeight = nFFF_HeaderValue + A400_Height;
            result = lHeight->SetValue(nFinalHeight);
        break;

        case CAM_MODEL::A70:
        case CAM_MODEL::A615:
        case CAM_MODEL::A700:
        case CAM_MODEL::FT1000:
            m_Cam_Params->nHeight = A645_Height;
            nFinalHeight = nFFF_HeaderValue + A645_Height;
            result = lHeight->SetValue(nFinalHeight);
            //m_bTLUTCapable = true;
        break;
        
    default:
        //lHeight->SetValue(nFinalHeight);
        break;
    }

//PvGenEnum* lWindowing = dynamic_cast<PvGenEnum*>(m_Device->GetParameters()->Get("IRWindowing"));
//if (lWindowing) {
//    int64_t mode = 0;
//    lWindowing->GetValue(mode);
//    m_WindowingMode = (int)mode;
//    if (m_WindowingMode == 1) 
//    {
//        lHeight->SetValue(240);
//    }
//    else if (m_WindowingMode == 2) 
//    {
//        lHeight->SetValue(120);
//    }
//}

    if (result.IsOK())
    {
        lHeight->GetValue(nHeight);
        CString strLog = _T("");
        // ī�޶� Height
        strLog.Format(_T("[Camera[%d]] FFF_HeightSummary = %d"), m_nCamIndex + 1, nHeight);
        Common::GetInstance()->AddLog(0, strLog);
        return true;
    }
       
    else
        return false;
}

// =============================================================================
//ī�޶� �Ķ���� �����ϱ�
bool CameraControl_rev::CameraParamSetting(int nIndex, PvDevice* aDevice)
{
    if (m_Cam_Params == nullptr)
        return false;

    PvGenParameterArray* lDeviceParams = aDevice->GetParameters();
    PvResult result = -1;
    CString strLog = _T("");
    bool bFindFlag = false;

    m_Camlist = FindCameraModel(nIndex, lDeviceParams);

    if(m_Camlist != CAM_MODEL::BlackFly)
        FFF_HeightSummary(lDeviceParams);

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
    case CAM_MODEL::FT1000:
    case CAM_MODEL::XSC:
    case CAM_MODEL::A300:
    case CAM_MODEL::A400:
    case CAM_MODEL::A500:
    case CAM_MODEL::A700:
    case CAM_MODEL::A615:
    case CAM_MODEL::A50:

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

    case CAM_MODEL::Ax5:

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

    case CAM_MODEL::BlackFly:
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
void CameraControl_rev::SetBuffer(PvBuffer* aBuffer)
{  
    if(aBuffer!= nullptr)
        m_Buffer = aBuffer;
}

// =============================================================================
PvBuffer* CameraControl_rev::GetBuffer()
{
    return m_Buffer;
}

// =============================================================================
// �̹����� �ȼ� ������ �����ϴ� �Լ�
void CameraControl_rev::SetupImagePixelType(PvImage* lImage)
{
    DWORD pixeltypeValue = ConvertHexValue(m_Cam_Params->strPixelFormat);
    m_pixeltype = (PvPixelType)pixeltypeValue;

    lImage->Alloc(lImage->GetWidth(), m_Cam_Params->nHeight, PvPixelRGB8);
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
    return nIndex < 0 || m_TStatus != THREAD_STATUS::THREAD_RUNNING || !IsValidBuffer(buffer);
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

// =============================================================================
void CameraControl_rev::SetIRFormat(CameraIRFormat IRFormat)
{
    m_IRFormat = IRFormat;
}

// =============================================================================
CameraIRFormat CameraControl_rev::GetIRFormat()
{
    return m_IRFormat;
}

// =============================================================================
void CameraControl_rev::SetIRRange(Camera_Parameter m_Cam_Params, int nQureyIndex)
{
    m_Cam_Params.nSelectCase = nQureyIndex;
}

// =============================================================================
int CameraControl_rev::GetIRRange()
{
    return m_Cam_Params->nSelectCase;
}

// =============================================================================
CTemperature CameraControl_rev::imgToTemp(long lPixval)
{
    double tmp;
    CTemperature K;

    tmp = imgToPow(lPixval);
    //tmp = clipPow(tmp,pValState);
    tmp = powToObjSig(tmp);
    K = objSigToTemp(tmp);

    return (K);
}

// =============================================================================
double CameraControl_rev::imgToPow(long lPixval)
{
    double pow;

    pow = (lPixval - m_tauPlanckConstants->J0) / m_tauPlanckConstants->J1;

    return (pow);
}

// =============================================================================
USHORT CameraControl_rev::tempToImg(double dKelvin)
{
    USHORT pixVal;
    double tmp;

    tmp = tempToObjSig(dKelvin);
    tmp = objSigToPow(tmp);
    pixVal = powToImg(tmp);

    return pixVal;
}

// =============================================================================
double CameraControl_rev::objSigToPow(double dObjSig)
{
#define POW_OVERFLOW   100000.0  
    double p;

    if (m_tauPlanckConstants->K1 > 0.0)
        p = (dObjSig + m_tauPlanckConstants->K2) / m_tauPlanckConstants->K1;
    else
        p = POW_OVERFLOW;

    return (p);
}

// =============================================================================
USHORT CameraControl_rev::powToImg(double dPow)
{
    // Convert a power equivalent pixel value to mapped FPA pixel value  
    long rpix;

    rpix = (long)(m_tauPlanckConstants->J1 * dPow + m_tauPlanckConstants->J0);

    return (USHORT)rpix;
}

// =============================================================================
double CameraControl_rev::tempToObjSig(double dblKelvin)
{
    double objSign = 0.0;
    double dbl_reg = dblKelvin;

    // objSign = R / (exp(B/T) - F)

    if (dbl_reg > 0.0) {

        dbl_reg = m_tauPlanckConstants->B / dbl_reg;

        if (dbl_reg < EXP_SAFEGUARD) {
            dbl_reg = exp(dbl_reg);

            if (m_tauPlanckConstants->F <= 1.0) {
                if (dbl_reg < ASY_SAFEGUARD)
                    dbl_reg = ASY_SAFEGUARD; // Don't get above a R/(1-F)
                                             // (horizontal) asymptote
            }
            else
            {
                // F > 1.0
                if (dbl_reg < m_tauPlanckConstants->F * ASY_SAFEGUARD)
                    dbl_reg = m_tauPlanckConstants->F * ASY_SAFEGUARD;
                // Don't get too close to a B/ln(F) (vertical) asymptote
            }

            objSign = m_tauPlanckConstants->R / (dbl_reg - m_tauPlanckConstants->F);
        }
    }

    return(objSign);
}

// =============================================================================
double CameraControl_rev::powToObjSig(double dPow)
{
    return (m_tauPlanckConstants->K1 * dPow - m_tauPlanckConstants->K2);
}

// =============================================================================
CTemperature CameraControl_rev::objSigToTemp(double dObjSig)
{
    double dbl_reg, tmp;
    CTemperature Tkelvin = 0.0;

    // Tkelvin = B /log(R / objSign + F)

    if (dObjSig > 0.0)
    {
        dbl_reg = m_tauPlanckConstants->R / dObjSig + m_tauPlanckConstants->F;

        if (m_tauPlanckConstants->F <= 1.0) {
            if (dbl_reg < ASY_SAFEGUARD)
                dbl_reg = ASY_SAFEGUARD; // Don't get above a R/(1-F)
                                         // (horizontal) asymptote
        }
        else { // if (m_F > 1.0)

            tmp = m_tauPlanckConstants->F * ASY_SAFEGUARD;
            if (dbl_reg < tmp)
                dbl_reg = tmp;
            // Don't get too close to a B/ln(F) (vertical) asymptote
        }
        Tkelvin = m_tauPlanckConstants->B / quicklog(dbl_reg);
    }

    return (Tkelvin);
}

// =============================================================================
void CameraControl_rev::OnParameterUpdate(PvGenParameter* aParameter)
{
    bool bBufferResize = false;
    PvString lName;
    static int trig1 = 0;
    static int trig2 = 0;

    if (!aParameter->GetName(lName).IsOK())
    {
        ASSERT(0); // Totally unexpected	
        return;
    }

    if (lName == "ObjectEmissivity")
    {
        PvGenFloat* lEmissivity = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("ObjectEmissivity"));
        lEmissivity->GetValue(m_objectParams->Emissivity);
        doUpdateCalcConst();
    }
    else if (lName == "ExtOpticsTransmission") 
    {
        PvGenFloat* lExtOptTransm = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("ExtOpticsTransmission"));
        lExtOptTransm->GetValue(m_objectParams->ExtOptTransm);
        doUpdateCalcConst();
    }
    else if (lName == "ExtOpticsTemperature") 
    {
        PvGenFloat* lEOT = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("ExtOpticsTemperature"));
        lEOT->GetValue(m_objectParams->ExtOptTemp);
        doUpdateCalcConst();
    }
    else if (lName == "RelativeHumidity") 
    {
        PvGenFloat* lRelHum = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("RelativeHumidity"));
        lRelHum->GetValue(m_objectParams->RelHum);
        doUpdateCalcConst();
    }
    else if (lName == "ObjectDistance") 
    {
        PvGenFloat* lDist = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("ObjectDistance"));
        lDist->GetValue(m_objectParams->ObjectDistance);
        doUpdateCalcConst();
    }
    else if (lName == "ReflectedTemperature") 
    {
        PvGenFloat* lAmb = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("ReflectedTemperature"));
        lAmb->GetValue(m_objectParams->AmbTemp);
        doUpdateCalcConst();
    }
    else if (lName == "AtmosphericTemperature") 
    {
        PvGenFloat* lAtm = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("AtmosphericTemperature"));
        lAtm->GetValue(m_objectParams->AtmTemp);
        doUpdateCalcConst();
    }
    //else if (lName == "WindowTransmission") {

    //    PvGenFloat* lExtOptTransm = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("WindowTransmission"));
    //    lExtOptTransm->GetValue(m_ExtOptTransm);
    //    doUpdateCalcConst();
    //}
    //else if (lName == "WindowTemperature") 
    //{
    //    PvGenFloat* lEOT = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("WindowTemperature"));
    //    lEOT->GetValue(m_ExtOptTemp);
    //    doUpdateCalcConst();
    //}

    if (bBufferResize)
    {
        bool bPipelineRestart = false;
        if (m_Pipeline->IsStarted())
        {
            m_Pipeline->Stop();
            bPipelineRestart = true;
        }
        if (bPipelineRestart)
            m_Pipeline->Start();
    }
}

// =============================================================================
void CameraControl_rev::UpdateCalcParams()
{
    if (!m_bMeasCapable || m_Device == nullptr)
        return;

    if (m_Camlist == CAM_MODEL::Ax5)
    {
        // Update RBF0
        PvGenInteger* lR = dynamic_cast<PvGenInteger*>(m_Device->GetParameters()->Get("R"));
        PvGenFloat* lB = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("B"));
        PvGenFloat* lF = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("F"));
        PvGenFloat* lO = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("O"));

        if (lR && lB && lF && lO)
        {
            PvResult lResult;
            int64_t tmpR = 0;
            lResult = lR->GetValue(tmpR);
            m_tauPlanckConstants->R = (int)tmpR;
            lB->GetValue(m_tauPlanckConstants->B);
            lF->GetValue(m_tauPlanckConstants->F);
            lO->GetValue(m_tauPlanckConstants->O);
            //TRACE("R=%d\r\n", m_R);
            //TRACE("B=%.1f\r\n", m_B);
            //TRACE("F=%.1f\r\n", m_F);
            //TRACE("O=%.1f\r\n", m_O);
        }

        doUpdateCalcConst();
    }
    else
    {
        // Update RBF
        PvGenFloat* lR = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("R"));
        PvGenFloat* lB = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("B"));
        PvGenFloat* lF = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("F"));

        if (lR && lB && lF)
        {
            PvResult lResult;
            double tmpR = 0;
            lR->GetValue(tmpR);
            m_tauPlanckConstants->R = (int)tmpR;
            lB->GetValue(m_tauPlanckConstants->B);
            lF->GetValue(m_tauPlanckConstants->F);
            //TRACE("R=%d\r\n", m_R);
            //TRACE("B=%.1f\r\n", m_B);
            //TRACE("F=%.1f\r\n", m_F);
        }

        // Update Gain and Offset (J1 and J0)
        PvGenInteger* lJ0 = dynamic_cast<PvGenInteger*>(m_Device->GetParameters()->Get("J0"));
        PvGenFloat* lJ1 = dynamic_cast<PvGenFloat*>(m_Device->GetParameters()->Get("J1"));
        if (lJ0 && lJ1)
        {
            int64_t tmp;
            lJ0->GetValue(tmp);
            m_tauPlanckConstants->J0 = (ULONG)tmp;
            lJ1->GetValue(m_tauPlanckConstants->J1);
      /*      TRACE("J0=%d\r\n", m_J0);
            TRACE("J1=%.1f\r\n", m_J1);*/
        }

        doUpdateCalcConst();
    }
}

// =============================================================================
void CameraControl_rev::UpdateDeviceOP()
{
    if (m_Device == nullptr)
        return;

    PvGenParameterArray* lGenDevice;
    lGenDevice = m_Device->GetParameters();

    if (m_Camlist!= CAM_MODEL::Ax5)
    {
        // Update device object parameters
        PvGenFloat* lEmissivity = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ObjectEmissivity"));
        PvGenFloat* lExtOptTransm = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ExtOpticsTransmission"));
        PvGenFloat* lRelHum = dynamic_cast<PvGenFloat*>(lGenDevice->Get("RelativeHumidity"));
        PvGenFloat* lDist = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ObjectDistance"));
        PvGenFloat* lAmb = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ReflectedTemperature"));
        PvGenFloat* lAtm = dynamic_cast<PvGenFloat*>(lGenDevice->Get("AtmosphericTemperature"));
        PvGenFloat* lEOT = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ExtOpticsTemperature"));

        if (lEmissivity && lExtOptTransm && lRelHum && lDist && lAmb && lAtm && lEOT)
        {
            lEmissivity->SetValue(m_objectParams->Emissivity);
            lExtOptTransm->SetValue(m_objectParams->ExtOptTransm);
            lRelHum->SetValue(m_objectParams->RelHum);
            lDist->SetValue(m_objectParams->ObjectDistance);
            lAmb->SetValue(m_objectParams->AmbTemp);
            lAtm->SetValue(m_objectParams->AtmTemp);
            lEOT->SetValue(m_objectParams->ExtOptTemp);
        }

        doUpdateCalcConst();
    }
    else
    {
        // Update device object parameters
        PvGenFloat* lEmissivity = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ObjectEmissivity"));
        PvGenFloat* lAmb = dynamic_cast<PvGenFloat*>(lGenDevice->Get("ReflectedTemperature"));
        PvGenFloat* lAtm = dynamic_cast<PvGenFloat*>(lGenDevice->Get("AtmosphericTemperature"));
        PvGenFloat* lEOT = dynamic_cast<PvGenFloat*>(lGenDevice->Get("WindowTemperature"));
        PvGenFloat* lExtOptTransm = dynamic_cast<PvGenFloat*>(lGenDevice->Get("WindowTransmission"));

        CString strEditText;
        ImgProc->GetMainDialog()->m_edExtOptTemp.GetWindowText(strEditText);
        m_objectParams->ExtOptTemp = _ttof(strEditText);

        if (lEmissivity && lAmb && lAtm && lEOT && lExtOptTransm)
        {
            lEmissivity->SetValue(m_objectParams->Emissivity);
            lAmb->SetValue(m_objectParams->AmbTemp);
            lAtm->SetValue(m_objectParams->AtmTemp);
            lEOT->SetValue(m_objectParams->ExtOptTemp);
            lExtOptTransm->SetValue(m_objectParams->ExtOptTransm);
        }

        if (m_bMeasCapable)
        {
            PvGenFloat* lAtmTransm = dynamic_cast<PvGenFloat*>(lGenDevice->Get("AtmosphericTransmission"));
            doUpdateCalcConst();
            if (lAtmTransm)
            {
                lAtmTransm->SetValue(m_objectParams->AtmTao);
            }
        }
    }
}

// =============================================================================
bool CameraControl_rev::TempRangeSearch(int nIndex, PvGenParameterArray* lDeviceParams)
{
    // ��ȭ�� ī�޶�� �µ����� �������� �ʿ����
    if (m_Camlist == CAM_MODEL::BlackFly)
        return false;

    lDeviceParams = m_Device->GetParameters();
    int nValue[QUERYCOUNT] = {0,};
    bool bQureyFlag[QUERYCOUNT] = { false, };
    int nCurrentCaseCnt = 0;
    int64_t nCurrentCaseValue;
    int64_t nPreviousCaseValue = -1; // ���� ���� ���̽� ���� �����ϱ� ���� ����
    CString strLog;
    bool bRtn = false;
    for (int i = 0; i < QUERYCOUNT; i++)
    {
        lDeviceParams->SetIntegerValue("QueryCase", i);
        lDeviceParams->GetBooleanValue("QueryCaseEnabled", bQureyFlag[i]);

        if (bQureyFlag[i] == true)
        {
            int64_t nCurrentCaseValue;
            lDeviceParams->GetIntegerValue("QueryCase", nCurrentCaseValue);

            // ���� ���� ���̽� ���� ���� ���� �ٸ� ��쿡�� nCurrentCaseCnt�� ������Ŵ
            if (nCurrentCaseValue != nPreviousCaseValue)
            {
                if (m_Cam_Params != nullptr)
                {
                    m_Cam_Params->nCurrentCase[nCurrentCaseCnt] = nCurrentCaseValue;
                    lDeviceParams->GetFloatValue("QueryCaseLowLimit", m_Cam_Params->dQueryCaseLowLimit[nCurrentCaseCnt]);
                    lDeviceParams->GetFloatValue("QueryCaseHighLimit", m_Cam_Params->dQueryCaseHighLimit[nCurrentCaseCnt]);


                    strLog.Format(_T("[Camera[%d]] QueryCase Value =  [%d] QueryCase LowLimit = %.2f QueryCase HighLimit = %.2f"), nIndex + 1, nCurrentCaseValue,
                        m_Cam_Params->dQueryCaseLowLimit[nCurrentCaseCnt], m_Cam_Params->dQueryCaseHighLimit[nCurrentCaseCnt]);
                    Common::GetInstance()->AddLog(0, strLog);

                    nCurrentCaseCnt++;

                    // ���� ���� ���� ���� ���̽� ������ ����
                    nPreviousCaseValue = nCurrentCaseValue;
                }
            }
        }
    }

    // ī�޶󿡼� �µ����� ����Ʈ�� 3���� ���⶧���� �ش� ���ǰ˻縦 ���ؼ� �����Ѵ�
    if (nCurrentCaseCnt >= 3)
    {
        bRtn = true;
        m_Cam_Params->nQueryCnt = nCurrentCaseCnt;
    }
    else
    {
        bRtn = false;
    }
        
    return bRtn;
}

// =============================================================================
bool CameraControl_rev::SetTempRange(int nQureyIndex)
{
    PvGenParameterArray* lDeviceParams;
    PvResult result = -1;
    CString strLog;
    lDeviceParams = m_Device->GetParameters();
    int64_t  nCurrentCaseValue = m_Cam_Params->nCurrentCase[nQureyIndex];
    result = lDeviceParams->SetIntegerValue("CurrentCase", nCurrentCaseValue);

    lDeviceParams->GetIntegerValue("CurrentCase", nCurrentCaseValue);

    strLog.Format(_T("[Camera[%d]] Change CurrentCase  =  [%d]"), GetCamIndex() + 1, nCurrentCaseValue);
    Common::GetInstance()->AddLog(0, strLog);

    return result;
}

//=============================================================================
void CameraControl_rev::AddBufferToQueue(PvBuffer* buffer)
{ 
    {
        // ť �۾� �� ������ �������� �����ϱ� ���� ���ؽ��� ��޴ϴ�.
        std::lock_guard<std::mutex> lock(bufferMutex);

        // ������ ���纻�� �����մϴ�.
        PvBuffer* copiedBuffer = CopyBuffer(buffer);
        if (copiedBuffer == nullptr)
        {
            // ���� ���� ���� �� ���� �޽����� �α׷� ����ϴ�.
            CString logMessage;
            logMessage.Format(_T("Failed to copy buffer. Address: %p"), buffer);
            Common::GetInstance()->AddLog(0, logMessage);
            return;
        }

        // ����� ���ۿ��� �̹����� ������ ���̸� �α׷� ����ϴ�.
        PvImage* lImage = copiedBuffer->GetImage();
        int height = lImage->GetHeight();
        //CString logMessage;
        //logMessage.Format(_T("Buffer added to queue. Address: %p, Buffer Height: %d"), copiedBuffer, height);
        //Common::GetInstance()->AddLog(0, logMessage);

        // ����� ���۸� ť�� �߰��մϴ�.
        bufferQueue.push(copiedBuffer);
    }
    // ���ο� ���۰� ť�� �߰��Ǿ����� ��� ���� �����忡 �˸��ϴ�.
    bufferNotEmpty.notify_one();
}

/**
 * @brief �־��� PvBuffer ��ü�� ���纻�� �����մϴ�.
 *
 * �� �Լ��� ������ PvBuffer ��ü�� ���� ���纻�� �����մϴ�. ���� ������ �����͸� �����Ͽ�
 * ���ο� PvBuffer ��ü�� ÷���մϴ�. ���簡 ���������� �Ϸ���� ������, �Ҵ�� �޸𸮸� �����ϰ�
 * nullptr�� ��ȯ�մϴ�.
 *
 * @param originalBuffer ������ ���� PvBuffer ��ü�Դϴ�.
 * @return PvBuffer* ���� ������ �����͸� �����ϴ� ���ο� PvBuffer ��ü�Դϴ�.
 */
 //=============================================================================
PvBuffer* CameraControl_rev::CopyBuffer(PvBuffer* originalBuffer)
{
    // ���� �̹������� �����͸� �����ɴϴ�.
    PvImage* originalImage = originalBuffer->GetImage();
    uint32_t width = originalImage->GetWidth();
    uint32_t height = originalImage->GetHeight();
    PvPixelType pixelType = originalImage->GetPixelType();
    void* originalData = originalImage->GetDataPointer();

    // ���� �����Ͱ� ��ȿ���� Ȯ���մϴ�.
    if (width == 0 || height == 0 || originalData == nullptr)
    {
        CString logMessage;
        logMessage.Format(_T("Original buffer data is invalid. Width: %u, Height: %u, Data Pointer: %p"), width, height, originalData);
        Common::GetInstance()->AddLog(0, logMessage);
        return nullptr;
    }

    // ���ο� ���ۿ� ����� �����͸� ���� �޸𸮸� �Ҵ��մϴ�.
    PvBuffer* copiedBuffer = new PvBuffer();
    void* copiedData = new uint8_t[height * width * (PvImage::GetPixelSize(pixelType) / 8)];
    memcpy(copiedData, originalData, height * width * (PvImage::GetPixelSize(pixelType) / 8));

    // ����� �����͸� ���ο� ���ۿ� ÷���մϴ�.
    PvResult result = copiedBuffer->GetImage()->Attach(copiedData, width, height, pixelType);
    if (!result.IsOK())
    {
        // ÷�ο� ������ ��� �Ҵ�� �޸𸮸� �����մϴ�.
        delete[] static_cast<uint8_t*>(copiedData);
        delete copiedBuffer;

        CString logMessage;
        logMessage.Format(_T("Buffer attachment failed. Width: %u, Height: %u, Data Pointer: %p"), width, height, copiedData);
        Common::GetInstance()->AddLog(0, logMessage);

        return nullptr;
    }

    // ���簡 ���������� �Ϸ�Ǿ����� �α׷� ����ϴ�.
    //CString logMessage;
    //logMessage.Format(_T("Buffer copy successful. Original Address: %p, Copy Address: %p, Width: %u, Height: %u"), originalBuffer, copiedBuffer, width, height);
    //Common::GetInstance()->AddLog(0, logMessage);

    return copiedBuffer;
}

//=============================================================================
/* 
 [condition_variable]
 ��Ƽ������ ���α׷��ֿ��� ������ ���� ȿ������ ��Ű� ����ȭ�� ���� �����̴�.
 �̸� ���� Ư�� ������ ������ ������ �����带 ����Ű��, ������ �����Ǹ� ��� ���� �����带 ���۽�Ų��. 

*@brief ť���� PvBuffer ��ü�� �����ɴϴ�.
*
* �� �Լ��� Q�� ���۰� ���� ������ ����� ��, Q���� ���۸� ������ ����.
* ť �۾� �� ������ �������� �����ϱ� ���� ���ؽ� ���
*
* @return PvBuffer * Q���� ������ PvBuffer 
*/
//=============================================================================
PvBuffer* CameraControl_rev::GetBufferFromQueue()
{
    // ť �۾� �� ������ �������� �����ϱ� ���� ���ؽ��� ��޴ϴ�.
    std::unique_lock<std::mutex> lock(bufferMutex);

    // ť�� ��� ���� ���� ������ ����մϴ�. ��� �߿��� ����� �����˴ϴ�.
    bufferNotEmpty.wait(lock, [this] { return !bufferQueue.empty(); });

    // ť�� �� �տ��� ���۸� ������ ť���� �����մϴ�.
    PvBuffer* buffer = bufferQueue.front();
    bufferQueue.pop();

    // ���ۿ��� �̹����� ������ ���̸� �α׷� ����ϴ�.
    PvImage* lImage = buffer->GetImage();
    int height = lImage->GetHeight();
    //CString logMessage;
    //logMessage.Format(_T("Height: %d , Buffer retrieved from queue. Address: %p,"), height, buffer);
    //Common::GetInstance()->AddLog(0, logMessage);

    return buffer;
}

//=============================================================================
void CameraControl_rev::ClearQueue()
{
    std::lock_guard<std::mutex> lock(bufferMutex);
    while (!bufferQueue.empty()) 
    {
        PvBuffer* buffer = bufferQueue.front();
        bufferQueue.pop();
        delete buffer; // ���� �޸� ����
    }
}

//=============================================================================
// Q�� ����ִ� ���� ���� 
int CameraControl_rev::GetQueueBufferCount()
{
    std::lock_guard<std::mutex> lock(bufferMutex);

    return bufferQueue.size();
}

//=============================================================================
void CameraControl_rev::SomeFunctionThatModifiesBuffer(PvBuffer* buffer)
{
    PvImage* lImage = buffer->GetImage();
    int heightBefore = lImage->GetHeight();

    // ���� ����
    // ...

    int heightAfter = lImage->GetHeight();
    if (heightBefore != heightAfter)
    {
        CString logMessage;
        logMessage.Format(_T("Buffer Height changed. Address: %p, Before: %d, After: %d"), buffer, heightBefore, heightAfter);
        Common::GetInstance()->AddLog(0, logMessage);
    }
}

void CameraControl_rev::StartImageProcessingThread(PvBuffer* aBuffer, int nIndex)
{
    if (ImgProcThreadRunning && imageProcessingThread.joinable())
    {
        imageProcessingThread.join();  // �̹� ���� ���� �����尡 �ִٸ� ���� ��ٸ�
    }
    ImgProcThreadRunning = true;
    ImageProcessor* imageProcessor = GetImageProcessor();
    imageProcessingThread = std::thread(&ImageProcessor::RenderDataSequence, imageProcessor, GetImageFromBuffer(aBuffer), aBuffer, GetImageFromBuffer(aBuffer)->GetDataPointer(), nIndex);
}

void CameraControl_rev::StopImageProcessingThread()
{
    ImgProcThreadRunning = false;
    if (imageProcessingThread.joinable()) {
        imageProcessingThread.join();  // �����尡 ����Ǳ� ��ٸ�
    }
}