#include "TransparentStatic.h"

BEGIN_MESSAGE_MAP(CTransparentStatic, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CTransparentStatic::SetText(const CString& text)
{
    m_text = text;
    Invalidate();  // ��Ʈ���� �ٽ� �׸����� ����
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
    memDC.FillSolidRect(&rect, RGB(0, 0, 0)); // ����� ���������� ä��

   // �ùٸ� ��Ʈ ����
    CFont* pOldFont = memDC.SelectObject(this->GetFont());
    memDC.SetTextColor(RGB(51, 255, 51)); // �ؽ�Ʈ�� ���� ������� ����

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
    Invalidate(); // ��Ʈ���� �ٽ� �׸����� ����
}

void CTransparentStatic::SetTextColor(COLORREF color)
{
    m_textColor = color;
    Invalidate(); // ��Ʈ���� �ٽ� �׸����� ��û
}

void CTransparentStatic::SetFont(int nHeight, BOOL bBold)
{
    // ��Ʈ�� �̹� �����Ǿ� �ִٸ� ���� �����մϴ�.
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
        // ��Ʈ ���� ���� ��, ���� ó�� �ڵ带 ���⿡ �ۼ�
        return;
    }

    // ���� ������ ��Ʈ�� ��Ʈ�ѿ� �����մϴ�.
    CStatic::SetFont(&m_font); // �θ� Ŭ������ SetFont�� ȣ��

    // ��Ʈ���� �ٽ� �׸����� �����մϴ�.
    Invalidate();
}