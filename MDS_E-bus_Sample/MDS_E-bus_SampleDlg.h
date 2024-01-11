
// MDS_Ebus_SampleDlg.h: 헤더 파일
//

#pragma once

#include "global.h"
#include "JudgeStatusDlg.h"

class CameraManager;
class CameraControl_rev;
class CTransparentStatic;


// CMDS_Ebus_SampleDlg 대화 상자
class CMDS_Ebus_SampleDlg : public CDialogEx
{
// 생성입니다.
public:
	CMDS_Ebus_SampleDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MDSTESTPGM_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:

	struct CameraInfoLabels
	{
		CTransparentStatic& lbFps;           // FPS를 표시할 레이블
		CTransparentStatic& lbMin;           // 최소 온도 값을 표시할 레이블
		CTransparentStatic& lbMax;           // 최대 온도 값을 표시할 레이블
		CTransparentStatic& lbROI;           // 관심 영역(ROI) 정보를 표시할 레이블
		CStatic& lbConnectStatus;			 // 카메라 연결 상태를 표시할 레이블
		CStatic& lbRecordingStatus;			 // 녹화 상태를 표시할 레이블

		CameraInfoLabels(CTransparentStatic& fps, CTransparentStatic& min, CTransparentStatic& max, CTransparentStatic& roi, CStatic& connectStatus, CStatic& recordingStatus)
			: lbFps(fps), lbMin(min), lbMax(max), lbROI(roi), lbConnectStatus(connectStatus), lbRecordingStatus(recordingStatus) {}
	};
	std::mutex Guimtx; // 뮤텍스 객체 생성

	cv::Point m_StartPos;
	cv::Point m_EndPos;
	CListBox m_List_Log;
	CStatic m_Cam1;
	CStatic m_Cam2;
	CStatic m_Cam3;
	CStatic m_Cam4;

	CSkinButton m_BtnDeviceCtrl;
	CSkinButton m_BtnCommunicationCtrl;
	CSkinButton m_BtnImageStreamCtrl;
	CSkinButton m_BtnStart;
	CSkinButton m_BtnStop;
	//CSkinButton m_BtnConnect;
	CSkinButton m_BtnDisconnect;
	CSkinButton m_BtnFPS30;
	CSkinButton m_BtnFPS60;
	CSkinButton m_BtnDeviceFind;
	CSkinButton m_BtnLoadiniFile;
	CSkinButton m_BtniniApply;
	CSkinButton m_BtnCamParam;
	CSkinButton m_BtnCamParamApply;
	CSkinButton m_BtnDataFolder;
	CSkinButton m_BtnImageSnap;
	CSkinButton m_BtnImageRecording;

	CStatic m_logo;
	CTransparentStatic m_LbCamCount;

	CTransparentStatic m_LbCam1fps;
	CTransparentStatic m_LbCam1min;
	CTransparentStatic m_LbCam1max;
	CTransparentStatic m_LbCam1ROI;
	
	CTransparentStatic m_LbCam2fps;
	CTransparentStatic m_LbCam2min;
	CTransparentStatic m_LbCam2max;
	CTransparentStatic m_LbCam2ROI;
	
	CTransparentStatic m_LbCam3fps;
	CTransparentStatic m_LbCam3min;
	CTransparentStatic m_LbCam3max;
	CTransparentStatic m_LbCam3ROI;
	
	CTransparentStatic m_LbCam4fps;
	CTransparentStatic m_LbCam4min;
	CTransparentStatic m_LbCam4max;
	CTransparentStatic m_LbCam4ROI;

	CStatic m_LbCam1ConnectStatus;
	CStatic m_LbCam2ConnectStatus;
	CStatic m_LbCam3ConnectStatus;
	CStatic m_LbCam4ConnectStatus;

	CStatic m_LbCam1RecordingStatus;
	CStatic m_LbCam2RecordingStatus;
	CStatic m_LbCam3RecordingStatus;
	CStatic m_LbCam4RecordingStatus;

	CBrush* m_brush;
	CBrush* m_brush2;
	CBrush m_bRed;
	CBrush m_bGreen;
	CBrush m_bYellow;
	COLORREF m_Color1, m_Color2, m_Color3;
	CFont m_basefont, m_Btnfont, m_StaticFont;

	CComboBox m_Cam1_Colormap;
	CComboBox m_Cam2_Colormap;
	CComboBox m_Cam3_Colormap;
	CComboBox m_Cam4_Colormap;

public:
	//공용변수
	CButton m_chGenICam_checkBox;
	CButton m_chEventsCheckBox;
	CButton m_chPointerCheckBox;
	CButton m_chColorMapCheckBox;
	CButton m_chUYVYCheckBox;
	CButton m_chMonoCheckBox;

