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

        if (key == 27)  // 27�� ESC Ű �ڵ��Դϴ�.
        {
            break;      // ESC Ű�� ������ ������ �����մϴ�.
        }
    }
    return cameraList[selectedIdx - 1]; // ������ �ε����� �ش��ϴ� ī�޶� ������ ��ȯ
}

CameraInfo CameraDeviceFind()
{
    PvSystem lSystem;
    lSystem.Find();

    std::vector<CameraInfo> cameraList; // ������ ī�޶� ������ ������ ����

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
                    cameraList.push_back(cameraInfo); // ī�޶� ������ ���Ϳ� ����

                    printf("%d. Model = [%S] Address = [%S]\n", cameraList.size(), strModelname.GetString(), strInterfaceID.GetString());
                }
            }
        }
    }

    // ����ڷκ��� �ε��� �Է� �ޱ�
    CameraInfo selectedCamera = InputSelectedIndex(cameraList.size(), cameraList);

    return selectedCamera;

}

int main()
{
	printf("MDS Camera Parameter Sample\n");
    printf("-------------------------------------------\n");

    PvDevice* Device = nullptr;
    PvStream* Stream = nullptr;
    PvPipeline* Pipeline = nullptr;
    bool bCheckFlag = false;
    PvResult result = -1;
    CString strAddress = "";
    int nDeviceCnt = 0;
    CameraInfo CamInfo;

    CamInfo = CameraDeviceFind();

    // ī�޶� ��ġ�� ����
    Device = ConnectToDevice(Device, CamInfo.strInterfaceID);
    if (Device != nullptr)
    {
        printf("%S Device Connect = [success]\n", CamInfo.strModelName);
    }

    // ī�޶� ��Ʈ�� ����
    Stream = OpenStream(Stream, CamInfo.strInterfaceID);
    if (Stream != nullptr)
    {
        printf("%S OpenStream = [success]\n", CamInfo.strModelName);
    }
    // ī�޶� ��Ʈ�� ����
    ConfigureStream(Device, Stream);

    if (Stream != nullptr && Device != nullptr)
    {
        printf("%S ConfigureStream = [success]\n", CamInfo.strModelName);
    }

    if (Stream != nullptr && Device != nullptr)
    {
        // ī�޶� ���������� ����
        Pipeline = CreatePipeline(Pipeline, Device, Stream);
        if (Pipeline != nullptr)
        {
            printf("CreatePipeline = [success]\n");
            // ������ ���� �������� �Ϸ�ȴٸ�, StreamingImage ���� �� ó�� �Ϸ�
            bCheckFlag = true;
        }
    }

    // ī�޶� �Ķ���͸� �޾ƿ��� ���� �������� ���� �� �Ķ���� �� �ҷ�����, ������ �����Ҽ��ִ�.
    if (bCheckFlag == true)
    {
        int64_t nValue = 0;
        // ī�޶� �Ķ���� �� �ҷ�����
        PvGenParameterArray* lDeviceParams = Device->GetParameters();
        printf("---------- Get Device Parameter ----------\n");
        // PixelFormat ���� command 
        
        result = lDeviceParams->SetEnumValue("PixelFormat", PvPixelMono8); // Default setting
        if (result.IsOK())
        {
            printf("Set PixelFormat = [Mono8]\n");
        }

        result = lDeviceParams->GetEnumValue("PixelFormat", nValue);
        if (PvPixelMono8 == nValue && result.IsOK())
        {
            printf("Get PixelFormat = [Mono8] [%d]\n", nValue);
        }
        printf("-------------------------------------------\n");

        printf("---------- Get Stream Parameter ----------\n");
        PvGenParameterArray* lStreamParams = Stream->GetParameters();

        double dFPSValue = 0.0;
        result = lStreamParams->GetFloatValue("AcquisitionRate", dFPSValue);
        if (result.IsOK())
        {
            printf("Get FPS = [%.2f]\n", dFPSValue);
        }
        printf("-------------------------------------------\n");

    }

    if (Device != nullptr)
    {
        result = Device->Disconnect();
        if (result.IsOK())
        {
            printf("Device Disconnect\n");
        }
        
        printf("-----------------------------------\n");
    }

    delete Pipeline;
    Pipeline = nullptr;
}
