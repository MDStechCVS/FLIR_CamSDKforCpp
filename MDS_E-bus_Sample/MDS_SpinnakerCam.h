#pragma once

#include"global.h"
#include "CameraManager.h"

class ImageProcessor;
class CMDS_Ebus_SampleDlg;

class MDS_SpinnakerCam : public CameraManager
{

private:

	int m_nCamIndex; // ī�޶� �ε��� ���� ����
	bool m_bThreadFlag; // ������ ���� ���� ����
	bool m_bCamRunning; // ������ ���� ���� ����
	bool m_bStart; // ī�޶� ���� ���� �÷���

private:
	ThreadStatus m_TStatus; // ������ ����
	double m_dCam_FPS;
	bool m_bReStart; // ��Ŀ���� ������ ����
	ImageProcessor* ImgProc;

public:
	Camera_Parameter* m_Cam_Params; // ī�޶� �Ķ���� ����ü
	CameraModelList m_Camlist;
	CWinThread* pThread[CAMERA_COUNT];
	CameraManager* Manager;
	PvPixelType m_pixeltype;

public:

	MDS_SpinnakerCam(int nIndex);
	virtual ~MDS_SpinnakerCam();

	ERRORCODE CameraStart();
	ERRORCODE CameraStop();
	ERRORCODE CameraDisconnect();
	ERRORCODE IsConnect();

	bool Camera_destroy(); // ī�޶� ����� ���� �Ҵ�� �޸� ����

	/*Status*/
	void SetCamRunningFlag(bool bFlag); // ī�޶� ���� �÷��� 
	bool GetCamRunningFlag();

	void SetReStartFlag(bool bFlag); // ��Ŀ���� ������ �÷���
	bool GetReStartFlag();

	void SetThreadFlag(bool bFlag);
	bool GetThreadFlag();

	void SetStartFlag(bool bFlag);
	bool GetStartFlag();

	void SetCamIndex(int nIndex);
	int GetCamIndex();

	

};

