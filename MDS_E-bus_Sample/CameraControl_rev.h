#pragma once

#include "global.h"
#include "CameraManager.h"

class CMDS_Ebus_SampleDlg;
class ImageProcessor;

struct IRFormatArray
{
	static const TCHAR* IRFormatStrings[];
};




class CameraControl_rev : public CameraManager
{

public:
    CameraControl_rev(int nIndex);
    virtual ~CameraControl_rev();

private:

    int m_nCamIndex; // ī�޶� �ε��� ���� ����
    bool m_bThreadFlag; // ������ ���� ���� ����
    bool m_bCamRunning; // ������ ���� ���� ����
    bool m_bStart; // ī�޶� ���� ���� �÷���
    ThreadStatus m_TStatus; // ������ ����
    double m_dCam_FPS;
    bool m_bReStart; // ��Ŀ���� ������ ����
    PvBuffer* m_Buffer;
    ImageProcessor* ImgProc;
    CMMTiming mm_timer;     // MM Ÿ�̸� ��ü
    std::queue<PvBuffer*> bufferQueue; // �̹��� ������ ���۸� �����ϴ� ť

    // Image Proc Thread
    std::thread imageProcessingThread;
    std::atomic<bool> ImgProcThreadRunning;

public:

    std::mutex bufferMutex;  // ���ۿ� ���� ������ ����ȭ�ϱ� ���� ���ؽ�
    std::condition_variable bufferNotEmpty; // ���� ���

    Camera_Parameter* m_Cam_Params; // ���� Camera_Parameter ����ü ���
    TauPlanckConstants* m_tauPlanckConstants; // �µ� ��� �Ķ����
    ObjectParams* m_objectParams; // object Params
    SpectralResponseParams* m_spectralResponseParams; //
    CameraModelList m_Camlist;

    PvDevice* m_Device;
    PvStream* m_Stream;
    PvPipeline* m_Pipeline;

    CWinThread* pThread[CAMERA_COUNT];
    CameraManager* Manager;
    PvPixelType m_pixeltype;
    CameraIRFormat m_IRFormat;



    bool m_bMeasCapable;
    bool m_bTLUTCapable; // Capacity to generate temperature linear images
    int m_WindowingMode;
    bool bbtnDisconnectFlag;

private:

    /* Thread Func */
    static UINT ThreadCam(LPVOID _mothod); // ī�޶� ������
    // Image Proc Thread
    void StartImageProcessingThread(PvBuffer* aBuffer, int nIndex);
    void StopImageProcessingThread();

    void StartThreadProc(int nIndex);
    void SetCameraFPS(double dValue); // ī�޶� FPS ����
    DWORD ConvertHexValue(const CString& hexString); // CString to Hex

    void SetBuffer(PvBuffer* aBuffer);

    /* Utill&Init */
    void Initvariable();
    /* Camera Func */
    PvDevice* ConnectToDevice(int nIndex); // PvDevice ���� �Լ�
    PvStream* OpenStream(int nIndex); // PvStream Open �Լ�
    void ConfigureStream(PvDevice* aDevice, PvStream* aStream, int nIndex); // Stream �Ķ���� ����
    PvPipeline* CreatePipeline(PvDevice* aDevice, PvStream* aStream, int nIndex); // ���������� ������ �Լ�

    CameraModelList FindCameraModel(int nCamIndex, PvGenParameterArray* lDeviceParams); // ī�޶� �� �̸� ã��
    bool CameraParamSetting(int nIndex, PvDevice* aDevice); // ī�޶� �Ķ���� ����
    /* Image Func */
    void DataProcessing(PvBuffer* aBuffer, int nIndex); // �̹��� �İ��� ������
    PvImage* GetImageFromBuffer(PvBuffer* aBuffer); // �̹��� ���� Ŭ���� ������ ȹ��
    uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
    void SetupImagePixelType(PvImage* lImage);
    unsigned char* GetImageDataPointer(PvImage* lImage);
    bool IsValidBuffer(PvBuffer* aBuffer);
    bool IsInvalidState(int nIndex, PvBuffer* buffer);
public:
    int SetStreamingCameraParameters(PvGenParameterArray* lDeviceParams, int nIndex, CameraModelList Camlist); // ��Ʈ���� �� �Ķ���� �����ϴ� �Լ�
    void OnParameterUpdate(PvGenParameter* aParameter);
    void UpdateCalcParams();
    void UpdateDeviceOP();
    void doUpdateCalcConst();
    double doCalcAtmTao(void);
    double doCalcK1(void);
    double doCalcK2(double dAmbObjSig, double dAtmObjSig, double dExtOptTempObjSig);
    CTemperature imgToTemp(long lPixval);
    CTemperature objSigToTemp(double dObjSig);
    double imgToPow(long lPixval);
    double powToObjSig(double dPow);
    double tempToObjSig(double dblKelvin);
    USHORT tempToImg(double dKelvin);
    double objSigToPow(double dObjSig);
    USHORT powToImg(double dPow);

    PvBuffer* GetBuffer();
    ImageProcessor* GetImageProcessor() const;
    bool DestroyThread(void); // ������ �Ҹ�
    void CameraManagerLink(CameraManager* _link);
    void CreateImageProcessor(int nIndex);

    /* Camera Func */
    bool CameraSequence(int nIndex); // ī�޶� ������
    bool CameraStart(int nIndex); // ī�޶� ����
    bool CameraStop(int nIndex); // ī�޶� ����
    void CameraDisconnect(); // ī�޶� ���� ����
    double GetCameraFPS(); // ī�޶� FPS ���
    bool AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex); // �Ķ���� ����
    int ReStartSequence(int nIndex); // �ٽ� ���� ������
    bool Camera_destroy(); // ī�޶� ����� ���� �Ҵ�� �޸� ����

    /* Status */
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

    bool TempRangeSearch(int nIndex, PvGenParameterArray* lDeviceParams);
    bool SetTempRange(int nQureyIndex);

    CameraIRFormat GetIRFormat();
    void SetIRFormat(CameraIRFormat IRFormat);

    void SetIRRange(Camera_Parameter m_Cam_Params, int nQureyIndex);
    int GetIRRange();

    bool FFF_HeightSummary(PvGenParameterArray* lDeviceParams);
    int UpdateHeightForA50Camera(int& nHeight, int nWidth);
    void ClearQueue();
    void AddBufferToQueue(PvBuffer* buffer);
    PvBuffer* GetBufferFromQueue();
    int GetQueueBufferCount();
    PvBuffer* CopyBuffer(PvBuffer* originalBuffer);
    void SomeFunctionThatModifiesBuffer(PvBuffer* buffer);
    void LogIRFormat(int nIndex, PvGenParameterArray* lDeviceParams);
};