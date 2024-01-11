#pragma once
#include "global.h"

class CTransparentStatic : public CStatic
{
public:
    void SetText(const CString& text);
    void GetText(CString& text) const;
    void SetFont(int nHeight, BOOL bBold);
    void SetTextColor(COLORREF color);
    void SetBackgroundColor(COLORREF color); // 배경 색상 설정 함수
protected:
    CString m_text;
    COLORREF m_backgroundColor; // 배경 색상을 저장하는 멤버 변수
    COLORREF m_textColor;
private:
    CFont m_font; // 폰트 객체를 저장하는 멤버 변수
    // 그리기 메시지 처리
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()
};
