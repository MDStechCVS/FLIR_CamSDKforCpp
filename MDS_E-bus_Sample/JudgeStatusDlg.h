#pragma once


// JudgeStatusDlg ��ȭ �����Դϴ�.

class JudgeStatusDlg : public CDialogEx
{
	DECLARE_DYNAMIC(JudgeStatusDlg)

public:
	JudgeStatusDlg(CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~JudgeStatusDlg();

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_JUDGE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	//JUDGESTATUS j;

	
private:
	CFont m_font;
	CBrush m_brush_Devstatus;
	COLORREF _textColor_Devstatus;
	bool m_bCancelled;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	void CloseDialog();
	void OnCancel();
	bool isCancleStatus();
	//void ShowJudge(JUDGESTATUS Judge);
};
