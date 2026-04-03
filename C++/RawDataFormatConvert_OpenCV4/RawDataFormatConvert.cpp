/***************************************************************************************************
* 
* Ghi chú về cách cấu hình môi trường và dự án OpenCV.
* 1. Bạn có thể chuẩn bị gói cài đặt cần thiết từ trang web chính thức: https://opencv.org/releases.html
* 2. Nếu tệp *.lib không có trong gói tải về, bạn cần tự biên dịch bằng công cụ CMake.
* 3. Thêm đường dẫn thư mục 'bin' vào biến môi trường PATH.
* 4. Cấu hình 'Additional Include Directories', 'Additional Library Directories' và 'Additional Dependencies' trong thuộc tính dự án hiện tại.
* 
* Nếu có bất kỳ câu hỏi hoặc yêu cầu nào, xin vui lòng liên hệ với chúng tôi.
* 
***************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include "opencv2/core/core_c.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "MvCameraControl.h"

enum CONVERT_TYPE
{
    OpenCV_Mat         = 0,    // Định dạng ảnh Mat
    OpenCV_IplImage    = 1,    // Định dạng ảnh IplImage
};

// Hiển thị thông tin các thiết bị đã tìm thấy
void PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
    if (NULL == pstMVDevInfo)
    {
        printf("    NULL info.\n\n");
        return;
    }

    // Chỉ hỗ trợ thiết bị GigE và U3V để lấy khung dữ liệu hình ảnh
    if (MV_GIGE_DEVICE == pstMVDevInfo->nTLayerType)
    {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        // Hiển thị IP và tên thiết bị do người dùng đặt
        printf("    IP: %d.%d.%d.%d\n" , nIp1, nIp2, nIp3, nIp4);
        printf("    UserDefinedName: %s\n" , pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
		printf("    Device Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
    }
    else if (MV_USB_DEVICE == pstMVDevInfo->nTLayerType)
    {
        printf("    UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
		printf("    Device Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
    }
    else
    {
        printf("    Not support.\n\n");
    }
}

// Chuyển đổi sắp xếp pixel từ RGB sang BGR
void RGB2BGR( unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight )
{
    if ( NULL == pRgbData )
    {
        return;
    }

    // Hoán đổi dữ liệu đỏ (red) và xanh dương (blue)
    for (unsigned int j = 0; j < nHeight; j++)
    {
        for (unsigned int i = 0; i < nWidth; i++)
        {
            unsigned char red = pRgbData[j * (nWidth * 3) + i * 3];
            pRgbData[j * (nWidth * 3) + i * 3]     = pRgbData[j * (nWidth * 3) + i * 3 + 2];
            pRgbData[j * (nWidth * 3) + i * 3 + 2] = red;
        }
    }
}

// Chuyển đổi luồng dữ liệu sang định dạng Mat và lưu ảnh
bool Convert2Mat(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char * pData)
{
    if (NULL == pstImageInfo || NULL == pData)
    {
        printf("NULL info or data.\n");
        return false;
    }

    cv::Mat srcImage;

    if ( PixelType_Gvsp_Mono8 == pstImageInfo->enPixelType )                // Kiểu Mono8
    {
        srcImage = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC1, pData);
    }
    else if ( PixelType_Gvsp_RGB8_Packed == pstImageInfo->enPixelType )     // Kiểu RGB8
    {
        // Định dạng sắp xếp pixel của Mat là BGR, cần phải chuyển đổi
        RGB2BGR(pData, pstImageInfo->nWidth, pstImageInfo->nHeight);
        srcImage = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
    }
    else
    {
		/* Phương pháp chuyển đổi định dạng Bayer sang Mat:
		1. Trước khi hủy handle camera, gọi MV_CC_ConvertPixelType để chuyển đổi các định dạng Bayer như PixelType_Gvsp_BayerRG8 sang PixelType_Gvsp_BGR8_Packed 
		2. Tham khảo ở trên để chuyển đổi BGR sang định dạng Mat		
		*/

        printf("Unsupported pixel format\n");
        return false;
    }

    if ( NULL == srcImage.data )
    {
        printf("Creat Mat failed.\n");
        return false;
    }

    try 
    {
        // Lưu ảnh Mat vào tệp cục bộ
        cv::imwrite("Image_Mat.bmp", srcImage);
    }
    catch (cv::Exception& ex) 
    {
        fprintf(stderr, "Exception in saving mat image: %s\n", ex.what());
    }

    srcImage.release();

    return true;
}

// Chuyển đổi luồng dữ liệu sang định dạng Ipl và lưu ảnh
bool Convert2Ipl(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char * pData)
{
    if (NULL == pstImageInfo || NULL == pData)
    {
        printf("NULL info or data.\n");
        return false;
    }

    IplImage* srcImage = NULL;

    if ( PixelType_Gvsp_Mono8 == pstImageInfo->enPixelType )                // Kiểu Mono8
    {
        srcImage = cvCreateImage(cvSize(pstImageInfo->nWidth, pstImageInfo->nHeight), IPL_DEPTH_8U, 1);
    }
    else if ( PixelType_Gvsp_RGB8_Packed == pstImageInfo->enPixelType )     // Kiểu RGB8
    {
        // Định dạng sắp xếp pixel của IplImage là BGR, cần phải chuyển đổi
        RGB2BGR(pData, pstImageInfo->nWidth, pstImageInfo->nHeight);
        srcImage = cvCreateImage(cvSize(pstImageInfo->nWidth, pstImageInfo->nHeight), IPL_DEPTH_8U, 3);
    }
    else
    {
        printf("Unsupported pixel format\n");
        return false;
    }

    if ( NULL == srcImage )
    {
        printf("Creat IplImage failed.\n");
        return false;
    }

    srcImage->imageData = (char *)pData;

    try 
    {
        // Lưu ảnh IplImage vào tệp cục bộ
        cv::Mat cConvertImage = cv::cvarrToMat(srcImage);
        cv::imwrite("Image_Ipl.bmp", cConvertImage);

        cConvertImage.release();
    }
    catch (cv::Exception& ex) 
    {
        fprintf(stderr, "Exception in saving IplImage: %s\n", ex.what());
    }

    cvReleaseImage(&srcImage);

    return true;
}

