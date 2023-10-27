#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <atlstr.h>
#include <conio.h>

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

void StreamingImage(bool bFlag, PvPipeline* Pipeline)
{
    PvBuffer* lBuffer; // PvBuffer 객체
    PvResult lOperationResult = -1; // 작업 결과 변수 초기화
    PvImage* lImage = nullptr;

    while(bFlag)
    {
        PvResult lResult = Pipeline->RetrieveNextBuffer(&lBuffer, 1000, &lOperationResult);

        if (lResult.IsOK())
        {
            Pipeline->ReleaseBuffer(lBuffer);           
            lImage = GetImageFromBuffer(lBuffer);

            unsigned char* ptr = GetImageDataPointer(lImage);

            cv::Mat imageMat(lImage->GetHeight(), lImage->GetWidth(), CV_8UC1, ptr, cv::Mat::AUTO_STEP);
            if (lImage->GetHeight() > 1 && lImage->GetWidth() > 1)
            {
                try 
                {
                    printf("[Height = %d] [Width = %d] [PixelSize = %d]\n", lImage->GetHeight(), lImage->GetWidth(), lImage->GetHeight()* lImage->GetWidth());
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
}

int main()
{
    PvDevice* Device = nullptr;
    PvStream* Stream = nullptr;
    PvPipeline* Pipeline = nullptr;
    bool bFlag = false;
    PvResult result = -1;
    CString strAddress = "";
    
    // 사용자로부터 카메라 IP 입력 받기
    std::wcout << L"IP Address : ";
    std::wstring input;
    std::getline(std::wcin, input);

    // CString으로 변환
    strAddress = input.c_str();

    // 카메라 장치에 연결
    Device = ConnectToDevice(Device, strAddress);
    if (Device != nullptr)
    {
        printf("Device Connect = [success]\n");
    }

    // 카메라 스트림 오픈
    Stream = OpenStream(Stream, strAddress);
    if (Stream != nullptr)
    {
        printf("OpenStream = [success]\n");
    }
    // 카메라 스트림 설정
    ConfigureStream(Device, Stream);

    if (Stream != nullptr && Device != nullptr)
    {
        printf("ConfigureStream = [success]\n");
    }

    if (Stream != nullptr && Device != nullptr)
    {
        // 카메라 파이프라인 생성
        Pipeline = CreatePipeline(Pipeline, Device, Stream);
        if (Pipeline != nullptr)
        {
            printf("CreatePipeline = [success]\n");
            // 파이프 라인 생성까지 완료된다면, StreamingImage 진입 전 처리 완료
            bFlag = true;
        }          
    }
    // 카메라 파라미터 설정
    if (bFlag)
    {
        PvGenParameterArray* lDeviceParams = Device->GetParameters();
        // GenICam AcquisitionStart 및 AcquisitionStop 명령 
        PvGenCommand* lStart = dynamic_cast<PvGenCommand*>(lDeviceParams->Get("AcquisitionStart"));

        // 스트리밍 사전 준비
        result = Pipeline->Reset();
        result = Pipeline->Start();

        if (result.IsOK())
        {
            result = Device->StreamEnable();
            result = lStart->Execute();

            StreamingImage(bFlag, Pipeline);
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

