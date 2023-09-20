#include "stdafx.h"
#include "SkinButton.h"
#include "MDS_E-bus_SampleDlg.h"
CSkinButton::CSkinButton()
{


	m_clrBackground = Gdiplus::Color(255, 64, 64, 64);
	m_clrBorder = Gdiplus::Color(255, 128, 128, 128);
	m_clrText = Gdiplus::Color(255, 255, 255, 255);
	m_strText = _T("Enabled");
	m_fSizeText = 14;
	mbBoldFont = false;
}


CSkinButton::~CSkinButton()
{

}


// =============================================================================
void CSkinButton::RandomizeColors()
{
	// 0~255 범위의 랜덤한 색상을 생성
	int a = rand() % 256;
	int r = rand() % 256;
	int g = rand() % 256;
	int b = rand() % 256;

	// 랜덤한 색상으로 배경색과 텍스트 색상 설정
	ColorType backgroundColor(r, g, b, a);  // 이 부분은 ColorType의 정의에 따라 다를 수 있습니다.
	SetColorBackground(backgroundColor);

	ColorType textColor(255 - r, 255 - g, 255 - b, a); // 대조색으로 텍스트 색상 설정
	SetColorText(textColor);
}

// =============================================================================
void CSkinButton::SetColorBackground(const ColorType& color)
{
	m_clrBackground = color;
}

// =============================================================================
void CSkinButton::SetColorBorder(const ColorType& color)
{
	m_clrBorder = color;
}

// =============================================================================
void CSkinButton::SetColorText(const ColorType& color)
{
	m_clrText = color;
}

// =============================================================================
void CSkinButton::SetSizeText(float size)
{
	m_fSizeText = size;
}

// =============================================================================
void CSkinButton::DrawBackground(Graphics *pG)
{
	CRect rect;
	GetClientRect(&rect);

	Gdiplus::SolidBrush brush(m_clrBackground);

	pG->FillRectangle(&brush, rect.left, rect.top, rect.right, rect.bottom);
}

// =============================================================================
void CSkinButton::DrawBorder(Graphics *pG)
{
	CRect rect;
	GetClientRect(&rect);

	Gdiplus::Pen pen(m_clrBorder, 1);
	pG->DrawRectangle(&pen, rect.left, rect.top, rect.Width(), rect.Height());
}

// =============================================================================
void CSkinButton::SetFontStyle(Gdiplus::FontStyle nType, bool bFlag)
{
	Gdiplus::FontFamily fontfam(_T("Arial"));
	Gdiplus::Font font(&fontfam, m_fSizeText, (Gdiplus::FontStyle)nType, Gdiplus::UnitPixel);
	mbBoldFont = bFlag;

}

// =============================================================================
void CSkinButton::DrawText(Graphics *pG)
{
	CRect rect;
	GetClientRect(&rect);

	Gdiplus::FontFamily fontfam(_T("Arial"));
	Gdiplus::FontStyle nType;
	if (mbBoldFont)
	{
		nType = Gdiplus::FontStyleBold;
	}
	else
	{
		nType = Gdiplus::FontStyleRegular;
	}

	Gdiplus::Font font(&fontfam, m_fSizeText, nType, Gdiplus::UnitPixel);

	Gdiplus::StringFormat stringAlign;
	stringAlign.SetAlignment(Gdiplus::StringAlignmentCenter);
	stringAlign.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	Gdiplus::SolidBrush brush(m_clrText);

	pG->DrawString(m_strText, m_strText.GetLength(), &font, 
		Gdiplus::RectF((float)rect.left, (float)rect.top, (float)rect.Width(), (float)rect.Height()), &stringAlign, &brush);
}

// =============================================================================
void CSkinButton::SetText(CString strText)
{
	m_strText = strText;

	UpdateData(true);
}

BEGIN_MESSAGE_MAP(CSkinButton, CButton)
	ON_WM_PAINT()
END_MESSAGE_MAP()

// =============================================================================
void CSkinButton::OnPaint()
{
	CPaintDC dc(this);

	CRect rect;
	GetClientRect(&rect);

	Graphics mainG(dc.GetSafeHdc());

	Bitmap bmp(rect.Width(), rect.Height());

	Graphics memG(&bmp);

	SolidBrush brush(m_clrBackground);
	memG.FillRectangle(&brush, 0, 0, rect.Width(), rect.Height());

	DrawBackground(&memG);
	DrawBorder(&memG);
	DrawText(&memG);

	mainG.DrawImage(&bmp, 0, 0);
}
