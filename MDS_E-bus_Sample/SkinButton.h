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
	Gdiplus::Color m_clrBackground; // ��� ���� 
	Gdiplus::Color m_clrBorder; // �ܰ��� ����
	Gdiplus::Color m_clrText; // �ؽ�Ʈ ����
	float m_fSizeText; // �ؽ�Ʈ ũ��
	CString m_strText; // ���ڿ�
	int mFontStyle;
	bool mbBoldFont;
	void DrawBackground(Gdiplus::Graphics *pG); // ��� �׸���
	void DrawBorder(Gdiplus::Graphics *pG); // �ܰ��� �׸���
	void DrawText(Gdiplus::Graphics *pG); // �ؽ�Ʈ �׸���
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
};

