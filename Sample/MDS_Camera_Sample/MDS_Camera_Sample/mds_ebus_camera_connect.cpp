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
    // GigE Vision ��ġ�� ���, GigE Vision Ư�� ��Ʈ���� �Ķ���͸� ����

    CString strLog = _T("");
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV*>(aDevice);

    if (lDeviceGEV != nullptr)
    {
        PvStreamGEV* lStreamGEV = static_cast<PvStreamGEV*>(aStream);
        // ��Ŷ ũ�� ����
        lDeviceGEV->NegotiatePacketSize();
        // ��ġ ��Ʈ���� ��� ����
        lDeviceGEV->SetStreamDestination(lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort());
    }
}

PvPipeline* CreatePipeline(PvPipeline* Pipeline, PvDevice* aDevice, PvStream* aStream)
{
    PvPipeline* lPipeline = nullptr; // PvPipeline ������ �ʱ�ȭ
    if (aDevice != nullptr)
    {
        uint32_t lSize = aDevice->GetPayloadSize(); // ��ġ�� ���̷ε� ũ�� ȹ��

        if (aStream != nullptr)
        {
            // ��Ʈ���� �����ϴ� ��� ���ο� PvPipeline ����
            lPipeline = new PvPipeline(aStream);
        }
        else if (aDevice != nullptr)
        {
            // ��Ʈ���� ���� ��� �̹� �����ϴ� m_Pipeline Ȱ��
            Pipeline->SetBufferCount(BUFFER_COUNT);
            Pipeline->SetBufferSize(lSize);
            lPipeline = Pipeline;
        }
        
        if (lPipeline != nullptr)
        {
            // Pipeline ���� (BufferCount, BufferSize)
            lPipeline->SetBufferCount(BUFFER_COUNT);
            lPipeline->SetBufferSize(lSize);
        }
    }
    return lPipeline; // ������ �Ǵ� ������ PvPipeline ������ ��ȯ
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

    // ����ڷκ��� ī�޶� IP �Է� �ޱ�
    std::wcout << L"IP Address : ";
    std::wstring input;
    std::getline(std::wcin, input);

    // CString���� ��ȯ
    strAddress = input.c_str();

    // ī�޶� ��ġ�� ����
    Device = ConnectToDevice(Device, strAddress);
    if (Device != nullptr)
    {
        printf("Device Connect = [success]\n");
    }


    // ī�޶� ��Ʈ�� ����
    Stream = OpenStream(Stream, strAddress);
    if (Stream != nullptr)
    {
        printf("Stream Open = [success]\n");
    }


    // ī�޶� ��Ʈ�� ����
    ConfigureStream(Device, Stream);

    if (Stream != nullptr && Device != nullptr)
    {
        printf("Stream Configure = [success]\n");
    }


    if (Stream != nullptr && Device != nullptr)
    {
        // ī�޶� ���������� ����
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