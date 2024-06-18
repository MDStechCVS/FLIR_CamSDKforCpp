#pragma once

#pragma warning(disable: 4800) // 비표준 경고 비활성화
#pragma warning(disable: 4244)

#include "resource.h"
#include "stdafx.h"
#include <windows.h>
#include <iomanip>
#include <list>
#include <set>
#include <memory>
#include <vector>
#include <thread>
#include <unordered_map>
#include <map>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <immintrin.h>
#include "mmsystem.h"
#include <sys/timeb.h>
#include <future>
#include <PvSampleUtils.h>
#include <PvDeviceFinderWnd.h>
#include <PvDevice.h>
#include <PvGenParameter.h>
#include <PvGenBrowserWnd.h>
#include <PvBuffer.h>
#include <PvStream.h>
#include <PvPipeline.h>
#include <PvDeviceGEV.h>
#include <PvGenStateStack.h>
#include <PvDeviceInfoGEV.h>
#include <PvStreamGEV.h>
#include <algorithm>
#include "Common.h"
#include "SkinButton.h"
#include "MDSColorpalette.h"
#include "CMMTiming.h"
#include "temperature.hpp"
#include "TransparentStatic.h"
#include "fff.h"
#include <condition_variable>

#define WM_ADDLOG (WM_APP + 1)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ASY_SAFEGUARD 1.0002
#define EXP_SAFEGUARD 709.78


/*----------메모리 누수 검사--------------*/
#include <cstdlib>
#define _CRTDBG_MAP_ALLOC
/*----------------------------------------*/

#define AUTO_START		0
#define REMOTE_DEBUG	0

#define DEFAULT_PAYLOAD_SIZE ( 1920 * 1080 * 2 )
#define WM_MY_MESSAGE (WM_APP + 1);


#define RED        RGB(127,  0,  0)
#define GREEN      RGB(  0,127,  0)
#define BLUE       RGB(  0,  0,127)
#define LIGHTRED   RGB(255,  0,  0)
#define LIGHTGREEN RGB(  51,255,  51)
#define LIGHTBLUE  RGB(  70,  163,210)
#define BLACK      RGB(  0,  0,  0)
#define WHITE      RGB(255,255,255)
#define GRAY       RGB(192,192,192)

#define RGB_BLACK			RGB(0,0,0)
#define RGB_RED				RGB(255,0,0)
#define RGB_BLUE			RGB(0,0,255)
#define RGB_GREEN			RGB(0,255,0)
#define RGB_WHITE			RGB(255,255,255)
#define RGBYELLOW			RGB(255,255,0)
#define RGB_SKYBLUE			RGB(0,128,255)
#define RGB_PURPLE			RGB(255,0,255)


#define SOURCE_SELECTOR ( "SourceSelector" )
#define DEVICE_TYPE_OLD ( "GevDeviceClass" )
#define DEVICE_TYPE_NEW ( "DeviceType" )
#define TRANSMITTER ( "Transmitter" )
#define PIXEL_FORMAT ( "PixelFormat" )
#define WIDTH ( "Width" )
#define HEIGHT ( "Height" )
#define PADDING_X ( "PaddingX" )
#define IMAGE_COMPRESSION_MODE ( "ImageCompressionMode" )
#define PAYLOAD_SIZE ( "PayloadSize" )
#define ACQUISITION_START ( "AcquisitionStart" )
#define ACQUISITION_STOP ( "AcquisitionStop" )
#define ACQUISITION_MODE ( "AcquisitionMode" )
#define DATA_LENGTH ( "DataLength" )
#define SOURCE_ID_VALUE_OLD ( "SourceStreamChannel" )
#define SOURCE_ID_VALUE_NEW ( "SourceIDValue" )
#define CHANNEL_SELECTOR ( "GevStreamChannelSelector" )
#define DEVICE_MODEL_NAME ( "DeviceModelName" )
#define SCCFG_PROTOCOL ( "GevSCCFGProtocol" )
#define SCCFG_TCPTRANSPORTENABLED ( "GevSCCFGTCPTransportEnabled" )
#define SCCFG_SAPENABLED ( "GevSCCFGSAPEnabled" )

#define BUFFER_COUNT			( 16 )

#define DOODLELENGTH			6
#define CAMERA_COUNT			4
#define	PALETTE_SIZE			256
#define	BUTTON_MAX_COUNT		100


#define MONO8 ( "0x1080001" )
#define MONO16 ( "0x1100007" )
#define MONO14 ( "0x1100025" )
#define YUV422_8_UYVY ( "0x210001F" )
#define RGB8PACK ( "0x2180014" )

