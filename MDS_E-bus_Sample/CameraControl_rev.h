#pragma once

#include "global.h"
#include "CameraManager.h"

class CMDS_Ebus_SampleDlg;
class ImageProcessor;

class CameraControl_rev : public CameraManager
{
private:

	int m_nCamIndex; // ī�޶� �ε��� ���� ����
	bool m_bThreadFlag; // ������ ���� ���� ����
	bool m_bCamRunning; // ������ ���� ���� ����
	bool m_bStart; // ī�޶� ���� ���� �÷���
public:

	CameraControl_rev(int nIndex);
	virtual ~CameraControl_rev();
	
private:

	ThreadStatus m_TStatus; // ������ ����
	double m_dCam_FPS;
	bool m_bReStart; // ��Ŀ���� ������ ����
	ImageProcessor* ImgProc;

public:

	Camera_Parameter* m_Cam_Params; // ī�޶� �Ķ���� ����ü
	CameraModelList m_Camlist;

	PvDevice* m_Device;
	PvStream* m_Stream;
	PvPipeline* m_Pipeline;

	CWinThread* pThread[CAMERA_COUNT];
	CameraManager* Manager;
	PvPixelType m_pixeltype;

private:

	/*Thread Func*/
	static UINT ThreadCam(LPVOID _mothod); // ī�޶� ������
	void StartThreadProc(int nIndex);
	void SetCameraFPS(double dValue); // ī�޶� FPS ����
	DWORD ConvertHexValue(const CString& hexString); // CString to Hex
	
	/*Utill&Init*/
	void Initvariable();
	/*Camera Func*/
	PvDevice* ConnectToDevice(int nIndex); // PvDevice ���� �Լ�
	PvStream* OpenStream(int nIndex); // PvStream Open �Լ�
	void ConfigureStream(PvDevice* aDevice, PvStream* aStream, int nIndex); // Stream �Ķ���� ����
	PvPipeline* CreatePipeline(PvDevice* aDevice, PvStream* aStream, int nIndex); // ���������� ������ �Լ�
	int SetStreamingCameraParameters(PvGenParameterArray* lDeviceParams, int nIndex, CameraModelList Camlist); // ��Ʈ���� �� �Ķ���� �����ϴ� �Լ�
	CameraModelList FindCameraModel(int nCamIndex); // ī�޶� �� �̸� ã��
	bool CameraParamSetting(int nIndex, PvDevice* aDevice); // ī�޶� �Ķ���� ����
	 /*-----------------------------------------*/
	 /*Image Func*/
	void DataProcessing(PvBuffer* aBuffer, int nIndex); // �̹��� �İ��� ������
	PvImage* GetImageFromBuffer(PvBuffer* aBuffer); // �̹��� ���� Ŭ���� ������ ȹ��
	uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
	void SetupImagePixelType(PvImage* lImage);
	unsigned char* GetImageDataPointer(PvImage* lImage);
	bool IsValidBuffer(PvBuffer* aBuffer);
	bool IsInvalidState(int nIndex, PvBuffer* buffer);
	 /*-----------------------------------------*/
public:

	ImageProcessor* GetImageProcessor() const;
	bool DestroyThread(void); // ������ �Ҹ�
	void CameraManagerLink(CameraManager* _link);
	void CreateImageProcessor(int nIndex);
	
	/*Camera Func*/
	bool CameraSequence(int nIndex); // ī�޶� ������
	bool CameraStart(int nIndex); // ī�޶� ����
	bool CameraStop(int nIndex); // ī�޶� ����
	void CameraDisconnect(); // ī�޶� ���� ����
	double GetCameraFPS(); // ī�޶� FPS ���
	bool AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex); // �Ķ���� ����
	int ReStartSequence(int nIndex); // �ٽ� ���� ������
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

	bool bbtnDisconnectFlag;
	/*-----------------------------------------*/
};