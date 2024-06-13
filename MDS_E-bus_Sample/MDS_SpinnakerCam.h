#pragma once

#include"global.h"
#include "CameraManager.h"

class ImageProcessor;
class CMDS_Ebus_SampleDlg;

class MDS_SpinnakerCam : public CameraManager
{

private:

	int m_nCamIndex; // 카메라 인덱스 저장 변수
	bool m_bThreadFlag; // 쓰레드 동작 상태 변수
	bool m_bCamRunning; // 시퀀스 동작 상태 변수
	bool m_bStart; // 카메라 시작 여부 플래그

private:
	ThreadStatus m_TStatus; // 스레드 상태
	double m_dCam_FPS;
	bool m_bReStart; // 리커버리 시퀀스 변수
	ImageProcessor* ImgProc;

public:
	Camera_Parameter* m_Cam_Params; // 카메라 파라미터 구조체
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

	bool Camera_destroy(); // 카메라 종료시 동적 할당된 메모리 정리

	/*Status*/
	void SetCamRunningFlag(bool bFlag); // 카메라 러닝 플래그 
	bool GetCamRunningFlag();

	void SetReStartFlag(bool bFlag); // 리커버리 시퀀스 플래그
	bool GetReStartFlag();

	void SetThreadFlag(bool bFlag);
	bool GetThreadFlag();

	void SetStartFlag(bool bFlag);
	bool GetStartFlag();

	void SetCamIndex(int nIndex);
	int GetCamIndex();

	

};

