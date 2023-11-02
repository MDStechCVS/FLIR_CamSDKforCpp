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

void StreamingImage(bool bFlag, PvPipeline* Pipeline, PvStream* stream)
{
    PvBuffer* lBuffer; // PvBuffer ��ü
    PvResult lOperationResult = -1; // �۾� ��� ���� �ʱ�ȭ
    PvImage* lImage = nullptr;
    PvGenParameterArray* lStreamParams = stream->GetParameters();
    double lFrameRateVal = 0.0; // ī�޶� ������ ����Ʈ ���� �����ϴ� ����
    unsigned char* ptr = nullptr;
    PvGenFloat* lFrameRate = dynamic_cast<PvGenFloat*>(lStreamParams->Get("AcquisitionRate"));
    uint32_t nTemp = 1; // ���� ��ȿ�� üũ
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
        else
        {
            // ���� ��Ȳ �ڵ� �м�
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
    // ī�޶� �Ķ���� ����
    if (bCheckFlag == true)
    {
        PvGenParameterArray* lDeviceParams = Device->GetParameters();
        // GenICam AcquisitionStart �� AcquisitionStop ��� 
        result = lDeviceParams->SetEnumValue("PixelFormat", PvPixelMono8); // Default setting

        PvGenCommand* lStart = dynamic_cast<PvGenCommand*>(lDeviceParams->Get("AcquisitionStart"));

        // ��Ʈ���� ���� �غ�
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

