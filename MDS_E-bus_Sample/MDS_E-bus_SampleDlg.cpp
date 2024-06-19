
// MDS_Ebus_SampleDlg.cpp: 구현 파일
//
#include "stdafx.h"
#include "MDS_E-bus_Sample.h"
#include "MDS_E-bus_SampleDlg.h"
#include "afxdialogex.h"

#include "CameraManager.h"
#include "CameraControl_rev.h"
#include "ImageProcessor.h"

JudgeStatusDlg* Judgedlg;
PDH_HQUERY hQuery = NULL;
PDH_HCOUNTER hCounterTotal = NULL;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// END
////////////////////////////////////

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIMEenum { IDD = IDD_ABOUTBOX };
//#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
    ON_WM_DRAWITEM()
END_MESSAGE_MAP()


// CMDS_Ebus_SampleDlg 대화 상자
// =============================================================================
// 라벨 변수초기화 추가
CMDS_Ebus_SampleDlg::CMDS_Ebus_SampleDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MDS_MAIN_DLG, pParent)
    , m_NeedInit(TRUE)
    , m_DeviceWnd(NULL)
    , m_CommunicationWnd(NULL)
    , m_StreamParametersWnd(NULL)
    , m_LbCamInfo{
        { m_LbCam1fps, m_LbCam1min, m_LbCam1max, m_LbCam1ROI, m_LbCam1ConnectStatus, m_LbCam1RecordingStatus },
        { m_LbCam2fps, m_LbCam2min, m_LbCam2max, m_LbCam2ROI, m_LbCam2ConnectStatus, m_LbCam2RecordingStatus },
        { m_LbCam3fps, m_LbCam3min, m_LbCam3max, m_LbCam3ROI, m_LbCam3ConnectStatus, m_LbCam3RecordingStatus },
        { m_LbCam4fps, m_LbCam4min, m_LbCam4max, m_LbCam4ROI, m_LbCam4ConnectStatus, m_LbCam4RecordingStatus }
}

{

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_LOG, m_List_Log);
    DDX_Control(pDX, IDC_BTN_DEVICE, m_BtnDeviceCtrl);
    DDX_Control(pDX, IDC_BTN_COMMUNICATION, m_BtnCommunicationCtrl);
    DDX_Control(pDX, IDC_BTN_IMG_STREAM, m_BtnImageStreamCtrl);
    DDX_Control(pDX, IDC_BTN_START, m_BtnStart);
    DDX_Control(pDX, IDC_BTN_STOP, m_BtnStop);
    DDX_Control(pDX, IDC_BTN_DISCONNECT, m_BtnDisconnect);
    DDX_Control(pDX, IDC_BTN_FPS30, m_BtnFPS30);
    DDX_Control(pDX, IDC_BTN_AF, m_BtnAF);
    DDX_Control(pDX, IDC_BTN_DEVICEFIND, m_BtnDeviceFind);
    DDX_Control(pDX, IDC_BTN_LOAD_INI_FILE, m_BtnLoadiniFile);
    DDX_Control(pDX, IDC_BTN_INI_APPLY, m_BtniniApply);
    DDX_Control(pDX, IDC_BTN_CAM_PARAM, m_BtnCamParam);
    DDX_Control(pDX, IDC_BTN_CAM_PARAM_APPLY, m_BtnCamParamApply);
    DDX_Control(pDX, IDC_BTN_OPEN_DATA_FOLDER, m_BtnDataFolder);
    DDX_Control(pDX, IDC_BTN_IMG_SNAP, m_BtnImageSnap);
    DDX_Control(pDX, IDC_BTN_IMG_RECORDING, m_BtnImageRecording);
    DDX_Control(pDX, IDC_CK_PARAM, m_chGenICam_checkBox);
    DDX_Control(pDX, IDC_CK_MONO, m_chMonoCheckBox);
    DDX_Control(pDX, IDC_CK_COLORMAP, m_chColorMapCheckBox);
    DDX_Control(pDX, IDC_CK_UYVY, m_chUYVYCheckBox);
    DDX_Control(pDX, IDC_CK_MESSAGE, m_chEventsCheckBox);
    DDX_Control(pDX, IDC_CK_POINTER, m_chPointerCheckBox);
    DDX_Control(pDX, IDC_CK_MARKER, m_chMarkerCheckBox);
    DDX_Control(pDX, IDC_CK_CAM1_ROI, m_ch_Cam1_ROI_CheckBox);
    DDX_Control(pDX, IDC_CK_CAM1_ROI2, m_ch_Cam1_ROI_Create);
    DDX_Control(pDX, IDC_STATIC_CAMCOUNT, m_LbCamCount);
    DDX_Control(pDX, IDC_STATIC_CAM1_FPS, m_LbCam1fps);
    DDX_Control(pDX, IDC_STATIC_CAM1_Max, m_LbCam1max);
    DDX_Control(pDX, IDC_STATIC_CAM1_Min, m_LbCam1min);
    DDX_Control(pDX, IDC_STATIC_CAM2_FPS, m_LbCam2fps);
    DDX_Control(pDX, IDC_STATIC_CAM2_Max, m_LbCam2max);
    DDX_Control(pDX, IDC_STATIC_CAM2_Min, m_LbCam2min);
    DDX_Control(pDX, IDC_STATIC_CAM3_FPS, m_LbCam3fps);
    DDX_Control(pDX, IDC_STATIC_CAM3_Max, m_LbCam3max);
    DDX_Control(pDX, IDC_STATIC_CAM3_Min, m_LbCam3min);
    DDX_Control(pDX, IDC_STATIC_CAM4_FPS, m_LbCam4fps);
    DDX_Control(pDX, IDC_STATIC_CAM4_Max, m_LbCam4max);
    DDX_Control(pDX, IDC_STATIC_CAM4_Min, m_LbCam4min);
    DDX_Control(pDX, IDC_STATIC_CAM1_ROI, m_LbCam1ROI);
    DDX_Control(pDX, IDC_STATIC_CAM2_ROI, m_LbCam2ROI);
    DDX_Control(pDX, IDC_STATIC_CAM3_ROI, m_LbCam3ROI);
    DDX_Control(pDX, IDC_STATIC_CAM4_ROI, m_LbCam4ROI);
    DDX_Control(pDX, IDC_CAM1_IS_CONNECT, m_LbCam1ConnectStatus);
    DDX_Control(pDX, IDC_CAM2_IS_CONNECT, m_LbCam2ConnectStatus);
    DDX_Control(pDX, IDC_CAM3_IS_CONNECT, m_LbCam3ConnectStatus);
    DDX_Control(pDX, IDC_CAM4_IS_CONNECT, m_LbCam4ConnectStatus);
    DDX_Control(pDX, IDC_CAM1_RECORDING, m_LbCam1RecordingStatus);
    DDX_Control(pDX, IDC_CAM2_RECORDING, m_LbCam2RecordingStatus);
    DDX_Control(pDX, IDC_CAM3_RECORDING, m_LbCam3RecordingStatus);
    DDX_Control(pDX, IDC_CAM4_RECORDING, m_LbCam4RecordingStatus);
    DDX_Control(pDX, IDC_LOGO, m_logo);
    DDX_Control(pDX, IDC_PROGRESS1, m_progress);
    DDX_Control(pDX, IDC_RADIO_CAM1, m_radio);
    DDX_Control(pDX, IDC_RADIO_RECT, m_radioshape);
    DDX_Control(pDX, IDC_CAM1, m_Cam1);
    DDX_Control(pDX, IDC_CAM2, m_Cam2);
    DDX_Control(pDX, IDC_CAM3, m_Cam3);
    DDX_Control(pDX, IDC_CAM4, m_Cam4);
    DDX_Control(pDX, IDC_CB_CAM1_COLORMAP, m_Cam1_Colormap);
    DDX_Control(pDX, IDC_CB_CAM2_COLORMAP, m_Cam2_Colormap);
    DDX_Control(pDX, IDC_CB_CAM3_COLORMAP, m_Cam3_Colormap);
    DDX_Control(pDX, IDC_CB_CAM4_COLORMAP, m_Cam4_Colormap);
    DDX_Control(pDX, IDC_CB_IR_FORMAT, m_cbIRFormat);
    DDX_Control(pDX, IDC_CB_TEMP_RANGE, m_cbTempRange);
    DDX_Control(pDX, IDC_EDIT_SCALE, m_Color_Scale);
    DDX_Control(pDX, IDC_EDIT_SCALE2, m_Color_Scale2);
    DDX_Control(pDX, IDC_EDIT_SCALE3, m_Color_Scale3);
    DDX_Control(pDX, IDC_EDIT_OPTTRANSM, m_edExtOptTemp);
}

