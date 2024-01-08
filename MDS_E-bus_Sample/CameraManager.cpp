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
// 연결되어있는 카메라 개수 반환
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
// 설정 파일에 저장되어 있는 IP Address와 물리적으로 연결되어 있는 IP Address를 비교하여 
// 저장된 IP와 연결되어 있는 IP가 동일하다면  IP Address를 구조체에 저장한다.
/*
void CameraManager::CameraDeviceFind(CMDS_Ebus_SampleDlg* MainDlg)
{
    if (MainDlg->gui_status == GUI_STEP_RUN)
        return;
    PvSystem lSystem;
    lSystem.Find();

    CString strlnterfaceID = _T("");

    if (!lDIVector.empty())
        lDIVector.clear();

    uint32_t nIntercafeCount = lSystem.GetInterfaceCount();
    int nIndex = 0;
    for (uint32_t i = 0; i < nIntercafeCount; i++)
    {
        const PvInterface* lInterface = dynamic_cast<const PvInterface*>(lSystem.GetInterface(i));
        if (lInterface != NULL)
        {
            uint32_t nLocalCnt = lInterface->GetDeviceCount();
            for (uint32_t j = 0; j < nLocalCnt; j++)
            {
                const PvDeviceInfo* lDI = dynamic_cast<const PvDeviceInfo*>(lInterface->GetDeviceInfo(j));
                if (lDI != NULL)
                {                       
                    strlnterfaceID.Format(_T("%s"), static_cast<LPCTSTR>(lDI->GetConnectionID()));
                    
                    // strLoadIPAddress에서 lDI와 일치하는 위치 찾기
                    auto it = std::find(m_strLoadIPAddress.begin(), m_strLoadIPAddress.end(), strlnterfaceID);
                    if (it != m_strLoadIPAddress.end())
                    {
                        int nIndex = (int)std::distance(m_strLoadIPAddress.begin(), it);
                        bool exists = false;
                        for (const PvDeviceInfo* deviceInfo : lDIVector)
                        {
                            // 중복을 확인.
                            if (CompareDeviceInfo(*lDI, *deviceInfo))
                            {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists)
                        {
                            if (lDIVector.empty())
                            {
                                lDIVector.push_back(lDI);
                                m_strLoadIPAddress.erase(it); // 삽입한 위치의 IP 주소는 삭제
                            }
                            else
                            {
                                // nIndex 위치에 lDI 삽입
                                nIndex = std::min(nIndex, static_cast<int>(lDIVector.size()));
                                lDIVector.insert(lDIVector.begin() + nIndex, lDI);
                                m_strLoadIPAddress.erase(it); // 삽입한 위치의 IP 주소는 삭제
                            }
                        }
                    }
                }
            }
        }
    }

    std::sort(m_strLoadIPAddress.begin(), m_strLoadIPAddress.end());
    m_strLoadIPAddress.erase(unique(m_strLoadIPAddress.begin(), m_strLoadIPAddress.end()), m_strLoadIPAddress.end());

    std::sort(lDIVector.begin(), lDIVector.end());
    lDIVector.erase(unique(lDIVector.begin(), lDIVector.end()), lDIVector.end());

    CString strLog = _T("");
    strLog.Format(_T("Setfile Load Device Count [%d]"), m_strLoadIPAddress.size());
    Common::GetInstance()->AddLog(0, strLog);

    strLog.Format(_T("Contact Device Count [%d]"), lDIVector.size());
    Common::GetInstance()->AddLog(0, strLog);

    int nDeviceCnt = (int)lDIVector.size();

    
    PvString strTemp;
    for (int i = 0; i < nDeviceCnt; i++)
    {
        if (i < lDIVector.size())
        {
            strTemp = lDIVector.at(i)->GetModelName();
            strLog.Format(_T("[Camera_%d]Model = [%s]"), i + 1, static_cast<LPCTSTR>(strTemp));
            Common::GetInstance()->AddLog(0, strLog);
            CString strValue = (_T(""));
            strValue.Format(_T("%s"), static_cast<LPCTSTR>(lDIVector.at(i)->GetModelName()));
            m_strSetModelName.push_back(strValue);

            strTemp = lDIVector.at(i)->GetConnectionID();
            strValue.Format(_T("%s"), static_cast<LPCTSTR>(lDIVector.at(i)->GetConnectionID()));
            m_strSetIPAddress.push_back(strValue);

            strLog.Format(_T("[Camera_%d] IP Address[%s]"), i + 1, static_cast<LPCTSTR>(m_strSetIPAddress.at(i)));
            Common::GetInstance()->AddLog(0, strLog);
         
            Common::GetInstance()->AddLog(0, _T("------------------------------------"));
        }
    }
    int nCnt = (int)m_strSetIPAddress.size();
    SetDeviceCount(nCnt);
}
*/
void CameraManager::CameraDeviceFind(CMDS_Ebus_SampleDlg* MainDlg)
{
    if (MainDlg->gui_status == GUI_STATUS::GUI_STEP_RUN)
        return;

    PvSystem lSystem;
    lSystem.Find();

    uint32_t nInterfaceCount = lSystem.GetInterfaceCount();

    // 먼저 모든 장치 정보를 저장
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

    // m_strLoadIPAddress 순서대로 일치하는 장치 찾기
    for (const CString& loadIP : m_strLoadIPAddress)
    {
        if (deviceMap.find(loadIP) != deviceMap.end())
        {
            m_strSetIPAddress.push_back(loadIP);
            m_strSetModelName.push_back(deviceMap[loadIP]);
        }
    }

    // 중복 제거 후 정렬
    //std::sort(m_strSetIPAddress.begin(), m_strSetIPAddress.end());
    //std::sort(m_strSetModelName.begin(), m_strSetModelName.end());

    SetDeviceCount(static_cast<int>(m_strSetIPAddress.size()));

    // 결과 출력 및 벡터에 저장
    int nDeviceCnt = static_cast<int>(m_strSetIPAddress.size());
    for (int i = 0; i < nDeviceCnt; i++)
    {
        CString strValue = m_strSetIPAddress[i];
        CString strModelName = m_strSetModelName[i]; // 모델 이름 가져오기

        CString strLog;
        strLog.Format(_T("[Camera_%d] IP Address[%s], Model Name[%s]"), i + 1, static_cast<LPCTSTR>(strValue), static_cast<LPCTSTR>(strModelName));
        Common::GetInstance()->AddLog(0, strLog);

        Common::GetInstance()->AddLog(0, _T("------------------------------------"));
    }

    SetDeviceCount(nDeviceCnt);
}


// =============================================================================
// 여기에서 a와 b의 필드를 비교하고, 중복 여부를 반환.
// 예를 들어, 연결 ID를 비교하는 방식으로 구현.
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
// 연결된 카메라 객체 생성
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
// 연결된 카메라 전체 시작
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
        strLog.Format(_T("[Camera_%d],Thread Stop"), i + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    Common::GetInstance()->AddLog(0, _T("------------------------------------"));

    for (int i = 0; i < nCnt; i++)
    {
        bFlag = m_Cam[i]->CameraStop(i);
        Sleep(100);
        m_Cam[i]->CameraDisconnect();

        strLog.Format(_T("[Camera_%d] CameraStop & Disconnect"), i + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }

    MainDlg->gui_status = GUI_STATUS::GUI_STEP_DISCONNECT;
    Common::GetInstance()->AddLog(0, _T("------------------------------------"));

    return bFlag;
}