int main()
{
    int nRet = MV_OK;
    void* handle = NULL;
    unsigned char * pData = NULL;

    do
    {
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

        // Liệt kê thiết bị
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet)
        {
            printf("Enum Devices fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Hiển thị thông tin thiết bị
        if (stDeviceList.nDeviceNum > 0)
        {
            for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
            {
                printf("[device %d]:\n", i);
                MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                }
                PrintDeviceInfo(pDeviceInfo);
            }
        }
        else
        {
            printf("Find No Devices!\n");
            break;
        }

        // Chọn thiết bị
        unsigned int nIndex = 0;
        while (1)
        {
            printf("Please Input camera index(0-%d): ", stDeviceList.nDeviceNum - 1);

            if (1 == scanf_s("%d", &nIndex))
            {
                while (getchar() != '\n')
                {
                    ;
                }

                // Nhập liệu hợp lệ
                if (nIndex >= 0 && nIndex < stDeviceList.nDeviceNum)
                {
                    // Thiết bị không thể truy cập, vui lòng nhập lại
                    if (false == MV_CC_IsDeviceAccessible(stDeviceList.pDeviceInfo[nIndex], MV_ACCESS_Exclusive))
                    {
                        printf("Can't connect! ");
                        continue;
                    }

                    break;
                }
            }
            else
            {
                while (getchar() != '\n')
                {
                    ;
                }
            }
        }

        // Tạo handle điều khiển thiết bị
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
        if (MV_OK != nRet)
        {
            printf("Create Handle fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Mở thiết bị
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("Open Device fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Dò tìm kích thước gói tin (packet) tối ưu (chỉ dành cho camera GigE)
        if (MV_GIGE_DEVICE == stDeviceList.pDeviceInfo[nIndex]->nTLayerType)
        {
            int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
            if (nPacketSize > 0)
            {
                // Cài đặt kích thước gói tin
                nRet = MV_CC_SetIntValue(handle, "GevSCPSPacketSize", nPacketSize);
                if (MV_OK != nRet)
                {
                    printf("Warning: Set Packet Size fail! nRet [0x%x]!", nRet);
                }
            }
            else
            {
                printf("Warning: Get Packet Size fail! nRet [0x%x]!", nPacketSize);
            }
        }

        // Tắt chế độ kích hoạt (trigger mode)
        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
        if (MV_OK != nRet)
        {
            printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Lấy kích thước dữ liệu (payload size)
        MVCC_INTVALUE stParam;
        memset(&stParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
        if (MV_OK != nRet)
        {
            printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
            break;
        }
        unsigned int nPayloadSize = stParam.nCurValue;

        // Khởi tạo thông tin hình ảnh
        MV_FRAME_OUT_INFO_EX stImageInfo = { 0 };
        memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
        pData = (unsigned char *)malloc(sizeof(unsigned char)* (nPayloadSize));
        if (NULL == pData)
        {
            printf("Allocate memory failed.\n");
            break;
        }
        memset(pData, 0, sizeof(pData));

        // Bắt đầu thu thập hình ảnh
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Lấy một khung hình từ camera với thời gian chờ 1000ms
        nRet = MV_CC_GetOneFrameTimeout(handle, pData, nPayloadSize, &stImageInfo, 1000);
        if (MV_OK == nRet)
        {
            printf("Get One Frame: Width[%d], Height[%d], FrameNum[%d]\n",
                stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);
        }
        else
        {
            printf("Get Frame fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Dừng thu thập hình ảnh
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Đóng thiết bị
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("ClosDevice fail! nRet [0x%x]\n", nRet);
            break;
        }

        // Nhập định dạng muốn chuyển đổi
        printf("\n[0] OpenCV_Mat\n");
        printf("[1] OpenCV_IplImage\n");
        int nFormat = 0;
        while (1)
        {
            printf("Please Input Format to convert: ");

            if (1 == scanf_s("%d", &nFormat))
            {
                // Nhập liệu hợp lệ
                if (0 == nFormat || 1 == nFormat)
                {
                    break;
                }
            }
            while (getchar() != '\n')
            {
                ;
            }
        }

        // Chuyển đổi dữ liệu hình ảnh
        bool bConvertRet = false;
        if (OpenCV_Mat == nFormat)
        {
            bConvertRet = Convert2Mat(&stImageInfo, pData);
        }
        else if (OpenCV_IplImage == nFormat)
        {
            bConvertRet = Convert2Ipl(&stImageInfo, pData);
        }

        // Hiển thị kết quả chuyển đổi
        if (bConvertRet)
        {
            printf("OpenCV format convert finished.\n");
        }
        else
        {
            printf("OpenCV format convert failed.\n");
        }
    } while (0);

    // Hủy bỏ handle
    if (handle)
    {
        MV_CC_DestroyHandle(handle);
        handle = NULL;
    }

    // Giải phóng bộ nhớ
    if (pData)
    {
        free(pData);
        pData = NULL;
    }

    system("pause");
    return 0;
}