// =============================================================================
BEGIN_MESSAGE_MAP(CMDS_Ebus_SampleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
    ON_MESSAGE(WM_ADDLOG, &CMDS_Ebus_SampleDlg::OnAddLog)
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_START, &CMDS_Ebus_SampleDlg::OnBnClickedBtnStart)
    ON_BN_CLICKED(IDC_BTN_STOP, &CMDS_Ebus_SampleDlg::OnBnClickedBtnStop)
    ON_BN_CLICKED(IDC_BTN_COMMUNICATION, &CMDS_Ebus_SampleDlg::OnBnClickedBtnCommunicationCtrl)
    ON_BN_CLICKED(IDC_BTN_DEVICE, &CMDS_Ebus_SampleDlg::OnBnClickedDeviceCtrl)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CMDS_Ebus_SampleDlg::OnBnClickedBtnDisconnect)
    ON_BN_CLICKED(IDC_BTN_IMG_STREAM, &CMDS_Ebus_SampleDlg::OnBnClickedBtnImgStream)
    ON_WM_MOVE()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    ON_WM_DESTROY()
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_AF, &CMDS_Ebus_SampleDlg::OnBnClickedBtnAF)
    ON_BN_CLICKED(IDC_BTN_FPS30, &CMDS_Ebus_SampleDlg::OnBnClickedBtnFps30)
    ON_BN_CLICKED(IDC_CK_PARAM, &CMDS_Ebus_SampleDlg::OnBnClickedCkParam)
    ON_BN_CLICKED(IDC_CK_POINTER, &CMDS_Ebus_SampleDlg::OnBnClickedCkPointer)
    ON_BN_CLICKED(IDC_CK_MARKER, &CMDS_Ebus_SampleDlg::OnBnClickedCkMarker)
    ON_BN_CLICKED(IDC_BTN_DEVICEFIND, &CMDS_Ebus_SampleDlg::OnBnClickedBtnDeviceFind)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO_CAM1, IDC_RADIO_CAM4, &CMDS_Ebus_SampleDlg::RadioCtrl)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO_RECT, IDC_RADIO_LINE, &CMDS_Ebus_SampleDlg::ShapeTypeRadioCtrl)
    ON_BN_CLICKED(IDC_BTN_LOAD_INI_FILE, &CMDS_Ebus_SampleDlg::OnBnClickedBtnLoadIniFile)
    ON_BN_CLICKED(IDC_BTN_INI_APPLY, &CMDS_Ebus_SampleDlg::OnBnClickedBtnIniApply)
    ON_CBN_SELCHANGE(IDC_CB_CAM1_COLORMAP, &CMDS_Ebus_SampleDlg::OnCbnSelchangeCam1)
    ON_CBN_SELCHANGE(IDC_CB_CAM2_COLORMAP, &CMDS_Ebus_SampleDlg::OnCbnSelchangeCam2)
    ON_CBN_SELCHANGE(IDC_CB_CAM3_COLORMAP, &CMDS_Ebus_SampleDlg::OnCbnSelchangeCam3)
    ON_CBN_SELCHANGE(IDC_CB_CAM4_COLORMAP, &CMDS_Ebus_SampleDlg::OnCbnSelchangeCam4)
    ON_BN_CLICKED(IDC_BTN_CAM_PARAM, &CMDS_Ebus_SampleDlg::OnBnClickedBtnCamParam)
    ON_BN_CLICKED(IDC_BTN_CAM_PARAM_APPLY, &CMDS_Ebus_SampleDlg::OnBnClickedBtnCamParamApply)
    ON_BN_CLICKED(IDC_CK_UYVY, &CMDS_Ebus_SampleDlg::OnBnClickedCheckBox)
    ON_BN_CLICKED(IDC_CK_COLORMAP, &CMDS_Ebus_SampleDlg::OnBnClickedCheckBox)
    ON_BN_CLICKED(IDC_CK_MESSAGE, &CMDS_Ebus_SampleDlg::OnBnClickedCheckBox)
    ON_BN_CLICKED(IDC_CK_MONO, &CMDS_Ebus_SampleDlg::OnBnClickedCheckBox)
    ON_BN_CLICKED(IDC_BTN_OPEN_DATA_FOLDER, &CMDS_Ebus_SampleDlg::OnBnClickedBtnOpenDataFolder)
    ON_CONTROL(STN_CLICKED, IDC_CAM1_RECORDING, &CMDS_Ebus_SampleDlg::OnStnClickedCam1Recording)
    ON_CONTROL(STN_CLICKED, IDC_CAM2_RECORDING, &CMDS_Ebus_SampleDlg::OnStnClickedCam1Recording)
    ON_CONTROL(STN_CLICKED, IDC_CAM3_RECORDING, &CMDS_Ebus_SampleDlg::OnStnClickedCam1Recording)
    ON_CONTROL(STN_CLICKED, IDC_CAM4_RECORDING, &CMDS_Ebus_SampleDlg::OnStnClickedCam1Recording)
    ON_BN_CLICKED(IDC_BTN_IMG_SNAP, &CMDS_Ebus_SampleDlg::OnBnClickedBtnImgSnap)
    ON_BN_CLICKED(IDC_BTN_IMG_RECORDING, &CMDS_Ebus_SampleDlg::OnBnClickedBtnImgRecording)
    ON_WM_DRAWITEM()
    ON_CBN_DROPDOWN(IDC_CB_CAM1_COLORMAP, &CMDS_Ebus_SampleDlg::OnCbnDropdownCbCam1Colormap)
    ON_BN_CLICKED(IDC_BTN_PARAM_SET, &CMDS_Ebus_SampleDlg::OnBnClickedBtnParamSet)
    ON_CBN_SELCHANGE(IDC_CB_IR_FORMAT, &CMDS_Ebus_SampleDlg::OnCbnSelchangeCbIrFormat)
    ON_CBN_SELCHANGE(IDC_CB_TEMP_RANGE, &CMDS_Ebus_SampleDlg::OnCbnSelchangeCbTempRange)
    ON_BN_CLICKED(IDC_CK_CAM1_ROI, &CMDS_Ebus_SampleDlg::OnBnClickedCkCam1Roi)
    ON_COMMAND(ID_POPUP_MENUITEM1, &CMDS_Ebus_SampleDlg::OnPopupMenuSave)
    ON_COMMAND(ID_POPUP_MENUITEM2, &CMDS_Ebus_SampleDlg::OnPopupMenuDelete)
    ON_COMMAND(ID_POPUP_MENUITEM3, &CMDS_Ebus_SampleDlg::OnPopupMenuAllDelete)

END_MESSAGE_MAP()


// CMDS_Ebus_SampleDlg 메시지 처리기
// =============================================================================
BOOL CMDS_Ebus_SampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
    
    //시스템 파라미터 설정
    InitSystemParam();
    //카메라 메니저 클래스 생성
    m_CamManager = new CameraManager();

    m_CamManager->InitData();
    // 파라미터 파일 로딩
    LoadiniFile();

    // 연결되어있는 카메라 검색
    m_CamManager->CameraDeviceFind(this);

    // 이니셜 대기 창 활성화
    ShowJudgeDlg();

    gui_status = GUI_STATUS::GUI_STEP_IDLE;

    m_radio.SetCheck(true);
    m_radioshape.SetCheck(true);
    m_ch_Cam1_ROI_CheckBox.SetCheck(true);
    SetTransparentStatic_ControlsFont();

    // GUI타이머 시작
    SetTimer(TIMER_ID_GUI_UPDATE, 1000, NULL);

    //gui 타이머를 감시하는 타이머로 설정
    SetTimer(TIMER_ID_OTHER_TASK, 5000, NULL);


    InitializePdh();
    //CPU 사용량
    SetTimer(TIMER_ID_CPU_USAGE, 1000, NULL);
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CMDS_Ebus_SampleDlg::InitSystemParam()
{
    //combo box 초기 컬러맵 데이터 설정
    PopulateComboBoxes();

    HBITMAP hbit;
    hbit = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_LOGO));
    //m_logo.ScreenToClient;
    m_logo.SetBitmap(hbit);

    m_brush = new CBrush(WHITE);
    m_brush2 = new CBrush(BLACK);

    ShowWindow(SW_SHOWMAXIMIZED);
    Common::GetInstance()->CreateLog(&m_List_Log);

    m_nSelectCamIndex = 0;
    m_nSelectedRoiIndex = -1;
    m_isRoiModeEnabled = false;
    m_isDraggingROI = false;
    m_selectedROIIndex = -1;

    m_Color1 = RGB_RED;
    m_Color2 = RGB_GREEN;
    m_Color3 = RGBYELLOW;
    m_bRed.CreateSolidBrush(m_Color1);
    m_bGreen.CreateSolidBrush(m_Color2);
    m_bYellow.CreateSolidBrush(m_Color3);

    SetTransparentStatic_ControlsFont();
    Btn_Interface_setting();

    RadioCtrl(IDC_RADIO_CAM1);

    ShapeTypeRadioCtrl(IDC_RADIO_RECT);
    // GUI 타이머 동작 상애 체크변수 초기화
    m_bGUITimerActive = false;
}

