#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <atlstr.h>
#include <conio.h>
#include <string>

#include <PvSampleUtils.h>
#include <PvDevice.h>
#include <PvBuffer.h>
#include <PvStream.h>

#include <PvStreamGEV.h>
#include <PvPipeline.h>
#include <PvDeviceGEV.h>
#include <PvDeviceInfoGEV.h>

#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2/imgproc.hpp>

#define BUFFER_COUNT			( 16 )

struct CameraInfo 
{
    CString strInterfaceID = "";
    CString strModelName = "";
    int nIndex = 0;
};

// =============================================================================
PvDevice* ConnectToDevice(PvDevice* Device, CString strAddress)
{
    PvResult lResult = -1;
    PvString ConnectionID = strAddress.GetBuffer(0);

    Device = PvDevice::CreateAndConnect(ConnectionID, &lResult);

    return Device;
}

PvStream* OpenStream(PvStream* Stream, CString strAddress)
{
    PvResult lResult = -1;
    PvString ConnectionID = (PvString)strAddress;

    Stream = PvStream::CreateAndOpen(ConnectionID, &lResult);

    return Stream;
}

void ConfigureStream(PvDevice* aDevice, PvStream* aStream)
{
    // GigE Vision 장치인 경우, GigE Vision 특정 스트리밍 파라미터를 설정

    CString strLog = _T("");
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV*>(aDevice);

    if (lDeviceGEV != nullptr)
    {
        PvStreamGEV* lStreamGEV = static_cast<PvStreamGEV*>(aStream);
        // 패킷 크기 협상
        lDeviceGEV->NegotiatePacketSize();
        // 장치 스트리밍 대상 설정
        lDeviceGEV->SetStreamDestination(lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort());
    }
}

PvPipeline* CreatePipeline(PvPipeline* Pipeline, PvDevice* aDevice, PvStream* aStream)
{
    PvPipeline* lPipeline = nullptr; // PvPipeline 포인터 초기화
    if (aDevice != nullptr)
    {
        uint32_t lSize = aDevice->GetPayloadSize(); // 장치의 페이로드 크기 획득

        if (aStream != nullptr)
        {
            // 스트림이 존재하는 경우 새로운 PvPipeline 생성
            lPipeline = new PvPipeline(aStream);
        }
        else if (aDevice != nullptr)
        {
            // 스트림이 없는 경우 이미 존재하는 m_Pipeline 활용
            Pipeline->SetBufferCount(BUFFER_COUNT);
            Pipeline->SetBufferSize(lSize);
            lPipeline = Pipeline;
        }

        if (lPipeline != nullptr)
        {
            // Pipeline 설정 (BufferCount, BufferSize)
            lPipeline->SetBufferCount(BUFFER_COUNT);
            lPipeline->SetBufferSize(lSize);
        }
    }
    return lPipeline; // 생성된 또는 설정된 PvPipeline 포인터 반환
}

PvImage* GetImageFromBuffer(PvBuffer* aBuffer)
{
    PvImage* lImage = aBuffer ? aBuffer->GetImage() : nullptr;

    if (!lImage)
    {

    }
    return lImage;
}

unsigned char* GetImageDataPointer(PvImage* image)
{
    return image ? image->GetDataPointer() : nullptr;
}

void StreamingImage(bool bFlag, PvPipeline* Pipeline, PvStream* stream)
{
    PvBuffer* lBuffer; // PvBuffer 객체
    PvResult lOperationResult = -1; // 작업 결과 변수 초기화
    PvImage* lImage = nullptr;
    PvGenParameterArray* lStreamParams = stream->GetParameters();
    double lFrameRateVal = 0.0; // 카메라 프레임 레이트 값을 저장하는 변수
    unsigned char* ptr = nullptr;
    PvGenFloat* lFrameRate = dynamic_cast<PvGenFloat*>(lStreamParams->Get("AcquisitionRate"));
    uint32_t nTemp = 1; // 버퍼 유효성 체크
    while(bFlag)
    {
        PvResult lResult = Pipeline->RetrieveNextBuffer(&lBuffer, 1000, &lOperationResult);

        if (lResult.IsOK())
        {
            if (lBuffer->GetOperationResult() == (PvResult::Code::CodeEnum::OK))
            {
                Pipeline->ReleaseBuffer(lBuffer);
                lImage = GetImageFromBuffer(lBuffer);
                ptr = GetImageDataPointer(lImage);

                cv::Mat imageMat(lImage->GetHeight(), lImage->GetWidth(), CV_8UC1, ptr, cv::Mat::AUTO_STEP);
                if (lImage->GetHeight() >= nTemp && lImage->GetWidth() >= nTemp && ptr != nullptr)
                {
                    try
                    {
                        lFrameRate->GetValue(lFrameRateVal);

                        printf("[FPS = %.2f][Height = %d] [Width = %d] [PixelSize = %d], [Code = %d]\n", lFrameRateVal,
                            lImage->GetHeight(), lImage->GetWidth(), lImage->GetHeight() * lImage->GetWidth(), lResult.GetCode());

                        cv::imshow("Live Buffer", imageMat);
                        int key = cv::waitKey(10);
                        if (key == 27)  // 27은 ESC 키 코드입니다.
                        {
                            break;      // ESC 키를 누르면 루프를 종료합니다.
                        }
                    }
                    catch (cv::Exception& e)
                    {
                        std::cerr << "Live Error: " << e.what() << std::endl;
                        // 예외 처리 코드 작성
                    }
                }
            }          
        }
        else
        {
            // 예외 상황 코드 분석
            printf("Result [Code = %d]\n", lResult.GetCode());
        }
    }
}

