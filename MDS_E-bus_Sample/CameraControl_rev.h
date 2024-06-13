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

    int m_nCamIndex; // 카메라 인덱스 저장 변수
    bool m_bThreadFlag; // 쓰레드 동작 상태 변수
    bool m_bCamRunning; // 시퀀스 동작 상태 변수
    bool m_bStart; // 카메라 시작 여부 플래그
    ThreadStatus m_TStatus; // 스레드 상태
    double m_dCam_FPS;
    bool m_bReStart; // 리커버리 시퀀스 변수
    PvBuffer* m_Buffer;
    ImageProcessor* ImgProc;
    CMMTiming mm_timer;     // MM 타이머 객체
    std::queue<PvBuffer*> bufferQueue; // 이미지 데이터 버퍼를 저장하는 큐

    // Image Proc Thread
    std::thread imageProcessingThread;
    std::atomic<bool> ImgProcThreadRunning;

public:

    std::mutex bufferMutex;  // 버퍼에 대한 접근을 동기화하기 위한 뮤텍스
    std::condition_variable bufferNotEmpty; // 버퍼 대기

    Camera_Parameter* m_Cam_Params; // 기존 Camera_Parameter 구조체 사용
    TauPlanckConstants* m_tauPlanckConstants; // 온도 계산 파라미터
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
    static UINT ThreadCam(LPVOID _mothod); // 카메라 스레드
    // Image Proc Thread
    void StartImageProcessingThread(PvBuffer* aBuffer, int nIndex);
    void StopImageProcessingThread();

    void StartThreadProc(int nIndex);
    void SetCameraFPS(double dValue); // 카메라 FPS 설정
    DWORD ConvertHexValue(const CString& hexString); // CString to Hex

    void SetBuffer(PvBuffer* aBuffer);

    /* Utill&Init */
    void Initvariable();
    /* Camera Func */
    PvDevice* ConnectToDevice(int nIndex); // PvDevice 연결 함수
    PvStream* OpenStream(int nIndex); // PvStream Open 함수
    void ConfigureStream(PvDevice* aDevice, PvStream* aStream, int nIndex); // Stream 파라미터 설정
    PvPipeline* CreatePipeline(PvDevice* aDevice, PvStream* aStream, int nIndex); // 파이프라인 생성자 함수

    CameraModelList FindCameraModel(int nCamIndex, PvGenParameterArray* lDeviceParams); // 카메라 모델 이름 찾기
    bool CameraParamSetting(int nIndex, PvDevice* aDevice); // 카메라 파라미터 설정
    /* Image Func */
    void DataProcessing(PvBuffer* aBuffer, int nIndex); // 이미지 후가공 시퀀스
    PvImage* GetImageFromBuffer(PvBuffer* aBuffer); // 이미지 버퍼 클래스 포인터 획득
    uint16_t* ConvertToUInt16Pointer(unsigned char* ptr);
    void SetupImagePixelType(PvImage* lImage);
    unsigned char* GetImageDataPointer(PvImage* lImage);
    bool IsValidBuffer(PvBuffer* aBuffer);
    bool IsInvalidState(int nIndex, PvBuffer* buffer);
public:
    int SetStreamingCameraParameters(PvGenParameterArray* lDeviceParams, int nIndex, CameraModelList Camlist); // 스트리밍 시 파라미터 설정하는 함수
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
    bool DestroyThread(void); // 스레드 소멸
    void CameraManagerLink(CameraManager* _link);
    void CreateImageProcessor(int nIndex);

    /* Camera Func */
    bool CameraSequence(int nIndex); // 카메라 시퀀스
    bool CameraStart(int nIndex); // 카메라 시작
    bool CameraStop(int nIndex); // 카메라 정지
    void CameraDisconnect(); // 카메라 연결 해제
    double GetCameraFPS(); // 카메라 FPS 얻기
    bool AcquireParameter(PvDevice* aDevice, PvStream* aStream, PvPipeline* aPipeline, int nIndex); // 파라미터 설정
    int ReStartSequence(int nIndex); // 다시 시작 시퀀스
    bool Camera_destroy(); // 카메라 종료시 동적 할당된 메모리 정리

    /* Status */
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