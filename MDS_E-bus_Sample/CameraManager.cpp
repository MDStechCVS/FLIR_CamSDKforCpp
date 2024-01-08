#include "CameraManager.h"
#include "MDS_E-bus_SampleDlg.h"
#include "CameraControl_rev.h"

CameraManager::CameraManager()
{
    m_nDeviceCnt = 0;

    for (int i = 0; i < CAMERA_COUNT; i++)
    {
        m_Cam[i] = NULL;
    }

    m_nDeviceCnt = 0;

    ClearAddressdata();

}

// =============================================================================
CameraManager::~CameraManager()
{
    ClearAddressdata();
}
void CameraManager::ClearAddressdata()
{
    lDIVector.clear();
    m_strLoadIPAddress.clear();
    m_strSetIPAddress.clear();
}

// =============================================================================
// ����Ǿ��ִ� ī�޶� ���� ��ȯ
int CameraManager::GetDeviceCount()
{
    return m_nDeviceCnt;
}

// =============================================================================
void CameraManager::SetDeviceCount(int nCnt)
{
    m_nDeviceCnt = nCnt;
}

// =============================================================================
// ���� ���Ͽ� ����Ǿ� �ִ� IP Address�� ���������� ����Ǿ� �ִ� IP Address�� ���Ͽ� 
// ����� IP�� ����Ǿ� �ִ� IP�� �����ϴٸ�  IP Address�� ����ü�� �����Ѵ�.
void CameraManager::CameraDeviceFind(CMDS_Ebus_SampleDlg* MainDlg)
{
    if (MainDlg->gui_status == GUI_STATUS::GUI_STEP_RUN)
        return;

    PvSystem lSystem;
    lSystem.Find();

    uint32_t nInterfaceCount = lSystem.GetInterfaceCount();

    // ���� ��� ��ġ ������ ����
    std::map<CString, CString> deviceMap;
    for (uint32_t i = 0; i < nInterfaceCount; i++)
    {
        const PvInterface* lInterface = dynamic_cast<const PvInterface*>(lSystem.GetInterface(i));
        if (lInterface != NULL)
        {
            uint32_t nDeviceCnt = lInterface->GetDeviceCount();
            for (uint32_t j = 0; j < nDeviceCnt; j++)
            {
                const PvDeviceInfo* lDI = dynamic_cast<const PvDeviceInfo*>(lInterface->GetDeviceInfo(j));
                if (lDI != NULL)
                {
                    CString strInterfaceID;
                    strInterfaceID.Format(_T("%s"), static_cast<LPCTSTR>(lDI->GetConnectionID()));

                    CString modelName = lDI->GetModelName();
                    deviceMap[strInterfaceID] = modelName;
                }
            }
        }
    }

    // m_strLoadIPAddress ������� ��ġ�ϴ� ��ġ ã��
    for (const CString& loadIP : m_strLoadIPAddress)
    {
        if (deviceMap.find(loadIP) != deviceMap.end())
        {
            m_strSetIPAddress.push_back(loadIP);
            m_strSetModelName.push_back(deviceMap[loadIP]);
        }
    }

    SetDeviceCount(static_cast<int>(m_strSetIPAddress.size()));

    // ��� ��� �� ���Ϳ� ����
    int nDeviceCnt = static_cast<int>(m_strSetIPAddress.size());
    for (int i = 0; i < nDeviceCnt; i++)
    {
        CString strValue = m_strSetIPAddress[i];
        CString strModelName = m_strSetModelName[i]; // �� �̸� ��������

        CString strLog;
        strLog.Format(_T("[Camera[%d]] IP Address[%s], Model Name[%s]"), i + 1, static_cast<LPCTSTR>(strValue), static_cast<LPCTSTR>(strModelName));
        Common::GetInstance()->AddLog(0, strLog);

        Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    }
}

// =============================================================================
// ���⿡�� a�� b�� �ʵ带 ���ϰ�, �ߺ� ���θ� ��ȯ.
// ���� ���, ���� ID�� ���ϴ� ������� ����.
bool CameraManager::CompareDeviceInfo(const PvDeviceInfo & a, const PvDeviceInfo & b)
{

    return a.GetConnectionID() == b.GetConnectionID();
}