bool CMDS_Ebus_SampleDlg::SetBtnEnabled(bool bFlag, CSkinButton* Btn)
{
    if (bFlag)  // Enabled true
    {
        Btn->EnableWindow(TRUE);
        Btn->SetColorBackground(COLOR_HOVER);
    }
    else  // Enabled false
    {
        Btn->EnableWindow(FALSE);
        Btn->SetColorBackground(COLOR_DEFAULT);
    }
    return TRUE;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::Interface_state(bool bSeleted)
{
    //SetBtnEnabled(FALSE, &m_BtnDisconnect);
    SetBtnEnabled(bSeleted, &m_BtnDeviceCtrl);
    SetBtnEnabled(bSeleted, &m_BtnCommunicationCtrl);
    SetBtnEnabled(bSeleted, &m_BtnImageStreamCtrl);
    SetBtnEnabled(bSeleted, &m_BtnStart);
    SetBtnEnabled(!bSeleted, &m_BtnStop);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

// =============================================================================
void CMDS_Ebus_SampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{

		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CMDS_Ebus_SampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnStart()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    CString strLog = _T("");

    if (m_CamManager != nullptr)
    {
        int nIndex = GetSelectCamIndex();
        if (m_CamManager->GetDeviceCount() > 0)
        {
            if (m_CamManager->m_Cam[nIndex]->GetCamRunningFlag() == FALSE)
            {
                m_CamManager->m_Cam[nIndex]->CameraStart(nIndex);
                strLog.Format(_T("Camera[%d] Streaming Start"), nIndex + 1);
                Common::GetInstance()->AddLog(0, strLog);
                this->EnableWindow(TRUE);
                this->SetFocus();

                UpdateWindow();
            }
        }

    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnStop()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    CString strLog = _T("");
    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() > 0)
    {
        if (m_CamManager->m_Cam[nIndex] != nullptr)
        {
            if (m_CamManager->m_Cam[nIndex]->GetCamRunningFlag() == TRUE)
            {
                m_CamManager->m_Cam[nIndex]->CameraStop(nIndex);

                strLog.Format(_T("Camera[%d] Streaming Stop"), nIndex + 1);
                Common::GetInstance()->AddLog(0, strLog);

                this->EnableWindow(TRUE);
                this->SetFocus();

                UpdateWindow();
            }
        }
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnDestroy()
{
    CDialogEx::OnDestroy();

    // TODO: 여기에 메시지 처리기 코드를 추가합니다.

    KillTimer(500);

    m_CamManager->ManagerDestroy();

    if (m_CamManager != nullptr)
    {
        delete m_CamManager;
        m_CamManager = nullptr;
    }
   
    if (m_brush != nullptr)
    {
        delete m_brush;
        m_brush = nullptr;
    }

    if (m_brush2 != nullptr)
    {
        delete m_brush2;
        m_brush2 = nullptr;
    }
    m_bRed.DeleteObject();
    m_bGreen.DeleteObject();
    m_bYellow.DeleteObject();

    if (Judgedlg != nullptr)
    {
        delete Judgedlg;
        Judgedlg = nullptr;
    }
    
    Common::GetInstance()->AddLog(1, _T("Program Destroy"));

    _CrtDumpMemoryLeaks();

}

// TEST=========================================================================
void CMDS_Ebus_SampleDlg::ShowGenWindow(PvGenBrowserWnd** aWnd, PvGenParameterArray* aParams, const CString& aTitle)
{
    if ((*aWnd) != NULL)
    {
        if ((*aWnd)->GetHandle() != 0)
        {
            CWnd lWnd;
            lWnd.Attach((*aWnd)->GetHandle());

            // Window already visible, give it focus and bring it on top
            lWnd.BringWindowToTop();
            lWnd.SetFocus();

            lWnd.Detach();
            return;
        }

        // Window object exists but was closed/destroyed. Free it before re-creating
        CloseGenWindow(aWnd);
    }

    // Create, assign parameters, set title and show modeless
    (*aWnd) = new PvGenBrowserWnd;
    (*aWnd)->SetTitle(PvString(aTitle));
    (*aWnd)->SetGenParameterArray(aParams);
    (*aWnd)->ShowModeless(GetSafeHwnd());
}

// =============================================================================
void CMDS_Ebus_SampleDlg::CloseGenWindow(PvGenBrowserWnd** aWnd)
{
    // If the window object does not even exist, do nothing
    if ((*aWnd) == NULL)
    {
        return;
    }

    // If the window object exists and is currently created (visible), close/destroy it
    if ((*aWnd)->GetHandle() != 0)
    {
        (*aWnd)->Close();
    }

    // Finally, release the window object
    delete (*aWnd);
    (*aWnd) = NULL;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnCommunicationCtrl()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() <= 0)
        return;

    if (m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->IsConnected())
    {
        ShowGenWindow(
            &m_CommunicationWnd,
            m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->GetCommunicationParameters(),
            _T("Communication Control"));

        this->EnableWindow(TRUE);
        this->SetFocus();

        UpdateWindow();
    }
}

// =============================================================================
BOOL CMDS_Ebus_SampleDlg::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

    //if (!CMDS_Ebus_SampleDlg::PreCreateWindow(cs))
    //    return FALSE;

    //cs.style = WS_POPUP;

    return TRUE;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedDeviceCtrl()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    int nIndex = m_CamManager->GetDeviceCount();
    if (nIndex <= 0)
        return;

    if (m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->IsConnected())
    {
        ShowGenWindow(
            &m_DeviceWnd,
            m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->GetParameters(),
            _T("Device Control"));

        this->EnableWindow(TRUE);
        this->SetFocus();

        UpdateWindow();
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnImgStream()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.


    if (m_CamManager->GetDeviceCount() <= 0)
        return;

    if (m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->IsConnected())
    {
        ShowGenWindow(
            &m_StreamParametersWnd,
            m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->GetParameters(),
            _T("Image Stream Control"));

        this->EnableWindow(TRUE);
        this->SetFocus();

        UpdateWindow();
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnMove(int x, int y)
{
    CDialogEx::OnMove(x, y);

    // TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnDisconnect()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    CString strLog = _T("");
    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() > 0)
    {
        if (m_CamManager->m_Cam[nIndex] != nullptr)
        {
            m_CamManager->m_Cam[nIndex]->SetStartFlag(false);
            m_CamManager->m_Cam[nIndex]->SetThreadFlag(false);
            m_CamManager->m_Cam[nIndex]->SetCamRunningFlag(false);

            strLog.Format(_T("[Camera[%d]] Thread Stop"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);

            Common::GetInstance()->AddLog(0, _T("------------------------------------"));

            m_CamManager->m_Cam[nIndex]->CameraStop(nIndex);
            m_CamManager->m_Cam[nIndex]->CameraDisconnect();

            strLog.Format(_T("[Camera[%d]],CameraStop & Disconnect"), nIndex + 1);
            Common::GetInstance()->AddLog(0, strLog);
            Common::GetInstance()->AddLog(0, _T("------------------------------------"));

            this->EnableWindow(TRUE);
            this->SetFocus();

            UpdateWindow();
        }
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

    CDialogEx::OnTimer(nIDEvent);

    if (nIDEvent == TIMER_ID_OTHER_TASK)
    {
        if (m_bGUITimerActive)
        {
            // 1번 타이머가 동작 중인 경우
            // 필요한 작업 수행
        }
        else
        {
            SetTimer(TIMER_ID_GUI_UPDATE, 500, NULL);
        }
    }
    else if (nIDEvent == TIMER_ID_GUI_UPDATE)
    {
        try
        {
            UpdateCameraSystems();
            UpdateWindow();


            if (gui_status == GUI_STATUS::GUI_STEP_RUN)
            {
                CloseJudgeDlg();
            }

        }
        catch (const std::exception& e)
        {
            LogError("Error in camera update: ", e.what());
        }
    }
    else if (nIDEvent == TIMER_ID_CPU_USAGE)
    {
        UpdateProgressValue(); // CPU 사용률 업데이트
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::UpdateCameraSystems()
{
    if (m_CamManager == nullptr) return;
    int nDeviceCnt = m_CamManager->GetDeviceCount();
    CameraAutoStart(nDeviceCnt);
    for (int i = 0; i < nDeviceCnt; i++)
    {
        if (m_CamManager->m_Cam[i]->GetCamRunningFlag())
        {
            m_CamManager->m_Cam[i]->UpdateCalcParams();
            m_CamManager->m_Cam[i]->UpdateDeviceOP();
            UpdateCameraInfo(m_CamManager->m_Cam[i], m_LbCamInfo[i]);
            InvalidateLabels(m_LbCamInfo[i]);
        }
    }
    if (m_cbTempRange.GetCount() < 1)
        PopulateComboBoxes_Range();
}

// =============================================================================
void CMDS_Ebus_SampleDlg::LogError(const std::string& message, const char* error)
{
    CString strError;
    strError.Format(_T("%s %s"), message.c_str(), error);
    Common::GetInstance()->AddLog(0, strError);
}

// =============================================================================
BOOL CMDS_Ebus_SampleDlg::OnEraseBkgnd(CDC* pDC)
{
    // TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

    return CDialogEx::OnEraseBkgnd(pDC);
}

// 카메라의 인덱스와 연결 여부에 따라 배경 색상과 브러시를 설정하고 반환하는 함수
// =============================================================================
HBRUSH CMDS_Ebus_SampleDlg::SetCameraFlagStatus(int camIndex, CameraManager* camManager, bool bFlag, CDC* pDC, HBRUSH greenBrush, HBRUSH redBrush)
{
    HBRUSH hbr = nullptr;
    if (camManager->m_Cam[camIndex] != nullptr)
    {
        if (bFlag)
        {
            pDC->SetBkColor(RGB_GREEN);
            hbr = greenBrush;
        }
        else {
            pDC->SetBkColor(RGB_RED);
            hbr = redBrush;
        }
    }

    return hbr;
}

// =============================================================================
HBRUSH CMDS_Ebus_SampleDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // TODO:  여기서 DC의 특성을 변경합니다.

    // TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
    UINT nID = pWnd->GetDlgCtrlID();

    switch (nCtlColor)
    {
    case CTLCOLOR_DLG:
        pDC->SetTextColor(BLACK);
        pDC->SelectStockObject(BLACK);
        //pDC->SetBkColor(WHITE);
        hbr = (HBRUSH)GetStockObject(NULL_BRUSH);
        return (HBRUSH)(m_brush2->GetSafeHandle());

    case CTLCOLOR_BTN:
        switch (nID)
        {
        }

    case CTLCOLOR_EDIT:
    case CTLCOLOR_LISTBOX:

        switch (nID)
        {

        case IDC_LIST_LOG:
            pDC->SetTextColor(WHITE);
            pDC->SelectStockObject(WHITE_PEN);
            pDC->SetBkColor(BLACK);
            pDC->SetBkMode(TRANSPARENT);
            return (HBRUSH)(m_brush2->GetSafeHandle());

        default:
            break;
        }

    case CTLCOLOR_STATIC:
        switch (nID) 
        {
       
        case IDC_CAM1_IS_CONNECT:
            if(m_CamManager->m_Cam[CAM_1] != nullptr)
                return SetCameraFlagStatus(CAM_1, m_CamManager, m_CamManager->m_Cam[CAM_1]->GetCamRunningFlag(), pDC, m_bGreen, m_bRed);
        case IDC_CAM2_IS_CONNECT:
            if (m_CamManager->m_Cam[CAM_2] != nullptr)
                return SetCameraFlagStatus(CAM_2, m_CamManager, m_CamManager->m_Cam[CAM_2]->GetCamRunningFlag(), pDC, m_bGreen, m_bRed);
        case IDC_CAM3_IS_CONNECT:
            if (m_CamManager->m_Cam[CAM_3] != nullptr)
                return SetCameraFlagStatus(CAM_3, m_CamManager, m_CamManager->m_Cam[CAM_3]->GetCamRunningFlag(), pDC, m_bGreen, m_bRed);
        case IDC_CAM4_IS_CONNECT:
            if (m_CamManager->m_Cam[CAM_4] != nullptr)
                return SetCameraFlagStatus(CAM_4, m_CamManager, m_CamManager->m_Cam[CAM_4]->GetCamRunningFlag(), pDC, m_bGreen, m_bRed);
        case IDC_CAM1_RECORDING:
            return HandleCameraRecordingStatus(CAM_1, pDC);
        case IDC_CAM2_RECORDING:
            return HandleCameraRecordingStatus(CAM_2, pDC);
        case IDC_CAM3_RECORDING:
            return HandleCameraRecordingStatus(CAM_3, pDC);
        case IDC_CAM4_RECORDING:
            return HandleCameraRecordingStatus(CAM_4, pDC);
        
        case IDC_RADIO_CAM1:
        case IDC_RADIO_CAM2:
        case IDC_RADIO_CAM3:
        case IDC_RADIO_CAM4:
        case IDC_CK_UYVY:
        case IDC_CK_COLORMAP:
        case IDC_CK_MESSAGE:
        case IDC_CK_MONO:
        case IDC_CK_CAM1_ROI:
        case IDC_CK_CAM1_ROI2:
        case IDC_STATIC1:
        case IDC_STATIC2:
        case IDC_STATIC3:
        case IDC_STATIC4:
        case IDC_STATIC5:
        case IDC_STATIC6:
        case IDC_STATIC7:
        case IDC_STATIC8:
        case IDC_STATIC9:
        case IDC_STATIC10:
        case IDC_STATIC11:
        case IDC_STATIC12:
        case IDC_STATIC13:
        case IDC_STATIC14:
        case IDC_STATIC15:
        case IDC_STATIC16:
        case IDC_STATIC17:
        case IDC_STATIC18:
        case IDC_STATIC19:
        case IDC_STATIC20:
        case IDC_STATIC21:
        case IDC_STATIC22:
        case IDC_STATIC23:
        case IDC_STATIC24:
        case IDC_STATIC25:
        case IDC_STATIC26:
        case IDC_STATIC27:
        case IDC_STATIC28:
        case IDC_STATIC29:
        case IDC_STATIC30:
        case IDC_STATIC31:
        case IDC_STATIC32:
        case IDC_STATIC33:
        case IDC_STATIC34:
        case IDC_STATIC35:
            pDC->SetTextColor(LIGHTBLUE);
            pDC->SetBkMode(TRANSPARENT);
            return (HBRUSH)::GetStockObject(NULL_BRUSH);       

        case IDC_STATIC_CAM1_ROI:
        case IDC_STATIC_CAM2_ROI:
        case IDC_STATIC_CAM3_ROI:
        case IDC_STATIC_CAM4_ROI:
        case IDC_STATIC_CAM1_Min:
        case IDC_STATIC_CAM1_Max:
        case IDC_STATIC_CAM2_Min:
        case IDC_STATIC_CAM2_Max:
        case IDC_STATIC_CAM3_Min:
        case IDC_STATIC_CAM3_Max:
        case IDC_STATIC_CAM4_Min:
        case IDC_STATIC_CAM4_Max:
        case IDC_STATIC_CAM1_FPS:
        case IDC_STATIC_CAM2_FPS:
        case IDC_STATIC_CAM3_FPS:
        case IDC_STATIC_CAM4_FPS:
        case IDC_RADIO_RECT:
        case IDC_RADIO_CIRCLE:
        case IDC_RADIO_ELLIPSE:
        case IDC_RADIO_LINE:
        case IDCLOSE:
        case IDC_STATIC_CAMCOUNT:
        case IDC_STATIC:
        case IDC_STATIC36:
            pDC->SetTextColor(LIGHTGREEN);
            pDC->SetBkMode(TRANSPARENT);
            return (HBRUSH)::GetStockObject(NULL_BRUSH); 

        case IDC_CB_CAM1_COLORMAP:
        case IDC_CB_CAM2_COLORMAP:
        case IDC_CB_CAM3_COLORMAP:
        case IDC_CB_CAM4_COLORMAP:
            pDC->SetTextColor(LIGHTGREEN);           
            pDC->SetBkColor(BLACK);
            pDC->SetBkMode(TRANSPARENT);
            return (HBRUSH)(m_brush2->GetSafeHandle());

        default:
            break;
        }
    }
    return hbr;
}

HBRUSH CMDS_Ebus_SampleDlg::HandleCameraRecordingStatus(int camIndex, CDC* pDC)
{
    HBRUSH  RedBrush, YellowBrush;
    RedBrush = m_bRed;
    YellowBrush = m_bYellow;

    if (m_CamManager->m_Cam[camIndex] != nullptr)
    {
        if (m_blink[camIndex] && m_CamManager->m_Cam[camIndex]->GetImageProcessor()->GetStartRecordingFlag())
        {
            // 녹화 중일 때 깜박이는 효과
            pDC->SetBkColor(RedBrush ? RGBYELLOW : RGB_RED);
            return RedBrush ? RedBrush : YellowBrush;
        }
        else
        {
            //return SetCameraFlagStatus(camIndex, m_CamManager, m_CamManager->m_Cam[camIndex]->GetStartRecordingFlag(), pDC, m_bGreen, m_bRed);
        }
    }
    return nullptr;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnAF()
{
    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
    {
        m_CamManager->m_Cam[GetSelectCamIndex()]->SetAutoFocus();
    }


    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    

    //ShellExecute(0, L"open", L"path\\to\\python.exe", L"path\\to\\script.py", NULL, SW_SHOW);

    // 다이얼로그를 통해 이미지 파일 열기
    //CString filePath;
    //CFileDialog dlg(TRUE); // TRUE는 파일 열기 모드
    //if (dlg.DoModal() == IDOK)
    //{
    //    filePath = dlg.GetPathName();

    //    string stdImage = Common::GetInstance()->CString2str(filePath);
    //    Mat originalImage = imread(stdImage);

    //    if (!originalImage.empty())
    //    {
    //        Common::GetInstance()->AddLog(0, _T("[Img] Load Image %s"), filePath);
    //        //resize(originalImage, originalImage, cv::Size(), 1.5, 1.5);

    //        imshow("originalImage", originalImage);


    //        Mat grayImage;
    //        cvtColor(originalImage, grayImage, COLOR_BGR2GRAY);

    //        // 이진화 수행 (임계값은 100으로 설정)
    //        int thresholdValue = 70;
    //        Mat binaryImage;
    //        threshold(grayImage, binaryImage, thresholdValue, 255, THRESH_BINARY);


    //        imshow("binaryImage", binaryImage);

    //        // 캐니 에지 검출 수행
    //        Mat cannyImage;
    //        Canny(binaryImage, cannyImage, 50, 150); // 임계값은 임의로 지정

    //        // 외곽선 표시를 위해 원본 이미지 복사
    //        Mat resultImage = originalImage.clone();

    //        // 외곽선 그리기
    //        std::vector<std::vector<cv::Point>> contours;
    //        findContours(cannyImage, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    //        drawContours(resultImage, contours, -1, Scalar(0, 255, 0), 2);

    //        // 결과 이미지 
    //        imshow("Contours", resultImage);

    //        waitKey(0); // 사용자가 아무 키나 누를 때까지 대기
    //    }
    //    else
    //    {
    //        // 이미지 읽기 실패 시에 대한 처리
    //        Common::GetInstance()->AddLog(0, _T("[Img] Image Load Fail"));
    //    }
    //}
    

    //PvResult result = -1;
    //if (m_CamManager != nullptr)
    //{
    //    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
    //    {
    //        PvGenParameterArray* lDeviceParams = m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->GetParameters();
    //        result = lDeviceParams->SetFloatValue("AcquisitionFrameRate", 30);
    //        if (result.IsOK())
    //        {
    //            Common::GetInstance()->AddLog(0, _T("[Cam %d] Set FPS : 30"), GetSelectCamIndex()+1);
    //        }
    //        else
    //            Common::GetInstance()->AddLog(0, _T("[Cam %d] Set FPS Fail"), GetSelectCamIndex()+1);
    //    }
    //}
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnFps30()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.+
    PvResult result = -1;
    if (m_CamManager != nullptr)
    {
        if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
        {
            PvGenParameterArray* lDeviceParams = m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->GetParameters();
            result = lDeviceParams->SetFloatValue("AcquisitionFrameRate", 10);
            if (result.IsOK())
            {
                Common::GetInstance()->AddLog(0, _T("[Cam %d] Set FPS : 10"), GetSelectCamIndex()+1);
            }
            else
                Common::GetInstance()->AddLog(0, _T("[Cam %d] Set FPS Fail"), GetSelectCamIndex()+1);
        }    
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedCkParam()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

     if (m_chGenICam_checkBox.GetCheck())
        Common::GetInstance()->m_bBufferCheck = true;
    else
        Common::GetInstance()->m_bBufferCheck = false;  
}

// =============================================================================
void CMDS_Ebus_SampleDlg::Btn_Interface_setting()
{
    /*m_BtnDeviceCtrl.SetFont(&m_basefont, true);*/


    SetWindowTheme(this->GetDlgItem(IDC_RADIO_CAM1)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_CAM2)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_CAM3)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_CAM4)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_RECT)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_CIRCLE)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_ELLIPSE)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_RADIO_LINE)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CK_UYVY)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CK_COLORMAP)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CK_MESSAGE)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CK_MONO)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CK_CAM1_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CK_CAM1_ROI2)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM1_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM2_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM3_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM1_Min)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM1_Max)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM2_Min)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM2_Max)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM3_Min)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM3_Max)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_Min)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_Max)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM1_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM2_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM3_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM3_Max)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_Min)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_Max)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_ROI)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM1_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM2_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM3_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAM4_FPS)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC_CAMCOUNT)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDCLOSE)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CAM1_RECORDING)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CAM2_RECORDING)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CAM3_RECORDING)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CAM4_RECORDING)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CB_CAM1_COLORMAP)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CB_CAM2_COLORMAP)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CB_CAM3_COLORMAP)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_CB_CAM4_COLORMAP)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC27)->GetSafeHwnd(), L"", L"");
    SetWindowTheme(this->GetDlgItem(IDC_STATIC30)->GetSafeHwnd(), L"", L"");


    

    m_BtnDeviceCtrl.SetText(_T("Device"));
    m_BtnCommunicationCtrl.SetText(_T("Comm"));
    m_BtnImageStreamCtrl.SetText(_T("Image stream"));
    m_BtnStart.SetText(_T("Start"));
    m_BtnStop.SetText(_T("Stop"));
    /*m_BtnConnect.SetText(_T("Connect / Select"));*/
    m_BtnDisconnect.SetText(_T("Disconnect"));
    m_BtnFPS30.SetText(_T("FPS 30"));
    m_BtnAF.SetText(_T("Auto Focus"));
    m_BtnDeviceFind.SetText(_T("Device Find"));
    m_BtnLoadiniFile.SetText(_T("System Param *.ini"));
    m_BtniniApply.SetText(_T("Apply"));
    m_BtnCamParam.SetText(_T("Camera Parameter"));
    m_BtnCamParamApply.SetText(_T("Apply"));
    m_BtnDataFolder.SetText(_T("Data Folder"));
    m_BtnImageSnap.SetText(_T("ImageSnap"));
    m_BtnImageRecording.SetText(_T("Image Recording"));

    m_progress.SetRange(0, 100);

    /*m_BtnConnect.EnableWindow(false);*/
}

