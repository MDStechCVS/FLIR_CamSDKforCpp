#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <atlstr.h>

#include <PvSampleUtils.h>
#include <PvDevice.h>
#include <PvBuffer.h>
#include <PvStream.h>

#include <PvStreamGEV.h>
#include <PvPipeline.h>
#include <PvDeviceGEV.h>
#include <PvDeviceInfoGEV.h>

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

int main() 
{
 
    PvDevice *Device = nullptr;
    PvStream *Stream = nullptr;
    PvPipeline* Pipeline = nullptr;
    bool bFlag = false;
    CString strAddress = "";

    printf("MDS Camera Sample\n");
    printf("-------------------------------------------\n");

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
        printf("Stream Open = [success]\n");
    }


    // 카메라 스트림 설정
    ConfigureStream(Device, Stream);

    if (Stream != nullptr && Device != nullptr)
    {
        printf("Stream Configure = [success]\n");
    }


    if (Stream != nullptr && Device != nullptr)
    {
        // 카메라 파이프라인 생성
        Pipeline = CreatePipeline(Pipeline, Device, Stream);
        if (Pipeline != nullptr)
        {
            printf("Pipeline Create = [success]\n");
        }
    }

    delete Pipeline;
    Pipeline = nullptr;

    return 0;
}