	CButton m_chMarkerCheckBox;
	CButton m_ch_Cam1_ROI_CheckBox;
	CProgressCtrl m_progress;
	CButton m_radio;
	CEdit m_Color_Scale;
	CEdit m_Color_Scale2;
	CEdit m_Color_Scale3;

private:
	bool m_bGUITimerActive;
	int m_nSelectCamIndex;
	bool m_blink[CAMERA_COUNT];
	CameraInfoLabels m_LbCamInfo[CAMERA_COUNT];

public:

	bool m_Cam_selecting_roi;
	GUI_STATUS gui_status;
	BOOL m_NeedInit;
	PvGenBrowserWnd* m_DeviceWnd;
	PvGenBrowserWnd* m_CommunicationWnd;
	PvGenBrowserWnd* m_StreamParametersWnd;
	CameraManager* m_CamManager;

public:
	void ShowGenWindow(PvGenBrowserWnd** aWnd, PvGenParameterArray* aParams, const CString& aTitle);
	void CloseGenWindow(PvGenBrowserWnd** aWnd);
	void Btn_Interface_setting();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void DlgEnabledWindows();
	bool LoadiniFile();
	void CameraParamsFileOpen();
	void InitSystemParam();
	void UpdateButtonStyle(CSkinButton* pbutton, const CPoint& cursorPos);
	cv::Rect mapRectToImage(const cv::Rect& rect, const cv::Size& imageSize, const cv::Size& dialogSize);
	void UpdateCameraROI(CStatic* displayControl, const CRect& controlRect, CameraManager* camManager,
		int cameraIndex, bool& selectingROI, cv::Point& startPos, cv::Point& endPos, UINT message);
	void Interface_state(bool bSeleted = false);
	bool SetBtnEnabled(bool bFlag, CSkinButton* Btn);
	void SetSelectCamIndex(int nIndex);
	int GetSelectCamIndex();
	HBRUSH SetCameraFlagStatus(int camIndex, CameraManager* camManager, bool bFlag, CDC* pDC, HBRUSH greenBrush, HBRUSH redBrush);
	HBRUSH HandleCameraRecordingStatus(int camIndex, CDC* pDC);
	
	PaletteTypes GetSelectedColormap(CComboBox& comboControl);
	void ApplyColorSettings(PaletteTypes selectedMap, int comboIndex);
	void PopulateComboBoxes();
	void HandleComboChange(int controlId);
	bool IsMouseEventCheck(UINT message);
	void ShowJudgeDlg();
	void CloseJudgeDlg();
	void UpdateCheckBoxes(int camIndex);
	void RedrawComboBox(int controlId);

	void UpdateCameraInfo(CameraControl_rev* cam, CameraInfoLabels& labels);
	void UpdateLabel(double value, CTransparentStatic& label);
	void UpdateLabel(int value, CTransparentStatic& label);
	void UpdateROIInfo(CameraControl_rev* cam, CTransparentStatic& label);
	void InvalidateLabels(CameraInfoLabels& labels);
	void CameraAutoStart(int deviceCount);
	void UpdateProgressValue();
	void SetTransparentStatic_ControlsFont();
	void SetControlsFont();

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnCommunicationCtrl();
	afx_msg void OnBnClickedDeviceCtrl();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnImgStream();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedBtnFps60();
	afx_msg void OnBnClickedBtnFps30();
	afx_msg void OnBnClickedCkParam();
	afx_msg void OnBnClickedBtnConnected();
	afx_msg void OnBnClickedCkPointer();
	afx_msg void OnBnClickedCkMarker();
	afx_msg void OnBnClickedBtnDeviceFind();
	afx_msg void RadioCtrl(UINT radio_Index);
	afx_msg void OnBnClickedBtnLoadIniFile();
	afx_msg void OnBnClickedBtnIniApply();
	afx_msg void OnCbnSelchangeCam1();
	afx_msg void OnCbnSelchangeCam2();
	afx_msg void OnCbnSelchangeCam3();
	afx_msg void OnCbnSelchangeCam4();
	afx_msg void OnBnClickedBtnCamParam();
	afx_msg void OnBnClickedBtnCamParamApply();
	afx_msg void OnBnClickedCheckBox();
	afx_msg void OnBnClickedBtnOpenDataFolder();
	afx_msg void OnStnClickedCam1Recording();
	afx_msg void OnBnClickedBtnImgSnap();
	afx_msg void OnBnClickedBtnImgRecording();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnCbnDropdownCbCam1Colormap();
};