#define FAHRENHEIT 273.15f

#define CAM_TYPE_A3XX 0
#define CAM_TYPE_A6XX 1
#define CAM_TYPE_ATAU 2
#define CAM_TYPE_T1K  3

#define PRES_SIGNAL     0
#define PRES_CELSIUS    1
#define PRES_FAHRENHEIT 2
#define PRES_KELVIN     3

#define QUERYCOUNT		12

#define CAM_1			0
#define CAM_2			1
#define CAM_3			2
#define CAM_4			3


const UINT_PTR TIMER_ID_GUI_UPDATE = 1000;
const UINT_PTR TIMER_ID_OTHER_TASK = 2000;

// 색상을 RGBA 형식으로 표현하기 위한 구조체
typedef struct RGBA 
{
	BYTE R;
	BYTE G;
	BYTE B;
	BYTE A;
} rgba;

const RGBA WhiteColor = { 255, 255, 255, 255 };
const RGBA YellowText = { 255, 255, 0, 0 };
const RGBA GrayBackground = { 255, 64, 64, 64 };
const RGBA WhiteText = { 255, 255, 255, 255 };

enum ERRORCODE
{
	OK,
	RESPONSE_FAIL,
	TIMEOUT,
};

enum class ThreadStatus
{
	THREAD_STOP = 0,
	THREAD_RUNNING = 1,
	THREAD_PAUSE = 2
};
using THREAD_STATUS = ThreadStatus;


enum class CameraModelList
{
	None	 = 0,
	A50,
	Ax5,
	A70,	
	A300,
	A400,
	A500,
	A615,
	A700,
	FT1000,
	XSC,
	BlackFly
};
using CAM_MODEL = CameraModelList;

enum class CameraIRFormat
{
	RADIOMETRIC = 0,
	TEMPERATURELINEAR100MK,
	TEMPERATURELINEAR10MK,
};
using Camera_IRFormat = CameraIRFormat;

typedef struct CAMERA_PARAMETER

{
	int index = -1;
	CString strPixelFormat = _T("");

	HANDLE param;
	
	int nCurrentCase[QUERYCOUNT] = { 0 };
	
	double dQueryCaseLowLimit[QUERYCOUNT] = { 0 };
	double dQueryCaseHighLimit[QUERYCOUNT] = { 0 };
	int nQueryCnt = 0;
	int nSelectCase = 0;

	int64_t nHeight = 0;
	int64_t nWidth = 0;

}Camera_Parameter;

typedef struct TauPlanckConstants
{
	int R;
	double B;
	double F;
	double O;
	double WT;
	double W4WT;
	ULONG J0; // Global offset
	double J1; // Global gain
	double K1;
	double K2;
}TPConstants;

typedef struct ObjectParams
{
	double Emissivity;
	double ObjectDistance;
	double AtmTemp;
	double AmbTemp;
	double AtmTao;
	double ExtOptTransm;
	double ExtOptTemp;
	double RelHum;
}ObjParams;

typedef struct SpectralResponseParams
{
	double X;
	double alpha1;
	double alpha2;
	double beta1;
	double beta2;
}stRParams;


enum class guistatus
{
	GUI_STEP_IDLE = 0,
	GUI_STEP_RUN,
	GUI_STEP_STOP,
	GUI_STEP_DISCONNECT,
	GUI_STEP_ERROR
};
using GUI_STATUS = guistatus;

// 최대 및 최소 스팟 값을 저장하기 위한 구조체
struct MDSMeasureMaxSpotValue
{
	int x; // x 좌표
	int y; // y 좌표
	int pointIdx; // 포인트 인덱스
	ushort tempValue; // 온도 값
	bool updated;

	MDSMeasureMaxSpotValue() : x(0), y(0), pointIdx(0), tempValue(std::numeric_limits<ushort>::min()), updated(false) {}
};

struct MDSMeasureMinSpotValue
{
	int x; // x 좌표
	int y; // y 좌표
	int pointIdx; // 포인트 인덱스
	ushort tempValue; // 온도 값
	bool updated;

	MDSMeasureMinSpotValue() : x(0), y(0), pointIdx(0), tempValue(std::numeric_limits<ushort>::max()), updated(false) {}
};

// ROI (관심 영역) 유형을 정의하는 열거형
enum class ShapeType
{
	None = -1,
	Rectangle = 0,
	Circle,
	Ellipse,
	Line
};

// 각 ROI의 결과를 저장하는 구조체
struct ROIResults
{
	cv::Rect roi; // ROI 영역
	int max_x, max_y, min_x, min_y; // 최대/최소 x, y 좌표
	ushort max_temp, min_temp; // 최대/최소 온도
	ShapeType shapeType; // 도형
	MDSMeasureMaxSpotValue maxSpot; // 최대 스팟 값
	MDSMeasureMinSpotValue minSpot; // 최소 스팟 값

