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
    PvBuffer* lBuffer; // PvBuffer ��ü
    PvResult lOperationResult = -1; // �۾� ��� ���� �ʱ�ȭ
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
                    if (key == 27)  // 27�� ESC Ű �ڵ��Դϴ�.
                    {
                        break;      // ESC Ű�� ������ ������ �����մϴ�.
                    }
                }
                catch (cv::Exception& e)
                {
                    std::cerr << "Live Error: " << e.what() << std::endl;
                    // ���� ó�� �ڵ� �ۼ�
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
        printf("OpenStream = [success]\n");
    }
    // ī�޶� ��Ʈ�� ����
    ConfigureStream(Device, Stream);

    if (Stream != nullptr && Device != nullptr)
    {
        printf("ConfigureStream = [success]\n");
    }

    if (Stream != nullptr && Device != nullptr)
    {
        // ī�޶� ���������� ����
        Pipeline = CreatePipeline(Pipeline, Device, Stream);
        if (Pipeline != nullptr)
        {
            printf("CreatePipeline = [success]\n");
            // ������ ���� �������� �Ϸ�ȴٸ�, StreamingImage ���� �� ó�� �Ϸ�
            bFlag = true;
        }          
    }
    // ī�޶� �Ķ���� ����
    if (bFlag)
    {
        PvGenParameterArray* lDeviceParams = Device->GetParameters();
        // GenICam AcquisitionStart �� AcquisitionStop ��� 
        PvGenCommand* lStart = dynamic_cast<PvGenCommand*>(lDeviceParams->Get("AcquisitionStart"));

        // ��Ʈ���� ���� �غ�
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