bool CMDS_Ebus_SampleDlg::IsMouseEventCheck(UINT message)
{
    return (message == WM_LBUTTONDOWN || message == WM_MOUSEMOVE || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK);
}

// =============================================================================
BOOL CMDS_Ebus_SampleDlg::PreTranslateMessage(MSG* pMsg)
{
    const int buttonIDs[] =
    {
        IDC_BTN_DEVICE, IDC_BTN_COMMUNICATION, IDC_BTN_IMG_STREAM,
        IDC_BTN_START, IDC_BTN_STOP,
        IDC_BTN_DISCONNECT, IDC_BTN_FPS30, IDC_BTN_AF,
        IDC_BTN_DEVICEFIND, IDC_BTN_LOAD_INI_FILE, IDC_BTN_INI_APPLY, IDC_BTN_CAM_PARAM,
        IDC_BTN_CAM_PARAM_APPLY, IDC_BTN_OPEN_DATA_FOLDER,IDC_BTN_IMG_SNAP, IDC_BTN_IMG_RECORDING,
    };
    CRect Camrect[CAMERA_COUNT];

    // 각 카메라의 영역을 정의합니다.
    CStatic* DisplayCam[CAMERA_COUNT] =
    {
        (CStatic*)GetDlgItem(IDC_CAM1),
        (CStatic*)GetDlgItem(IDC_CAM2),
        (CStatic*)GetDlgItem(IDC_CAM3),
        (CStatic*)GetDlgItem(IDC_CAM4)
    };

    CPoint point;
    GetCursorPos(&point);
    ScreenToClient(&point);

    if (IsMouseEventCheck(pMsg->message))
    {   
        for (int i = 0; i < CAMERA_COUNT; ++i)
        {
            DisplayCam[i]->GetWindowRect(&Camrect[i]);
            ScreenToClient(&Camrect[i]);
        }

        for (int i = 0; i < CAMERA_COUNT; ++i)
        {
            if (Camrect[i].PtInRect(point))
            {
                int nCamIndex = (i);
                UpdateCameraROI(DisplayCam[i], Camrect[i], m_CamManager, nCamIndex, m_Cam_selecting_roi, m_StartPos, m_EndPos, pMsg->message);
            }
        }
    }

    if (pMsg->message == WM_LBUTTONDOWN)
    {
        for (const auto& buttonID : buttonIDs)
        {
            CSkinButton* pbutton = (CSkinButton*)GetDlgItem(buttonID);
            CRect rect;
            pbutton->GetWindowRect(rect);

            if (rect.PtInRect(pMsg->pt))
            {
                pbutton->RandomizeColors();
            }
        }

        for (int i = 0; i < CAMERA_COUNT; ++i)
        {
            DisplayCam[i]->GetWindowRect(&Camrect[i]);
            ScreenToClient(&Camrect[i]);
        }

        UINT radioIDs[] = { IDC_RADIO_CAM1, IDC_RADIO_CAM2, IDC_RADIO_CAM3, IDC_RADIO_CAM4 };
        for (int i = 0; i < CAMERA_COUNT; ++i)
        {
            if (Camrect[i].PtInRect(point))
            {
                int camIndex = (i);

                if (i < sizeof(radioIDs) / sizeof(radioIDs[0]) && m_CamManager->m_Cam[i] != nullptr)
                {
                    RadioCtrl(radioIDs[i]); // 라디오 버튼 업데이트 함수 호출
                }
                break; // 해당 카메라에서 이벤트 처리 후 루프 종료
            }
        }
    }

    if (pMsg->message == WM_MOUSEMOVE)
    {
        POINT pos;
        GetCursorPos(&pos);

        // 버튼ID 배열에 대하여 스타일을 업데이트합니다.
        for (const auto& buttonID : buttonIDs)
        {
            // 버튼의 ID를 사용하여 해당 버튼 객체 획득
            CSkinButton* pbutton = (CSkinButton*)GetDlgItem(buttonID);
            //마우스 위치 정보 전달.
            UpdateButtonStyle(pbutton, pMsg->pt);
        }

        UpdateData(true);

    }

    if (pMsg->message == WM_RBUTTONDOWN)
    {
        
        CPoint point(pMsg->pt);
        ScreenToClient(&point);

        for (int i = 0; i < CAMERA_COUNT; ++i)
        {
            CRect rect;
            DisplayCam[i]->GetWindowRect(&rect);
            ScreenToClient(&rect);

            // 포인트가 특정 카메라 영역 내에 있는지 확인
            if (rect.PtInRect(point))
            {
                int nCamIndex = i;  // 현재 선택된 카메라 인덱스
                if (m_CamManager->m_Cam[nCamIndex] != nullptr)
                {       
                    // 스크린 좌표를 이미지 좌표로 변환
                    cv::Point imagePoint = m_CamManager->m_Cam[nCamIndex]->GetImageProcessor()->mapPointToImage(point, m_CamManager->m_Cam[nCamIndex]->GetImageProcessor()->GetImageSize(), rect);

                    // ROI 인덱스 찾기
                    int roiIndex = m_CamManager->m_Cam[nCamIndex]->GetImageProcessor()->findROIIndexAtPoint(imagePoint);

                    if (roiIndex != -1)
                    {
                        // 팝업 메뉴 생성
                        ShowPopupMenu(point, roiIndex);
                    }
                }
                return TRUE;  // 메시지 처리 완료
            }
        }
        
    }


    switch (pMsg->message)

    {
        // 키가 눌렸을때
        case WM_KEYDOWN:
        {
            switch (pMsg->wParam)

            {
            case 0x53: // "s" key
                OnBnClickedBtnStart();
                return TRUE;
            case 0x44: // "d" key
                OnBnClickedBtnStop();
                return TRUE;
            case 0x43: // "c" key
                m_List_Log.ResetContent();
                return TRUE;

            case VK_F1:
                if (m_DeviceWnd == NULL)
                {
                    OnBnClickedDeviceCtrl();
                }
                else
                {
                    CloseGenWindow(&m_DeviceWnd);
                }
                return TRUE;
            case VK_F2:
                if (m_CommunicationWnd == NULL)
                {
                    OnBnClickedBtnCommunicationCtrl();
                }
                else
                {
                    CloseGenWindow(&m_CommunicationWnd);
                }
                return TRUE;

            case VK_F3:
                if (m_StreamParametersWnd == NULL)
                {
                    OnBnClickedBtnImgStream();
                }
                else
                {
                    CloseGenWindow(&m_StreamParametersWnd);
                }
                return TRUE;

                // 리턴키\tab
            case VK_RETURN:
            {

            }
            return TRUE;

            // ESC키

            case VK_ESCAPE:
            {

            }
            
            return TRUE;

            }
        }
    }

    return false;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::UpdateButtonStyle(CSkinButton* pbutton, const CPoint& cursorPos)
{
    // UpdateButtonStyle: 주어진 CSkinButton 객체의 스타일을 업데이트합니다.
    // pbutton: 스타일을 업데이트할 CSkinButton 객체의 포인터입니다.
    // cursorPos: 현재 마우스 커서의 위치입니다.


     // pbutton이 nullptr이거나 윈도우가 비활성화된 경우 함수를 종료합니다.
    if (pbutton == nullptr || !pbutton->IsWindowEnabled()) 
    {
        return;
    }

    CRect rect;
    pbutton->GetWindowRect(rect);

    // 만약 마우스 커서가 버튼 영역 내에 있다면
    if (rect.PtInRect(cursorPos)) 
    {
        pbutton->SetColorBackground(COLOR_HOVER);
        pbutton->SetColorText(COLOR_TEXT_HOVER);
        pbutton->SetFontStyle(Gdiplus::FontStyleBold, true);
    }
    else 
    {
        pbutton->SetColorBackground(COLOR_DEFAULT);
        pbutton->SetColorText(COLOR_TEXT_DEFAULT);
        pbutton->SetFontStyle(Gdiplus::FontStyleRegular);
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::DlgEnabledWindows()
{

}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedCkPointer()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    if (m_chPointerCheckBox.GetCheck())
        Common::GetInstance()->m_bPointerCheck = true;
    else
        Common::GetInstance()->m_bPointerCheck = false;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedCkMarker()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    if (m_chMarkerCheckBox.GetCheck())
        Common::GetInstance()->m_bMarkerCheck = true;
    else
        Common::GetInstance()->m_bMarkerCheck = false;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnDeviceFind()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    if(m_CamManager != nullptr)
        m_CamManager->CameraDeviceFind(this);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::SetSelectCamIndex(int nIndex)
{
    m_nSelectCamIndex = nIndex;
}

// =============================================================================
int CMDS_Ebus_SampleDlg::GetSelectCamIndex()
{
    return m_nSelectCamIndex;
}

// =============================================================================
void CMDS_Ebus_SampleDlg::RadioCtrl(UINT radio_Index)
{

    if (m_CamManager == nullptr)
        return;

   
    // Radio 인덱스를 카메라 인덱스로 변환
    int camIndex = -1;
    switch (radio_Index)
    {
    case IDC_RADIO_CAM1:
        camIndex = 0;
        break;
    case IDC_RADIO_CAM2:
        camIndex = 1;
        break;
    case IDC_RADIO_CAM3:
        camIndex = 2;
        break;
    case IDC_RADIO_CAM4:
        camIndex = 3;
        break;
    default:
        camIndex = -1; // 유효하지 않은 인덱스
        break;
    }

    UpdateData(TRUE);
    if (camIndex != -1)
    {
        SetSelectCamIndex(camIndex);

        // 모든 라디오 버튼을 초기화
        const UINT radioIDs[] = { IDC_RADIO_CAM1, IDC_RADIO_CAM2, IDC_RADIO_CAM3, IDC_RADIO_CAM4 };
        for (UINT id : radioIDs)
        {
            CButton* pCheck = (CButton*)GetDlgItem(id);
            if (pCheck != nullptr)
            {
                // 현재 인덱스의 라디오 버튼만 체크, 나머지는 체크 해제
                pCheck->SetCheck(id == radio_Index);
            }
        }

        UpdateCheckBoxes(camIndex);
        //bug
        //PopulateComboBoxes_Range();
    }

    UpdateData(FALSE);
}

void CMDS_Ebus_SampleDlg::UpdateCheckBoxes(int camIndex)
{
    if (m_CamManager->m_Cam[camIndex] != nullptr)
    {
        // 체크박스 상태 설정 로직
         //16비트 타입일때는 UYVY 사용안함

        int nIndex = GetSelectCamIndex();
        if (m_CamManager->m_Cam[nIndex]->GetImageProcessor()->Get16BitType())
        {
            m_chUYVYCheckBox.EnableWindow(FALSE);
            if (m_CamManager->m_Cam[nIndex]->GetImageProcessor()->GetGrayType())
            {
                m_chMonoCheckBox.SetCheck(TRUE);
                m_chColorMapCheckBox.SetCheck(FALSE);
            }
            else if (m_CamManager->m_Cam[nIndex]->GetImageProcessor()->GetColorPaletteType())
            {
                m_chColorMapCheckBox.SetCheck(TRUE);
                m_chMonoCheckBox.SetCheck(FALSE);
            }
            else if (m_CamManager->m_Cam[nIndex]->GetImageProcessor()->GetRGBType())
            {
                m_chUYVYCheckBox.SetCheck(FALSE);
                m_chColorMapCheckBox.SetCheck(FALSE);
                m_chMonoCheckBox.SetCheck(FALSE);
            }
        }
        // 실화상 카메라일경우
        if (m_CamManager->m_Cam[nIndex]->GetImageProcessor()->GetRGBType())
        {
            m_chUYVYCheckBox.EnableWindow(FALSE);
            m_chColorMapCheckBox.EnableWindow(FALSE);
            m_chMonoCheckBox.EnableWindow(FALSE);
            m_chEventsCheckBox.EnableWindow(FALSE);
        }
        else if (m_CamManager->m_Cam[nIndex]->GetImageProcessor()->GetYUVYType() == FALSE && m_CamManager->m_Cam[nIndex]->GetImageProcessor()->Get16BitType())
        {
            //m_chUYVYCheckBox.EnableWindow(TRUE);
            m_chColorMapCheckBox.EnableWindow(TRUE);
            m_chMonoCheckBox.EnableWindow(TRUE);
            m_chEventsCheckBox.EnableWindow(FALSE);
        }
        //UYVY 경우
        else
        {
            m_chUYVYCheckBox.EnableWindow(TRUE);
            m_chColorMapCheckBox.EnableWindow(FALSE);
            m_chMonoCheckBox.EnableWindow(FALSE);
            m_chEventsCheckBox.EnableWindow(FALSE);
        }
    }
}

// =============================================================================
bool CMDS_Ebus_SampleDlg::LoadiniFile()
{
    bool bFlag = false;

    CString filePath = _T(""), strSetfilepath = _T("");
    CString iniSection = _T(""), iniKey = _T(""), iniValue = _T("");
    TCHAR cbuf[MAX_PATH] = { 0, };
    //int iValue;
    //BOOL bValue;
    //iValue = _ttoi(iniValue);



    filePath.Format(Common::GetInstance()->SetProgramPath(_T("CameraParams.ini")));

    // 기존 데이터가 있다면 삭제
    m_CamManager->ClearAddressdata();

    iniSection.Format(_T("System"));
    iniKey.Format(_T("Path"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    strSetfilepath.Format(_T("%s\\%s\\"), Common::GetInstance()->GetProgramDirectory(), cbuf);
    Common::GetInstance()->Setsetingfilepath(strSetfilepath);

    /*-------------------------------------------------------------------*/
    iniKey.Format(_T("AutoStart"));
    bool bAutoFlag = GetPrivateProfileInt(iniSection, iniKey, (bool)false, filePath);
    Common::GetInstance()->SetAutoStartFlag(bAutoFlag);
    
    /*-------------------------------------------------------------------*/
    iniSection.Format(_T("Camera_IP_Address"));
    iniKey.Format(_T("Cam1"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    m_CamManager->SetCameraIPAddress(iniValue);

    iniKey.Format(_T("Cam2"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    m_CamManager->SetCameraIPAddress(iniValue);

    iniKey.Format(_T("Cam3"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    m_CamManager->SetCameraIPAddress(iniValue);

    iniKey.Format(_T("Cam4"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    m_CamManager->SetCameraIPAddress(iniValue);
    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/
 /*   iniSection.Format(_T("Camera_1_Params"));
    iniKey.Format(_T("Cam1"));
    GetPrivateProfileString(iniSection, iniKey, _T("0"), cbuf, MAX_PATH, filePath);
    iniValue.Format(_T("%s"), cbuf);
    m_CamManager->SetCameraIPAddress(iniValue);*/

    return bFlag;
}

// =============================================================================
// 마우스 드래그 영역 정보를 저장한다.
void CMDS_Ebus_SampleDlg::UpdateCameraROI(CStatic* displayControl, const CRect& controlRect, CameraManager* camManager, int cameraIndex, bool& selectingROI, cv::Point& startPos, cv::Point& endPos, UINT message)
{
    CRect rect;
    displayControl->GetWindowRect(&rect);
    ScreenToClient(&rect);
    CPoint clientPoint;
    GetCursorPos(&clientPoint);
    ScreenToClient(&clientPoint);

    cv::Size imageSize;
    ImageProcessor* processor = nullptr;

    if (camManager && camManager->m_Cam[cameraIndex]) 
    {
        processor = camManager->m_Cam[cameraIndex]->GetImageProcessor();
        if (processor)
        {
            imageSize = processor->GetImageSize();
        }
        else 
        {
            return;
        }
    }
    else 
    {
        return;
    }

    bool isCreatingROI = (m_ch_Cam1_ROI_Create.GetCheck() == BST_CHECKED);

    switch (message) {
    case WM_LBUTTONDOWN:
        if (rect.PtInRect(clientPoint))
        {
            if (isCreatingROI) 
            {
                processor->StartCreatingROI(clientPoint, rect, imageSize, startPos, endPos, selectingROI);
            }
            else 
            {
                processor->StartDraggingROI(clientPoint, rect, imageSize, m_dragStartPos, m_selectedROIIndex, m_isDraggingROI);
            }
        }
        break;

    case WM_MOUSEMOVE:
        if (selectingROI && isCreatingROI) 
        {
            processor->UpdateCreatingROI(clientPoint, rect, imageSize, startPos, endPos);
        }
        else if (m_isDraggingROI)
        {
            processor->UpdateDraggingROI(clientPoint, rect, imageSize, m_dragStartPos, m_selectedROIIndex);
        }
        break;

    case WM_LBUTTONUP:
        if (selectingROI && isCreatingROI) 
        {
            processor->ConfirmCreatingROI(startPos, endPos, rect, imageSize, selectingROI);
        }
        else if (m_isDraggingROI) 
        {
            processor->ConfirmDraggingROI(m_isDraggingROI, m_selectedROIIndex);
        }
        break;

    case WM_RBUTTONDOWN:
        if (selectingROI) 
        {
            selectingROI = false;
            processor->m_isAddingNewRoi = false;
            processor->ClearTemporaryROI();
        }
        break;
    }
}

// =============================================================================
// 설정 파일 오픈
void CMDS_Ebus_SampleDlg::OnBnClickedBtnLoadIniFile()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    CString filePath = _T(""), strSetfilepath = _T("");

    //실행파일 경로내에 카메라 설정 파일 경로
    filePath.Format(Common::GetInstance()->SetProgramPath(_T("CameraParams.ini")));

    // 파일 열기
    if (PathFileExists(filePath))
    {
        // 파일이 존재하면 열기
        ShellExecute(NULL, _T("open"), filePath, NULL, NULL, SW_SHOWNORMAL);
    }
    else
    {
        Common::GetInstance()->AddLog(0, _T("CameraParams.ini File not found"));
    }
}
// 카메라 설정 파일 오픈
void CMDS_Ebus_SampleDlg::CameraParamsFileOpen()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    int selectedCamIndex = GetSelectCamIndex();


    if (selectedCamIndex >= CAM_1 && selectedCamIndex <= CAM_4)
    {
        CString strFileName;
        strFileName.Format(_T("CameraParams_%d.ini"), selectedCamIndex + 1);

        CString filePath = Common::GetInstance()->SetProgramPath(strFileName);

        if (PathFileExists(filePath))
        {
            // 파일이 존재하면 열기
            ShellExecute(NULL, _T("open"), filePath, NULL, NULL, SW_SHOWNORMAL);
        }
        else
        {
            Common::GetInstance()->AddLog(0, _T("%s.ini File not found"), strFileName);
        }
    }
}

// =============================================================================
// 설정파일 데이터를 다시 불러와 적용시킨다.
void CMDS_Ebus_SampleDlg::OnBnClickedBtnIniApply()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() < 0)
        return;

    LoadiniFile();
    m_CamManager->CameraDeviceFind(this);

}

// =============================================================================
// 컬러맵 인덱스를 반환한다
PaletteTypes CMDS_Ebus_SampleDlg::GetSelectedColormap(CComboBox& comboControl)
{
    int selectedIndex = comboControl.GetCurSel(); // 현재 선택된 아이템의 인덱스 선택
    if (selectedIndex != CB_ERR) // 유효한 선택인지 확인.
    {
        return static_cast<PaletteTypes>(selectedIndex); // 인덱스를 ColormapTypes 열거형으로 캐스팅합니다.
    }
    else
    {
        // 오류 처리. 예를 들어, 기본 값을 반환할 수 있습니다.
        return PALETTE_TYPE::PALETTE_IRON;
    }
}

// =============================================================================
// 온도 설정 값  인덱스를 반환한다
CameraIRFormat CMDS_Ebus_SampleDlg::GetSelectedTempType(CComboBox& comboControl)
{
    int selectedIndex = comboControl.GetCurSel(); // 현재 선택된 아이템의 인덱스 선택
    if (selectedIndex != CB_ERR) // 유효한 선택인지 확인.
    {
        return static_cast<CameraIRFormat>(selectedIndex); // 인덱스를 ColormapTypes 열거형으로 캐스팅합니다.
    }
    else
    {
        // 오류 처리. 예를 들어, 기본 값을 반환할 수 있습니다.
        return Camera_IRFormat::RADIOMETRIC;
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnCbnSelchangeCam1()
{
    HandleComboChange(IDC_CB_CAM1_COLORMAP);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnCbnSelchangeCam2()
{
    HandleComboChange(IDC_CB_CAM2_COLORMAP);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnCbnSelchangeCam3()
{
    HandleComboChange(IDC_CB_CAM3_COLORMAP);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnCbnSelchangeCam4()
{
    HandleComboChange(IDC_CB_CAM4_COLORMAP);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::HandleComboChange(int controlId)
{
    int comboIndex = -1; // 초기화
    switch (controlId)
    {
    case IDC_CB_CAM1_COLORMAP:
        comboIndex = CAM_1;
        break;

    case IDC_CB_CAM2_COLORMAP:
        comboIndex = CAM_2;
        break;

    case IDC_CB_CAM3_COLORMAP:
        comboIndex = CAM_3;
        break;

    case IDC_CB_CAM4_COLORMAP:
        comboIndex = CAM_4;
        break;

    default:
        comboIndex = CAM_1;
        break;
    }

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(controlId);
    if (pComboBox)
    {
        PaletteTypes selectedMap = GetSelectedColormap(*pComboBox);
        ApplyColorSettings(selectedMap, comboIndex);
        pComboBox->RedrawWindow();
        pComboBox->Invalidate();
        pComboBox->UpdateData();
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::ApplyColorSettings(PaletteTypes selectedMap, int comboIndex)
{
    if (comboIndex > -1 && m_CamManager->m_Cam[comboIndex] != nullptr)
    {
        m_CamManager->m_Cam[comboIndex]->GetImageProcessor()->SetPaletteType(selectedMap);
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::ApplyIRformatSettings(CameraIRFormat selectedIR)
{
    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
    {
        PvGenParameterArray* lDeviceParams;

        m_CamManager->m_Cam[GetSelectCamIndex()]->SetIRFormat(selectedIR);
        lDeviceParams = m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device->GetParameters();
        m_CamManager->m_Cam[GetSelectCamIndex()]->SetStreamingCameraParameters(lDeviceParams, GetSelectCamIndex(), (CameraModelList)m_CamManager->m_Cam[GetSelectCamIndex()]->m_Camlist);
    }
}
void CMDS_Ebus_SampleDlg::ApplyTempRange(int nRangeIndex)
{
    bool bFlag = false;
    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
    {    
        bFlag = m_CamManager->m_Cam[GetSelectCamIndex()]->SetTempRange(nRangeIndex);
    }
}


// =============================================================================
// 컬러맵 콤보박스 초기값 설정
void CMDS_Ebus_SampleDlg::PopulateComboBoxes()
{
    // 할당된 콤보박스 아이템 데이터를 모두 동일하게 초기값 설정
    CComboBox* comboBoxes[] = { &m_Cam1_Colormap, &m_Cam2_Colormap, &m_Cam3_Colormap, &m_Cam4_Colormap };
    CComboBox* comboBoxes_IR = &m_cbIRFormat;
    int arrayLength = 0, arrayLength_IR = 0;

    while (ColormapArray::colormapStrings[arrayLength] != nullptr)
    {
        ++arrayLength;
    }

    while (IRFormatArray::IRFormatStrings[arrayLength_IR] != nullptr)
    {
        ++arrayLength_IR;
    }

    for (int i = 0; i < sizeof(comboBoxes) / sizeof(comboBoxes[0]); ++i)
    {
        CComboBox* combo = comboBoxes[i];
        combo->ModifyStyle(0, CBS_OWNERDRAWFIXED);
        for (int j = 0; j < arrayLength; ++j)
        {
            
            combo->AddString(ColormapArray::colormapStrings[j]);
            combo->SetItemData(j, LIGHTGREEN);
        }
        // 초기 선택값을 설정 (예: 첫 번째 항목을 선택)
        combo->SetCurSel(0); // 초기값 Iron color 

        combo->Invalidate();
        combo->UpdateData();
    }

    // IR Format String Add
    comboBoxes_IR->ModifyStyle(0, CBS_OWNERDRAWFIXED);
    for (int j = 0; j < arrayLength_IR; ++j)
    {
        comboBoxes_IR->AddString(IRFormatArray::IRFormatStrings[j]);
        comboBoxes_IR->SetItemData(j, LIGHTGREEN);
    }
    comboBoxes_IR->SetCurSel(0); // 초기값 RADIOMETRIC 
    comboBoxes_IR->Invalidate();
    comboBoxes_IR->UpdateData();

    // IR Format String Add
    if (comboBoxes_IR->GetCount() < 1)
    {
        comboBoxes_IR->ModifyStyle(0, CBS_OWNERDRAWFIXED);
        for (int j = 0; j < arrayLength_IR; ++j)
        {
            comboBoxes_IR->AddString(IRFormatArray::IRFormatStrings[j]);
            comboBoxes_IR->SetItemData(j, LIGHTGREEN);
        }
        comboBoxes_IR->SetCurSel(0); // 초기값 RADIOMETRIC 
        comboBoxes_IR->Invalidate();
        comboBoxes_IR->UpdateData();
    }
}

void CMDS_Ebus_SampleDlg::PopulateComboBoxes_Range()
{
    if (m_CamManager->m_Cam[GetSelectCamIndex()] == nullptr || m_CamManager == nullptr)
        return;

    
    CComboBox* comboBoxes_Range = &m_cbTempRange;

    if(m_cbTempRange.GetCount() > 1)
        m_cbTempRange.ResetContent();

    CString strRange;
    int nQCnt = m_CamManager->m_Cam[GetSelectCamIndex()]->m_Cam_Params->nQueryCnt;
    
    // Temp Range String Add
    comboBoxes_Range->ModifyStyle(0, CBS_OWNERDRAWFIXED);
    for (int j = 0; j < nQCnt; ++j)
    {
        strRange.Format(_T("%.2f ~ %.2f"), m_CamManager->m_Cam[GetSelectCamIndex()]->m_Cam_Params->dQueryCaseLowLimit[j], m_CamManager->m_Cam[GetSelectCamIndex()]->m_Cam_Params->dQueryCaseHighLimit[j]);
        comboBoxes_Range->AddString(strRange);
        comboBoxes_Range->SetItemData(j, LIGHTGREEN);
    }
    comboBoxes_Range->SetCurSel(0); // 초기값 RADIOMETRIC 
    comboBoxes_Range->Invalidate();
    comboBoxes_Range->UpdateData();
}

// =============================================================================
//카메라 파라미터 설정파일 로드
void CMDS_Ebus_SampleDlg::OnBnClickedBtnCamParam()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    CameraParamsFileOpen();
}

// =============================================================================
// 카메라 파라미터 설정파일  적용
void CMDS_Ebus_SampleDlg::OnBnClickedBtnCamParamApply()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() < 0)
        return;

    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr && (gui_status == GUI_STATUS::GUI_STEP_STOP || gui_status == GUI_STATUS::GUI_STEP_IDLE))
    {
        m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->LoadCaminiFile(GetSelectCamIndex());
        m_CamManager->m_Cam[GetSelectCamIndex()]->AcquireParameter(m_CamManager->m_Cam[GetSelectCamIndex()]->m_Device, 
            m_CamManager->m_Cam[GetSelectCamIndex()]->m_Stream, 
            m_CamManager->m_Cam[GetSelectCamIndex()]->m_Pipeline, GetSelectCamIndex());
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::ShowJudgeDlg()
{
    if (m_CamManager->GetDeviceCount() > 0)
    {
        Judgedlg = new JudgeStatusDlg(this);
        Judgedlg->Create(IDD_DLG_JUDGE, this);
        Judgedlg->ShowWindow(SW_SHOW);
    }

}

// =============================================================================
void CMDS_Ebus_SampleDlg::CloseJudgeDlg()
{
    if (Judgedlg->isCancleStatus())
        return;

    if (Judgedlg != nullptr && Judgedlg->GetSafeHwnd() != nullptr)
    {
        if (IsWindowVisible()) 
        {
            Judgedlg->OnCancel();
        }
        else 
        {
            // 윈도우가 숨겨져 있을 때 수행할 작업
        }
    }
    else 
    {
        // dlg 객체가 유효하지 않거나 윈도우 핸들을 얻을 수 없는 경우 처리
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedCheckBox()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    if (m_CamManager->m_Cam[GetSelectCamIndex()] == nullptr)
        return;

    CString strLog = _T("");
    // Mono 체크박스 상태 확인 및 처리
    BOOL isMonoChecked = m_chMonoCheckBox.GetCheck();
    BOOL isColorMapChecked = m_chColorMapCheckBox.GetCheck();
    
    // 클릭된 체크박스의 ID를 얻어옵니다.
    UINT nID = ((CButton*)GetFocus())->GetDlgCtrlID();

    if (nID == IDC_CK_MONO)
    {
        // m_chMonoCheckBox가 선택되었을 때 m_chColorMapCheckBox를 선택 해제
        if (isMonoChecked)
        {
            m_chColorMapCheckBox.SetCheck(BST_UNCHECKED);
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetGrayType(TRUE);
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetColorPaletteType(FALSE);
            strLog.Format(_T("[Camera[%d]] Gray Palette Mode"), GetSelectCamIndex() + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
    }
    else if (nID == IDC_CK_COLORMAP)
    {
        // m_chColorMapCheckBox가 선택되었을 때 m_chMonoCheckBox를 선택 해제
        if (isColorMapChecked)
        {
            m_chMonoCheckBox.SetCheck(BST_UNCHECKED);
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetGrayType(FALSE);
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetColorPaletteType(TRUE);

            strLog.Format(_T("[Camera[%d]] Color Palette Mode"), GetSelectCamIndex() + 1);
            Common::GetInstance()->AddLog(0, strLog);
        }
    }

    // UYVY 체크박스 상태 확인 및 처리
    BOOL isUYVYChecked = m_chUYVYCheckBox.GetCheck();
    m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetYUVYType(isUYVYChecked);
    // 16bit / 8bit 체크박스 상태 확인 및 처리
    BOOL is16BitChecked = m_chEventsCheckBox.GetCheck();
    m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->Set16BitType(is16BitChecked);
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnOpenDataFolder()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    if (m_CamManager == nullptr || m_CamManager->m_Cam[GetSelectCamIndex()] == nullptr)
        return;

    CString strLog = _T("");
    std::string strPath = m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->GetRawdataPath();
    if (Common::GetInstance()->OpenFolder(strPath))
    {

    }
    else
    {
        strLog.Format(_T("[Camera[%d]] Need to check folder path"), GetSelectCamIndex() + 1);
        Common::GetInstance()->AddLog(0, strLog);
    }   
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnStnClickedCam1Recording()
{
    CWnd* pClickedWnd = GetDlgItem(GetCurrentMessage()->wParam);
    if (!pClickedWnd)
        return;

    int nID = pClickedWnd->GetDlgCtrlID();

    if (nID >= IDC_CAM1_RECORDING && nID <= IDC_CAM4_RECORDING)
    {
        int camIndex = nID - IDC_CAM1_RECORDING; // 컨트롤의 ID에 따라 카메라 인덱스 계산

        if (m_CamManager->m_Cam[camIndex] != nullptr)
        {
            bool bFlag = m_CamManager->m_Cam[camIndex]->GetImageProcessor()->GetStartRecordingFlag();
            if (!bFlag)
            {
                m_CamManager->m_Cam[camIndex]->GetImageProcessor()->SetStartRecordingFlag(true);
            }
            else
            {
                m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->StopRecording();
            }
        }
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnImgSnap()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() < 0)
        return;

    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
    {
        bool bFlag = m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->GetMouseImageSaveFlag();
        if (!bFlag)
        {
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetMouseImageSaveFlag(true);
        }
    }
}

// =============================================================================
void CMDS_Ebus_SampleDlg::OnBnClickedBtnImgRecording()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    int nIndex = GetSelectCamIndex();

    if (m_CamManager->GetDeviceCount() < 0)
        return;

    if (m_CamManager->m_Cam[GetSelectCamIndex()] != nullptr)
    {
        bool bFlag = m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->GetStartRecordingFlag();
        if (!bFlag)
        {
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SetStartRecordingFlag(true);
        }
        else
        {
            m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->StopRecording();
        }
    }
}

void CMDS_Ebus_SampleDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    // TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

    CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);

    switch (nIDCtl)
    {
    case IDC_CB_CAM1_COLORMAP:
    case IDC_CB_CAM2_COLORMAP:
    case IDC_CB_CAM3_COLORMAP:
    case IDC_CB_CAM4_COLORMAP:
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);

        int nIndex = lpDrawItemStruct->itemID;
        if (nIndex >= 0) {
            CComboBox* pComboBox = (CComboBox*)GetDlgItem(nIDCtl);
            CString strItemText;
            pComboBox->GetLBText(nIndex, strItemText);
            COLORREF textColor = pComboBox->GetItemData(nIndex);

            dc.SetTextColor(textColor); // 아이템의 데이터로부터 텍스트 색상 설정
            dc.SetBkColor(::GetSysColor(COLOR_WINDOW)); // 배경 색상 설정

            dc.TextOut(lpDrawItemStruct->rcItem.left, lpDrawItemStruct->rcItem.top, strItemText);
        }

        dc.Detach();
    }

    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);

}

// =============================================================================
void CMDS_Ebus_SampleDlg::RedrawComboBox(int controlId)
{
    int comboIndex = -1; // 초기화
    switch (controlId)
    {
    case IDC_CB_CAM1_COLORMAP:
        comboIndex = CAM_1;
        break;

    case IDC_CB_CAM2_COLORMAP:
        comboIndex = CAM_2;
        break;

    case IDC_CB_CAM3_COLORMAP:
        comboIndex = CAM_3;
        break;

    case IDC_CB_CAM4_COLORMAP:
        comboIndex = CAM_4;
        break;

    default:
        comboIndex = CAM_1;
        break;
    }

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(controlId);
    if (pComboBox)
    {
        pComboBox->RedrawWindow();
        pComboBox->Invalidate();
        pComboBox->UpdateData();
    }
}

void CMDS_Ebus_SampleDlg::OnCbnDropdownCbCam1Colormap()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    RedrawComboBox(IDC_CB_CAM1_COLORMAP);
}

void CMDS_Ebus_SampleDlg::UpdateCameraInfo(CameraControl_rev* cam, CameraInfoLabels& labels)
{
    if (cam == nullptr) return;

    UpdateLabel(cam->GetCameraFPS(), labels.lbFps);
    
    if (cam->m_Camlist != CAM_MODEL::BlackFly)
    {
    //    UpdateLabel(cam->GetImageProcessor()->m_MinSpot.tempValue, labels.lbMin);
    //    UpdateLabel(cam->GetImageProcessor()->m_MaxSpot.tempValue, labels.lbMax);
        UpdateROIInfo(cam, labels.lbROI);
    }
    
    // 녹화상태 깜빡임 변수
    if (cam->GetCamRunningFlag() && cam->GetImageProcessor()->GetStartRecordingFlag())
    {
        m_blink[cam->GetCamIndex()] = !m_blink[cam->GetCamIndex()];
    }
}

void CMDS_Ebus_SampleDlg::UpdateLabel(double value, CTransparentStatic& label)
{
    CString currentText, newText;
    label.GetText(currentText);
    newText.Format(_T("%.2f"), value);

    // 텍스트가 변경된 경우에만 업데이트
    if (currentText != newText)
    {
        label.SetText(newText);
    }
}

void CMDS_Ebus_SampleDlg::UpdateLabel(int value, CTransparentStatic& label)
{
    CString currentText, newText;
    label.GetText(currentText);
    newText.Format(_T("[%d]"), value);

    // 텍스트가 변경된 경우에만 업데이트
    if (currentText != newText)
    {
        label.SetText(newText);
    }
}

void CMDS_Ebus_SampleDlg::UpdateROIInfo(CameraControl_rev* cam, CTransparentStatic& label)
{
    //cv::Rect rt = cam->GetImageProcessor()->m_Select_rect;
    CString currentText, newText;
    currentText.Format(_T("ROI Count : [%d]"), cam->GetImageProcessor()->m_roiResults.size());
    label.SetText(currentText);
    //newText.Format(_T("[Position] X [%d] Y [%d] W [%d] H [%d] Pixel Count [%d]"),
    //    rt.x, rt.y, rt.width, rt.height, rt.width * rt.height);

    //// 텍스트가 변경된 경우에만 업데이트
    //if (currentText != newText)
    //{
    //    label.SetText(newText);
    //}
}

void CMDS_Ebus_SampleDlg::InvalidateLabels(CameraInfoLabels& labels) 
{
    labels.lbFps.Invalidate();
    labels.lbMin.Invalidate();
    labels.lbMax.Invalidate();
    labels.lbROI.Invalidate();
    labels.lbConnectStatus.Invalidate();
    labels.lbRecordingStatus.Invalidate();

    labels.lbROI.UpdateWindow();
    labels.lbFps.UpdateWindow();
    labels.lbMax.UpdateWindow();
    labels.lbMin.UpdateWindow();
    labels.lbConnectStatus.UpdateWindow();
    labels.lbRecordingStatus.UpdateWindow();
}

void CMDS_Ebus_SampleDlg::CameraAutoStart(int deviceCount)
{
    if (gui_status == GUI_STATUS::GUI_STEP_IDLE && deviceCount > 0)
    {
        if (Common::GetInstance()->GetAutoStartFlag())
        {
            m_CamManager->CameraAllStart(this);
        }
        else 
        {
            for (int i = 0; i < deviceCount; i++) 
            {
                m_CamManager->CreateCamera(i);
            }
        }
    }

    // Update camera count display
    UpdateLabel(deviceCount, m_LbCamCount);

}

void CMDS_Ebus_SampleDlg::UpdateProgressValue() 
{
    //static PDH_HQUERY hQuery = NULL;
    //static PDH_HCOUNTER hCounterTotal = NULL;
    PDH_FMT_COUNTERVALUE counterValue;
    PDH_STATUS status;

    //if (hQuery == NULL) {
    //    // 쿼리 초기화
    //    status = PdhOpenQuery(NULL, 0, &hQuery);
    //    if (status != ERROR_SUCCESS) return;

    //    // 카운터 추가
    //    status = PdhAddCounter(hQuery, L"\\Processor(_Total)\\% Processor Time", 0, &hCounterTotal);
    //    if (status != ERROR_SUCCESS) {
    //        PdhCloseQuery(hQuery);
    //        hQuery = NULL;
    //        return;
    //    }
    //}

    // 데이터 수집
    PdhCollectQueryData(hQuery);

    // CPU 사용률 가져오기
    status = PdhGetFormattedCounterValue(hCounterTotal, PDH_FMT_DOUBLE, NULL, &counterValue);
    if (status == ERROR_SUCCESS) {
        double cpuUsage = counterValue.doubleValue;
        m_progress.SetPos(static_cast<int>(cpuUsage));
    }
}

void CMDS_Ebus_SampleDlg::SetControlsFont()
{

    m_StaticFont.DeleteObject();

    int nFontHeight = 16;
    m_StaticFont.CreateFont(
        nFontHeight,  // 높이
        0,   // 너비
        0,   // 이스케이프먼트
        0,   // 오리엔테이션
        FW_BOLD,  // 두께
        FALSE,    // 이탤릭
        FALSE,    // 밑줄
        0,        // 취소선
        ANSI_CHARSET,  // 문자셋
        OUT_DEFAULT_PRECIS,  // 출력 정밀도
        CLIP_DEFAULT_PRECIS, // 클리핑 정밀도
        DEFAULT_QUALITY,  // 품질
        DEFAULT_PITCH | FF_SWISS,  // 피치와 패밀리
        _T("Arial"));  // 폰트 이름



    const int staticControlIDs[] = {
      IDC_STATIC_CAMCOUNT, IDC_CK_UYVY, IDC_CK_MESSAGE, IDC_CK_MONO, IDC_CK_CAM1_ROI, IDC_CK_COLORMAP, IDC_CK_CAM1_ROI2,
      IDC_RADIO_CAM1, IDC_RADIO_CAM2, IDC_RADIO_CAM3, IDC_RADIO_CAM4,
      IDC_STATIC1, IDC_STATIC2, IDC_STATIC3, IDC_STATIC4, IDC_STATIC5,
      IDC_STATIC6, IDC_STATIC7, IDC_STATIC8, IDC_STATIC9, IDC_STATIC10,
      IDC_STATIC11, IDC_STATIC12, IDC_STATIC13, IDC_STATIC14, IDC_STATIC15,
      IDC_STATIC16, IDC_STATIC17, IDC_STATIC18, IDC_STATIC19, IDC_STATIC20,
      IDC_STATIC21, IDC_STATIC22, IDC_STATIC23, IDC_STATIC24, IDC_STATIC25,
      IDC_STATIC26, IDC_STATIC27, IDC_STATIC28, IDC_STATIC29, IDC_STATIC30,
      IDC_STATIC31, IDC_STATIC32, IDC_STATIC33, IDC_STATIC34,
      IDC_RADIO_RECT, IDC_RADIO_CIRCLE, IDC_RADIO_ELLIPSE, IDC_RADIO_LINE
    };

    for (int id : staticControlIDs)
    {
        CStatic* pStatic = (CStatic*)GetDlgItem(id);
        if (pStatic != nullptr)
        {
            pStatic->SetFont(&m_StaticFont);
        }
    }
}

void CMDS_Ebus_SampleDlg::SetTransparentStatic_ControlsFont()
{
    SetControlsFont();

    for (auto& labels : m_LbCamInfo)
    {
        labels.lbFps.SetFont(16, TRUE); // 예시로 모든 fps 레이블에 대해 폰트 설정
        labels.lbMin.SetFont(16, TRUE); // 다른 레이블에 대해서도 동일하게 설정
        labels.lbMax.SetFont(16, TRUE); // 다른 레이블에 대해서도 동일하게 설정
        labels.lbROI.SetFont(16, TRUE); // 다른 레이블에 대해서도 동일하게 설정
        // 이하 동일
    }
}

void CMDS_Ebus_SampleDlg::OnBnClickedBtnParamSet()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    //CString strEditText;
    //m_edExtOptTemp.GetWindowText(strEditText);
    //m_CamManager->m_Cam[GetSelectCamIndex()]->m_ExtOptTransm = _ttof(strEditText);

}


void CMDS_Ebus_SampleDlg::OnCbnSelchangeCbIrFormat()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CB_IR_FORMAT);
    if (pComboBox)
    {
        CameraIRFormat selectedIR = GetSelectedTempType(*pComboBox);
        
        ApplyIRformatSettings(selectedIR);
        pComboBox->RedrawWindow();
        pComboBox->Invalidate();
        pComboBox->UpdateData();
    }
}


void CMDS_Ebus_SampleDlg::OnCbnSelchangeCbTempRange()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    if (m_CamManager->m_Cam[GetSelectCamIndex()] == nullptr)
        return;

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CB_TEMP_RANGE);
    if (pComboBox)
    {
        CameraIRFormat selectedIR = GetSelectedTempType(*pComboBox);

        ApplyTempRange(pComboBox->GetCurSel());
        pComboBox->RedrawWindow();
        pComboBox->Invalidate();
        pComboBox->UpdateData();
    }
}

// 핸들러 함수 정의
void CMDS_Ebus_SampleDlg::OnPopupMenuSave()
{
    Common::GetInstance()->AddLog(1, _T("Save ROI selected"));
    
    m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->SaveRoisToIniFile(GetSelectCamIndex());
    // 추가적인 처리 코드 작성
}

void CMDS_Ebus_SampleDlg::OnPopupMenuDelete()
{
    Common::GetInstance()->AddLog(1, _T("Delete ROI selected"));

    if (m_nSelectedRoiIndex != -1)
    {
        m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->deleteROI(m_nSelectedRoiIndex);
        m_nSelectedRoiIndex = -1; // 선택된 ROI 인덱스 초기화
    }

    // 추가적인 처리 코드 작성
}

void CMDS_Ebus_SampleDlg::OnPopupMenuAllDelete()
{
    Common::GetInstance()->AddLog(1, _T("Delete All ROI "));

    m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->deleteAllROIs();

}

void CMDS_Ebus_SampleDlg::ShowPopupMenu(CPoint point, int roiIndex)
{
    CMenu menu;
    menu.CreatePopupMenu();

    menu.AppendMenu(MF_STRING, ID_POPUP_MENUITEM1, _T("Save ROI"));
    menu.AppendMenu(MF_STRING, ID_POPUP_MENUITEM2, _T("Delete ROI"));
    menu.AppendMenu(MF_STRING, ID_POPUP_MENUITEM3, _T("All Delete ROI"));

    // Menu event mapping
    CWnd* pWnd = GetForegroundWindow();

    if (pWnd != nullptr)
    {
        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd);

        // 현재 선택된 ROI 인덱스를 저장합니다.
        m_nSelectedRoiIndex = roiIndex;
    }

    menu.DestroyMenu();
}

void CMDS_Ebus_SampleDlg::OnBnClickedCkCam1Roi()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    if (m_CamManager->m_Cam[GetSelectCamIndex()] == nullptr)
        return;

    if (m_ch_Cam1_ROI_CheckBox.GetCheck())
        m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->m_ROIEnabled = true;
    else
        m_CamManager->m_Cam[GetSelectCamIndex()]->GetImageProcessor()->m_ROIEnabled = false;
}

void CMDS_Ebus_SampleDlg::ShapeTypeRadioCtrl(UINT radio_Index)
{
    if (m_CamManager == nullptr)
        return;

    ShapeType selectedType;

    switch (radio_Index)
    {
    case IDC_RADIO_RECT:
        selectedType = ShapeType::Rectangle;
        break;
    case IDC_RADIO_CIRCLE:
        selectedType = ShapeType::Circle;
        break;
    case IDC_RADIO_ELLIPSE:
        selectedType = ShapeType::Ellipse;
        break;
    case IDC_RADIO_LINE:
        selectedType = ShapeType::Line;
        break;
    default:
        AfxMessageBox(_T("Invalid shape type selected."));
        return;
    }

    // 카메라 인덱스는 이전에 선택된 값에서 가져옵니다.
    int camIndex = GetSelectCamIndex();
    if (camIndex != -1)
    {
        
        // 선택된 도형 유형을 카메라 매니저 또는 이미지 프로세서에 전달
        if (m_CamManager->m_Cam[camIndex] != nullptr)
            m_CamManager->m_Cam[camIndex]->GetImageProcessor()->SetCurrentShapeType(selectedType);

        // UI 업데이트
        const UINT shapeRadioIDs[] = { IDC_RADIO_RECT, IDC_RADIO_CIRCLE, IDC_RADIO_ELLIPSE, IDC_RADIO_LINE };
        for (UINT id : shapeRadioIDs)
        {
            CButton* pCheck = (CButton*)GetDlgItem(id);
            if (pCheck != nullptr)
            {
                pCheck->SetCheck(id == radio_Index);
            }
        }
    }
    else
    {
        AfxMessageBox(_T("No camera selected."));
    }
}

// OnAddLog 메시지 핸들러
LRESULT CMDS_Ebus_SampleDlg::OnAddLog(WPARAM wParam, LPARAM lParam)
{
    CString* pStr = reinterpret_cast<CString*>(wParam);
    if (pStr != nullptr) 
    {
        int nIndex = m_List_Log.AddString(*pStr);  // 항목 추가하고 인덱스를 받아옴
        m_List_Log.SetTopIndex(nIndex);  // 추가된 항목으로 스크롤 이동
        delete pStr;

        // 가로 스크롤바 설정 업데이트
        Common::GetInstance()->CreateHorizontalScroll();
    }
    return 0;
}

void CMDS_Ebus_SampleDlg::InitializePdh()
{
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &hQuery);
    if (status != ERROR_SUCCESS) return;

    status = PdhAddCounter(hQuery, L"\\Processor(_Total)\\% Processor Time", 0, &hCounterTotal);
    if (status != ERROR_SUCCESS) {
        PdhCloseQuery(hQuery);
        hQuery = NULL;
        return;
    }

    PdhCollectQueryData(hQuery); // Initial collect
}