// =============================================================================
void CameraManager::SetCameraIPAddress(CString _IPAddress)
{
    m_strLoadIPAddress.push_back(_IPAddress);

    if (REMOTE_DEBUG)
    {
        CString strLog = _T("");
        Common::GetInstance()->AddLog(0, _T("------------------------------------"));

        m_strLoadIPAddress.push_back(_IPAddress);

        strLog.Format(_T("Setfile Load Device Camera Index[%d]"), m_strLoadIPAddress.size());
        Common::GetInstance()->AddLog(0, strLog);

        strLog.Format(_T("Setfile Load IPAddress = [%s]"), _IPAddress);
        Common::GetInstance()->AddLog(0, strLog);

        Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    }
}

// =============================================================================
CString CameraManager::GetCameraIPAddress(int nIndex)
{
    return m_strLoadIPAddress.at(nIndex);
}

// =============================================================================
// ����� ī�޶� ��ü ����
bool CameraManager::CreateCamera(int cameraIndex)
{
    CMDS_Ebus_SampleDlg* MainDlg = (CMDS_Ebus_SampleDlg*)AfxGetApp()->GetMainWnd();

    if (cameraIndex >= 0 && cameraIndex < CAMERA_COUNT)
    {
        if (m_Cam[cameraIndex] == nullptr)
        {
            m_Cam[cameraIndex] = new CameraControl_rev(cameraIndex);
            m_Cam[cameraIndex]->CameraManagerLink(MainDlg->m_CamManager);
            return true;
        }
    }
    return false;
}

// =============================================================================
// ����� ī�޶� ��ü ����
bool CameraManager::CameraAllStart(CMDS_Ebus_SampleDlg* MainDlg)
{
    CString strLog = _T("");

    strLog.Format(_T("-----------Camera Start------------"));
    Common::GetInstance()->AddLog(0, strLog);

    bool bFlag = false;
    int nCameraCnt = GetDeviceCount();
    int npercent = 100 / nCameraCnt;
    if (nCameraCnt < 0)
    {
        MainDlg->gui_status = (GUI_STATUS)GUI_STEP_IDLE;
        return bFlag;
    }
    else if (nCameraCnt >= 1)
    {
        for (int i = 0; i < nCameraCnt; i++)
        {
            if (m_Cam[i] == NULL)
            {
                CreateCamera(i);
                m_Cam[i]->CameraSequence(i);
            }
            else
            {
                if (m_Cam[i]->GetStartFlag())
                {
                    m_Cam[i]->CameraSequence(i);
                }
                else
                {
                    m_Cam[i]->ReStartSequence(i);
                }
            }
        }
    }

    bFlag = true;
    MainDlg->gui_status = GUI_STATUS::GUI_STEP_RUN;

    return bFlag;
}

// =============================================================================
bool CameraManager::CameraAllStop(CMDS_Ebus_SampleDlg* MainDlg)
{
    bool bFlag = false;
    for (int i = 0; i < GetDeviceCount(); i++)
    {
        
        bFlag = m_Cam[i]->CameraStop(i);
        m_Cam[i]->SetRunningFlag(false);
        Sleep(1);
    }

    MainDlg->gui_status = GUI_STATUS::GUI_STEP_STOP;
    Common::GetInstance()->AddLog(0, _T("All Camera Streaming Stop"));

    return bFlag;
}

// =============================================================================
bool CameraManager::CameraAllDisConnect(CMDS_Ebus_SampleDlg* MainDlg)
{
    CString strLog = _T("");
    bool bFlag = false;
    int nCnt = GetDeviceCount();
    for (int i = 0; i < nCnt; i++)
    {
        m_Cam[i]->m_bThreadFlag = false;
        m_Cam[i]->SetRunningFlag(false);
        strLog.Format(_T("[Camera[%d]],Thread Stop"), i + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    Common::GetInstance()->AddLog(0, _T("------------------------------------"));

    for (int i = 0; i < nCnt; i++)
    {
        bFlag = m_Cam[i]->CameraStop(i);
        Sleep(100);
        m_Cam[i]->CameraDisconnect();

        strLog.Format(_T("[Camera[%d]] CameraStop & Disconnect"), i + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    MainDlg->gui_status = GUI_STATUS::GUI_STEP_DISCONNECT;
    Common::GetInstance()->AddLog(0, _T("------------------------------------"));

    return bFlag;
}