CameraInfo InputSelectedIndex(int maxIndex, const std::vector<CameraInfo>& cameraList)
{
    int selectedIdx = -1;
    int key = 0;
    while (selectedIdx < 1 || selectedIdx > maxIndex)
    {
        std::cout << "Select a camera by entering its index (1-" << maxIndex << "): ";
        std::cin >> selectedIdx;
        if (selectedIdx < 1 || selectedIdx > maxIndex)
        {
            std::cout << "Invalid index. Please enter a valid index." << std::endl;
        }

        if (key == 27)  // 27은 ESC 키 코드입니다.
        {
            break;      // ESC 키를 누르면 루프를 종료합니다.
        }
    }
    return cameraList[selectedIdx - 1]; // 선택한 인덱스에 해당하는 카메라 정보를 반환
}

CameraInfo CameraDeviceFind()
{
    PvSystem lSystem;
    lSystem.Find();

    std::vector<CameraInfo> cameraList; // 선택한 카메라 정보를 저장할 벡터

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
                    CString strInterfaceID, strModelname;
                    strInterfaceID.Format(_T("%s"), static_cast<LPCTSTR>(lDI->GetConnectionID()));
                    strModelname.Format(_T("%s"), static_cast<LPCTSTR>(lDI->GetModelName()));

                    CameraInfo cameraInfo;
                    cameraInfo.strInterfaceID = strInterfaceID;
                    cameraInfo.strModelName = strModelname;
                    cameraInfo.nIndex = cameraList.size();
                    cameraList.push_back(cameraInfo); // 카메라 정보를 벡터에 저장

                    printf("%d. Model = [%S] Address = [%S]\n", cameraList.size(), strModelname.GetString(), strInterfaceID.GetString());
                }
            }
        }
    }

    // 사용자로부터 인덱스 입력 받기
    CameraInfo selectedCamera = InputSelectedIndex(cameraList.size(), cameraList);

    return selectedCamera;

}

int main()
{
    PvDevice* Device = nullptr;
    PvStream* Stream = nullptr;
    PvPipeline* Pipeline = nullptr;
    bool bCheckFlag = false;
    PvResult result = -1;
    CString strAddress = "";
    int nDeviceCnt = 0;
    CameraInfo CamInfo;

    CamInfo = CameraDeviceFind();

    // 카메라 장치에 연결
    Device = ConnectToDevice(Device, CamInfo.strInterfaceID);
    if (Device != nullptr)
    {
        printf("%S Device Connect = [success]\n", CamInfo.strModelName);
    }

    // 카메라 스트림 오픈
    Stream = OpenStream(Stream, CamInfo.strInterfaceID);
    if (Stream != nullptr)
    {
        printf("%S OpenStream = [success]\n", CamInfo.strModelName);
    }
    // 카메라 스트림 설정
    ConfigureStream(Device, Stream);

    if (Stream != nullptr && Device != nullptr)
    {
        printf("%S ConfigureStream = [success]\n", CamInfo.strModelName);
    }

    if (Stream != nullptr && Device != nullptr)
    {
        // 카메라 파이프라인 생성
        Pipeline = CreatePipeline(Pipeline, Device, Stream);
        if (Pipeline != nullptr)
        {
            printf("CreatePipeline = [success]\n");
            // 파이프 라인 생성까지 완료된다면, StreamingImage 진입 전 처리 완료
            bCheckFlag = true;
        }          
    }
    // 카메라 파라미터 설정
    if (bCheckFlag == true)
    {
        PvGenParameterArray* lDeviceParams = Device->GetParameters();
        // GenICam AcquisitionStart 및 AcquisitionStop 명령 
        result = lDeviceParams->SetEnumValue("PixelFormat", PvPixelMono8); // Default setting

        PvGenCommand* lStart = dynamic_cast<PvGenCommand*>(lDeviceParams->Get("AcquisitionStart"));

        // 스트리밍 사전 준비
        result = Pipeline->Reset();
        result = Pipeline->Start();

        if (result.IsOK())
        {
            result = Device->StreamEnable();
            result = lStart->Execute();
            
            StreamingImage(bCheckFlag, Pipeline, Stream);
        }

        delete Pipeline;
        Pipeline = nullptr;
    }
    else
    {
        printf("Camera Connect = [fail]\n");
    }

	return 0;

}

