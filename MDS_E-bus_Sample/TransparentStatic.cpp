#include "TransparentStatic.h"

BEGIN_MESSAGE_MAP(CTransparentStatic, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CTransparentStatic::SetText(const CString& text)
{
    m_text = text;
    Invalidate();  // 컨트롤을 다시 그리도록 설정
}

void CTransparentStatic:: GetText(CString& text) const 
{
    text = m_text;
}

void CTransparentStatic::OnPaint()
{
    CPaintDC paintDC(this); // device context for painting
    CRect rect;
    GetClientRect(&rect);

    CDC memDC; // Create memory DC
    memDC.CreateCompatibleDC(&paintDC);

    CBitmap memBitmap; // Create a bitmap for the memory DC to use
    memBitmap.CreateCompatibleBitmap(&paintDC, rect.Width(), rect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);

    memDC.SetBkMode(TRANSPARENT);
    memDC.FillSolidRect(&rect, RGB(0, 0, 0)); // 배경을 검은색으로 채움

   // 올바른 폰트 설정
    CFont* pOldFont = memDC.SelectObject(this->GetFont());
    memDC.SetTextColor(RGB(51, 255, 51)); // 텍스트를 밝은 녹색으로 설정

    // Draw the text
    memDC.DrawText(m_text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Copy from memory DC to screen DC
    paintDC.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);

    // Clean up
    memDC.SelectObject(pOldBitmap);
    memDC.SelectObject(pOldFont);
}

void CTransparentStatic::SetBackgroundColor(COLORREF color)
{
    m_backgroundColor = color;
    Invalidate(); // 컨트롤을 다시 그리도록 설정
}

void CTransparentStatic::SetTextColor(COLORREF color)
{
    m_textColor = color;
    Invalidate(); // 컨트롤을 다시 그리도록 요청
}

void CTransparentStatic::SetFont(int nHeight, BOOL bBold)
{
    // 폰트가 이미 생성되어 있다면 먼저 삭제합니다.
    m_font.DeleteObject();

    BOOL bCreateFontResult = m_font.CreateFont(
        nHeight,                 // nHeight
        0,                  // nWidth
        0,                  // nEscapement
        0,                  // nOrientation
        bBold ? FW_BOLD : FW_NORMAL,            // nWeight
        FALSE,              // bItalic
        FALSE,              // bUnderline
        0,                  // cStrikeOut
        ANSI_CHARSET,       // nCharSet
        OUT_DEFAULT_PRECIS, // nOutPrecision
        CLIP_DEFAULT_PRECIS,// nClipPrecision
        DEFAULT_QUALITY,    // nQuality
        DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
        _T("Arial"));       // lpszFacename

    if (!bCreateFontResult)
    {
        // 폰트 생성 실패 시, 오류 처리 코드를 여기에 작성
        return;
    }

    // 새로 생성한 폰트를 컨트롤에 적용합니다.
    CStatic::SetFont(&m_font); // 부모 클래스의 SetFont를 호출

    // 컨트롤을 다시 그리도록 설정합니다.
    Invalidate();
}