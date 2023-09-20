#pragma once

typedef Gdiplus::Color ColorType;
const ColorType COLOR_HOVER(255, 255, 255, 255);
const ColorType COLOR_DEFAULT(255, 64, 64, 64);
const ColorType COLOR_TEXT_HOVER(255, 255, 0, 0);
const ColorType COLOR_TEXT_DEFAULT(255, 255, 255, 255);

class CSkinButton : public CButton
{
public:
	CSkinButton();
	~CSkinButton();

public :
	void SetColorBackground(const ColorType& color);
	void SetColorBorder(const ColorType& color);
	void SetColorText(const ColorType& color);
	void SetSizeText(float size);
	void SetText(CString strText);
	void SetFontStyle(Gdiplus::FontStyle nType, bool bFlag = false);
	void RandomizeColors();
private :
	Gdiplus::Color m_clrBackground; // 배경 색상 
	Gdiplus::Color m_clrBorder; // 외곽선 색상
	Gdiplus::Color m_clrText; // 텍스트 색상
	float m_fSizeText; // 텍스트 크기
	CString m_strText; // 문자열
	int mFontStyle;
	bool mbBoldFont;
	void DrawBackground(Gdiplus::Graphics *pG); // 배경 그리기
	void DrawBorder(Gdiplus::Graphics *pG); // 외곽선 그리기
	void DrawText(Gdiplus::Graphics *pG); // 텍스트 그리기
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
};