	MDSMeasureMaxSpotValue lastMaxSpot; // 마지막으로 업데이트된 최대 스팟 위치
	MDSMeasureMinSpotValue lastMinSpot; // 마지막으로 업데이트된 최소 스팟 위치

	int span; // 스팬 값
	int level; // 레벨 값
	int nIndex;
	bool needsRedraw; // 다시 그릴 필요 여부
	cv::Scalar color;

	ROIResults() : max_x(0), max_y(0), min_x(0), min_y(0),
		max_temp(0), min_temp(65535),
		shapeType(ShapeType::None),
		span(0), level(0), nIndex(-1),
		needsRedraw(true),
		color(cv::Scalar(51, 255, 51)) {
		lastMaxSpot = maxSpot; // 초기화 시 현재 스팟 값으로 설정
		lastMinSpot = minSpot; // 초기화 시 현재 스팟 값으로 설정
	}
};

static double quicklog(double dbl_val)
{
#define sqr2div2 0.7071067811865476
#define ln2 0.6931471805599453
#define C1 2.88539129   
#define C3 0.961470632
#define C5 0.598978649

#ifdef _WIN32
	/* Little endian machine assumed */
#define DBL_2EXP_ADDR(dbl) ((unsigned short int*)&dbl+3)
#define TRUE_DBL_2EXP(dbl) (((*DBL_2EXP_ADDR(dbl)&0x7ff0)>>4)-0x3ff)
#define SET_DBL_2EXP(dbl) *DBL_2EXP_ADDR(dbl)&=~0x7ff0,\
                          *DBL_2EXP_ADDR(dbl)|= 0x3fe0
#else
	/* Big endian machine assumed */
#define DBL_2EXP_ADDR(dbl) ((unsigned long int*)&dbl)
#define TRUE_DBL_2EXP(dbl) (((*DBL_2EXP_ADDR(dbl)&0x7ff00000)>>20)-0x3ff)
#define SET_DBL_2EXP(dbl) *DBL_2EXP_ADDR(dbl)&=~0x7ff00000,\
                          *DBL_2EXP_ADDR(dbl)|= 0x3fe00000
#endif


	double Z, Z2;
	int times_2;

	times_2 = TRUE_DBL_2EXP(dbl_val) + 1;
	SET_DBL_2EXP(dbl_val); /* to -1 */

	Z = (dbl_val - sqr2div2) / (dbl_val + sqr2div2);
	Z2 = Z * Z;
	return (times_2 + (((C5 * Z2 + C3) * Z2 + C1) * Z - 0.5)) * ln2;

#undef sqr2div2
#undef ln2
#undef C1
#undef C3
#undef C5
#undef DBL_2EXP_ADDR
#undef TRUE_DBL_2EXP
#undef SET_DBL_2EXP
}


