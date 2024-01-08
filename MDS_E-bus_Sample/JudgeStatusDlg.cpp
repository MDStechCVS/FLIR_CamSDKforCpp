// JudgeStatusDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "MDS_E-bus_Sample.h"
#include "JudgeStatusDlg.h"
#include "afxdialogex.h"


// JudgeStatusDlg ��ȭ �����Դϴ�.

IMPLEMENT_DYNAMIC(JudgeStatusDlg, CDialogEx)

JudgeStatusDlg::JudgeStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DLG_JUDGE, pParent)
{

}

JudgeStatusDlg::~JudgeStatusDlg()
{
}

void JudgeStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(JudgeStatusDlg, CDialogEx)
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()


// JudgeStatusDlg �޽��� ó�����Դϴ�.


BOOL JudgeStatusDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���̾�α� ũ�⸦ �����ɴϴ�
	CRect dlgRect;
	GetWindowRect(dlgRect);

	// ��ũ�� ũ�⸦ �����ɴϴ�
	CRect screenRect;
	GetDesktopWindow()->GetWindowRect(screenRect);


	GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("initialization...."));

	// ���̾�α׸� ��ũ�� ����� �̵���ŵ�ϴ�
	int xPos = (screenRect.Width() - dlgRect.Width()) / 2;
	int yPos = (screenRect.Height() - dlgRect.Height()) / 2;
	SetWindowPos(NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	// TODO:  ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	m_font.CreateFont(150, // nHeight
		100, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_EXTRABOLD, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		CLIP_DEFAULT_PRECIS,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		_T("Calibri")); // lpszFacename

	GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetFont(&m_font);



	m_brush_Devstatus.DeleteObject();
	m_brush_Devstatus.CreateSolidBrush(RGB(0, 255, 0));
	_textColor_Devstatus = RGB(255, 255, 255);

	m_bCancelled = FALSE;


	return TRUE;  // return TRUE unless you set the focus to a control
				  // ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}


BOOL JudgeStatusDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	if (WM_KEYDOWN == pMsg->message)
	{
		OnCancel();
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

//void JudgeStatusDlg::ShowJudge(JUDGESTATUS Judge)
//{
//	switch (Judge)
//	{
//	case JUDGE_PASS_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(0, 255, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[0].szJudge));
//		break;
//	case JUDGE_IMAGE_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[1].szJudge));
//		break;
//	case JUDGE_DEFOCUS_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[2].szJudge));
//		break;
//	case JUDGE_BD_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[3].szJudge));
//		break;
//	case JUDGE_SP_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[4].szJudge));
//		break;
//	case JUDGE_BDSP_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[5].szJudge));
//		break;
//	case JUDGE_OTP_WRITE_:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[6].szJudge));
//		break;
//	case JUDGE_DEFOCUS_2:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("JUDGE_DEFOCUS_2"));
//		break;
//	case JUDGE_BD_2:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("JUDGE_BD_2"));
//		break;
//	case JUDGE_SP_2:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("JUDGE_SP_2"));
//		break;
//	case JUDGE_BDSP_2:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("JUDGE_BDSP_2"));
//		break;
//	case STAND_BY_:
//		break;
//	case DATABASE_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("DB FAIL"));
//		break;
//	case FLAG_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("FLAG FAIL"));
//		break;
//		
//	case DUMP_FILE_WRITE_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("WRITE FAIL"));
//		break;
//
//	case EEPROM_CHECK_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("DATA FAIL"));
//		break;
//	case SENSORID_CHECK_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("SENSORID FAIL"));
//		break;
//	case HEADER_CRC_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("HEADER CRC FAIL"));
//		break;
//	case CAL_CRC_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("CAL CRC FAIL"));
//		break;
//	case FACTORY_CRC_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("FACTORY CRC FAIL"));
//		break;
//	case HEADER_CHECK_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("HEADER AREA FAIL"));
//		break;
//	case CAL_CHECK_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("CAL AREA FAIL"));
//		break;
//	case FACTORY_CHECK_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("FACTORY AREA FAIL"));
//		break;
//	case CHECKSUM_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("CHECKSUM FAIL"));
//		break;
//	case EEPROM_SIZE_FAIL:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(255, 0, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("SIZE FAIL"));
//		break;
//
//	case EEPROM_ALL_PASS:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(0, 255, 0));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(Common::GetInstance()->str2CString(JUDGE_CODE_DATA[0].szJudge));
//		break;
//
//	case IMAGE_SAVE:
//		m_brush_Devstatus.DeleteObject();
//		m_brush_Devstatus.CreateSolidBrush(RGB(146, 129, 205));
//		_textColor_Devstatus = RGB(255, 255, 255);
//		GetDlgItem(IDC_STATIC_DLG_JUDGE)->SetWindowTextW(_T("IMAGE SAVE"));
//		break;
//
//	default:
//	
//		break;
//	}
//}

HBRUSH JudgeStatusDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  ���⼭ DC�� Ư���� �����մϴ�.
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		if (pWnd->GetDlgCtrlID() == IDC_STATIC_DLG_JUDGE)
		{
			CRect rc;
			GetDlgItem(IDC_STATIC_DLG_JUDGE)->GetWindowRect(rc);
			ScreenToClient(rc);
			pDC->SetBkMode(TRANSPARENT);
			pDC->FillRect(rc, &m_brush_Devstatus);

			pDC->SetTextColor(_textColor_Devstatus);
			return m_brush_Devstatus;
		}
	}

	// TODO:  �⺻���� �������� ������ �ٸ� �귯�ø� ��ȯ�մϴ�.
	return hbr;
}


void JudgeStatusDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	OnCancel();
	CDialogEx::OnLButtonDown(nFlags, point);
}


void JudgeStatusDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	OnCancel();
	CDialogEx::OnRButtonDown(nFlags, point);
}

void JudgeStatusDlg::CloseDialog()
{

}

bool JudgeStatusDlg::isCancleStatus()
{
	return m_bCancelled;
}

void JudgeStatusDlg::OnCancel()
{
	m_bCancelled = TRUE;

	CDialogEx::OnCancel();
}





