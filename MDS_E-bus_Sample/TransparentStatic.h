#pragma once
#include "global.h"

class CTransparentStatic : public CStatic
{
public:
    void SetText(const CString& text);
    void GetText(CString& text) const;
    void SetFont(int nHeight, BOOL bBold);
    void SetTextColor(COLORREF color);
    void SetBackgroundColor(COLORREF color); // ��� ���� ���� �Լ�
protected:
    CString m_text;
    COLORREF m_backgroundColor; // ��� ������ �����ϴ� ��� ����
    COLORREF m_textColor;
private:
    CFont m_font; // ��Ʈ ��ü�� �����ϴ� ��� ����
    // �׸��� �޽��� ó��
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()
};