static BYTE bmiBuf[sizeof(BITMAPINFOHEADER) + 1024];
static const RGBQUAD GrayPalette[256] =
{
	{ 0,   0,   0,  0 },{ 1,   1,   1,  0 },{ 2,   2,   2,  0 },{ 3,   3,   3,  0 },{ 4,   4,   4,  0 },{ 5,   5,   5,  0 },{ 6,   6,   6,  0 },{ 7,   7,   7,  0 },{ 8,   8,   8,  0 },{ 9,   9,   9,  0 },
	{ 10,  10,  10,  0 },{ 11,  11,  11,  0 },{ 12,  12,  12,  0 },{ 13,  13,  13,  0 },{ 14,  14,  14,  0 },{ 15,  15,  15,  0 },{ 16,  16,  16,  0 },{ 17,  17,  17,  0 },{ 18,  18,  18,  0 },{ 19,  19,  19,  0 },
	{ 20,  20,  20,  0 },{ 21,  21,  21,  0 },{ 22,  22,  22,  0 },{ 23,  23,  23,  0 },{ 24,  24,  24,  0 },{ 25,  25,  25,  0 },{ 26,  26,  26,  0 },{ 27,  27,  27,  0 },{ 28,  28,  28,  0 },{ 29,  29,  29,  0 },
	{ 30,  30,  30,  0 },{ 31,  31,  31,  0 },{ 32,  32,  32,  0 },{ 33,  33,  33,  0 },{ 34,  34,  34,  0 },{ 35,  35,  35,  0 },{ 36,  36,  36,  0 },{ 37,  37,  37,  0 },{ 38,  38,  38,  0 },{ 39,  39,  39,  0 },
	{ 40,  40,  40,  0 },{ 41,  41,  41,  0 },{ 42,  42,  42,  0 },{ 43,  43,  43,  0 },{ 44,  44,  44,  0 },{ 45,  45,  45,  0 },{ 46,  46,  46,  0 },{ 47,  47,  47,  0 },{ 48,  48,  48,  0 },{ 49,  49,  49,  0 },
	{ 50,  50,  50,  0 },{ 51,  51,  51,  0 },{ 52,  52,  52,  0 },{ 53,  53,  53,  0 },{ 54,  54,  54,  0 },{ 55,  55,  55,  0 },{ 56,  56,  56,  0 },{ 57,  57,  57,  0 },{ 58,  58,  58,  0 },{ 59,  59,  59,  0 },
	{ 60,  60,  60,  0 },{ 61,  61,  61,  0 },{ 62,  62,  62,  0 },{ 63,  63,  63,  0 },{ 64,  64,  64,  0 },{ 65,  65,  65,  0 },{ 66,  66,  66,  0 },{ 67,  67,  67,  0 },{ 68,  68,  68,  0 },{ 69,  69,  69,  0 },
	{ 70,  70,  70,  0 },{ 71,  71,  71,  0 },{ 72,  72,  72,  0 },{ 73,  73,  73,  0 },{ 74,  74,  74,  0 },{ 75,  75,  75,  0 },{ 76,  76,  76,  0 },{ 77,  77,  77,  0 },{ 78,  78,  78,  0 },{ 79,  79,  79,  0 },
	{ 80,  80,  80,  0 },{ 81,  81,  81,  0 },{ 82,  82,  82,  0 },{ 83,  83,  83,  0 },{ 84,  84,  84,  0 },{ 85,  85,  85,  0 },{ 86,  86,  86,  0 },{ 87,  87,  87,  0 },{ 88,  88,  88,  0 },{ 89,  89,  89,  0 },
	{ 90,  90,  90,  0 },{ 91,  91,  91,  0 },{ 92,  92,  92,  0 },{ 93,  93,  93,  0 },{ 94,  94,  94,  0 },{ 95,  95,  95,  0 },{ 96,  96,  96,  0 },{ 97,  97,  97,  0 },{ 98,  98,  98,  0 },{ 99,  99,  99,  0 },
	{ 100, 100, 100,  0 },{ 101, 101, 101,  0 },{ 102, 102, 102,  0 },{ 103, 103, 103,  0 },{ 104, 104, 104,  0 },{ 105, 105, 105,  0 },{ 106, 106, 106,  0 },{ 107, 107, 107,  0 },{ 108, 108, 108,  0 },{ 109, 109, 109,  0 },
	{ 110, 110, 110,  0 },{ 111, 111, 111,  0 },{ 112, 112, 112,  0 },{ 113, 113, 113,  0 },{ 114, 114, 114,  0 },{ 115, 115, 115,  0 },{ 116, 116, 116,  0 },{ 117, 117, 117,  0 },{ 118, 118, 118,  0 },{ 119, 119, 119,  0 },
	{ 120, 120, 120,  0 },{ 121, 121, 121,  0 },{ 122, 122, 122,  0 },{ 123, 123, 123,  0 },{ 124, 124, 124,  0 },{ 125, 125, 125,  0 },{ 126, 126, 126,  0 },{ 127, 127, 127,  0 },{ 128, 128, 128,  0 },{ 129, 129, 129,  0 },
	{ 130, 130, 130,  0 },{ 131, 131, 131,  0 },{ 132, 132, 132,  0 },{ 133, 133, 133,  0 },{ 134, 134, 134,  0 },{ 135, 135, 135,  0 },{ 136, 136, 136,  0 },{ 137, 137, 137,  0 },{ 138, 138, 138,  0 },{ 139, 139, 139,  0 },
	{ 140, 140, 140,  0 },{ 141, 141, 141,  0 },{ 142, 142, 142,  0 },{ 143, 143, 143,  0 },{ 144, 144, 144,  0 },{ 145, 145, 145,  0 },{ 146, 146, 146,  0 },{ 147, 147, 147,  0 },{ 148, 148, 148,  0 },{ 149, 149, 149,  0 },
	{ 150, 150, 150,  0 },{ 151, 151, 151,  0 },{ 152, 152, 152,  0 },{ 153, 153, 153,  0 },{ 154, 154, 154,  0 },{ 155, 155, 155,  0 },{ 156, 156, 156,  0 },{ 157, 157, 157,  0 },{ 158, 158, 158,  0 },{ 159, 159, 159,  0 },
	{ 160, 160, 160,  0 },{ 161, 161, 161,  0 },{ 162, 162, 162,  0 },{ 163, 163, 163,  0 },{ 164, 164, 164,  0 },{ 165, 165, 165,  0 },{ 166, 166, 166,  0 },{ 167, 167, 167,  0 },{ 168, 168, 168,  0 },{ 169, 169, 169,  0 },
	{ 170, 170, 170,  0 },{ 171, 171, 171,  0 },{ 172, 172, 172,  0 },{ 173, 173, 173,  0 },{ 174, 174, 174,  0 },{ 175, 175, 175,  0 },{ 176, 176, 176,  0 },{ 177, 177, 177,  0 },{ 178, 178, 178,  0 },{ 179, 179, 179,  0 },
	{ 180, 180, 180,  0 },{ 181, 181, 181,  0 },{ 182, 182, 182,  0 },{ 183, 183, 183,  0 },{ 184, 184, 184,  0 },{ 185, 185, 185,  0 },{ 186, 186, 186,  0 },{ 187, 187, 187,  0 },{ 188, 188, 188,  0 },{ 189, 189, 189,  0 },
	{ 190, 190, 190,  0 },{ 191, 191, 191,  0 },{ 192, 192, 192,  0 },{ 193, 193, 193,  0 },{ 194, 194, 194,  0 },{ 195, 195, 195,  0 },{ 196, 196, 196,  0 },{ 197, 197, 197,  0 },{ 198, 198, 198,  0 },{ 199, 199, 199,  0 },
	{ 200, 200, 200,  0 },{ 201, 201, 201,  0 },{ 202, 202, 202,  0 },{ 203, 203, 203,  0 },{ 204, 204, 204,  0 },{ 205, 205, 205,  0 },{ 206, 206, 206,  0 },{ 207, 207, 207,  0 },{ 208, 208, 208,  0 },{ 209, 209, 209,  0 },
	{ 210, 210, 210,  0 },{ 211, 211, 211,  0 },{ 212, 212, 212,  0 },{ 213, 213, 213,  0 },{ 214, 214, 214,  0 },{ 215, 215, 215,  0 },{ 216, 216, 216,  0 },{ 217, 217, 217,  0 },{ 218, 218, 218,  0 },{ 219, 219, 219,  0 },
	{ 220, 220, 220,  0 },{ 221, 221, 221,  0 },{ 222, 222, 222,  0 },{ 223, 223, 223,  0 },{ 224, 224, 224,  0 },{ 225, 225, 225,  0 },{ 226, 226, 226,  0 },{ 227, 227, 227,  0 },{ 228, 228, 228,  0 },{ 229, 229, 229,  0 },
	{ 230, 230, 230,  0 },{ 231, 231, 231,  0 },{ 232, 232, 232,  0 },{ 233, 233, 233,  0 },{ 234, 234, 234,  0 },{ 235, 235, 235,  0 },{ 236, 236, 236,  0 },{ 237, 237, 237,  0 },{ 238, 238, 238,  0 },{ 239, 239, 239,  0 },
	{ 240, 240, 240,  0 },{ 241, 241, 241,  0 },{ 242, 242, 242,  0 },{ 243, 243, 243,  0 },{ 244, 244, 244,  0 },{ 245, 245, 245,  0 },{ 246, 246, 246,  0 },{ 247, 247, 247,  0 },{ 248, 248, 248,  0 },{ 249, 249, 249,  0 },
	{ 250, 250, 250,  0 },{ 251, 251, 251,  0 },{ 252, 252, 252,  0 },{ 253, 253, 253,  0 },{ 254, 254, 254,  0 },{ 255, 255, 255,  0 }
};

enum PIXELS_SUBTAG 
{
	LE
};

enum TAGID
{
	FREE = 0x00,
	Pixels,
	GainMap,
	OffsMap,
	DeadMap,
	GainDeadMap,
	CoarseMap,
	ImageMap,
	RRMap,
	OOMap,
	OPMap,
	BackgrMap,
	ShutterMap,
	MMap,
	YCbCrPixels,
	FloatOffsMap,
	DiffPixels,
	GasOverlay,
	CompressBurst,
	DistanceMap,
	BasicData = 0x20,
	Measure = 0x21,
	ColorPal = 0x22,
	TextComment = 0x23,
	VoiceComment = 0x24,
	MeasureResult = 0x25,
	VisualMarker = 0x26,
	IRMarker = 0x27,
	ImageOverlay = 0x28,
	Alarm = 0x29,
	Fusion,
	GPS,
	ExternalSensors = 0x2C
};
