#pragma once

#include "global.h"
#include "MDS_E-bus_SampleDlg.h"

class CameraControl_rev;

class CameraManager
{
public:

	CameraManager();
	virtual ~CameraManager();

private:

	int m_nDeviceCnt;
public:

	void ManagerDestroy();
	void InitData();
	int  GetDeviceCount();
	void SetDeviceCount(int nCnt);
	void CameraDeviceFind(CMDS_Ebus_SampleDlg* MainDlg);
	void SetCameraIPAddress(CString _IPAddress);
	bool CameraAllStart(CMDS_Ebus_SampleDlg* MainDlg);
	bool CameraAllStop(CMDS_Ebus_SampleDlg* MainDlg);
	bool CameraAllDisConnect(CMDS_Ebus_SampleDlg* MainDlg);
	CString GetCameraIPAddress(int nIndex);
	bool CompareDeviceInfo(const PvDeviceInfo& a, const PvDeviceInfo& b);
	void ClearAddressdata();
	bool CreateCamera(int cameraIndex);

public:

	vector<const PvDeviceInfo*> lDIVector;
	vector<CString> m_strLoadIPAddress;
	vector<CString> m_strSetIPAddress;
	vector<CString> m_strSetModelName;
	CameraControl_rev* m_Cam[CAMERA_COUNT];
};

