﻿#include "stdafx.h"
#include "DrawCommon.h"
#pragma comment(lib, "d2d1.lib")

CDrawCommon::CDrawCommon()
{
}

CDrawCommon::~CDrawCommon()
{
}

void CDrawCommon::Create(CDC* pDC, CWnd* pMainWnd)
{
    m_pDC = pDC;
    m_pMainWnd = pMainWnd;
    if (pMainWnd != nullptr)
        m_pfont = m_pMainWnd->GetFont();
}

void CDrawCommon::SetFont(CFont* pfont)
{
    m_pfont = pfont;
    m_pDC->SelectObject(m_pfont);
}

void CDrawCommon::SetDC(CDC* pDC)
{
    m_pDC = pDC;
}

UINT DrawCommonHelper::ProccessTextFormat(CRect rect, CSize text_length, Alignment align, bool multi_line) noexcept
{
    UINT result; // CDC::DrawText()函数的文本格式
    if (multi_line)
        result = DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX;
    else
        result = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;

    if (text_length.cx > rect.Width()) //如果文本宽度超过了矩形区域的宽度，设置了居中时左对齐
    {
        if (align == Alignment::RIGHT)
            result |= DT_RIGHT;
    }
    else
    {
        switch (align)
        {
        case Alignment::RIGHT:
            result |= DT_RIGHT;
            break;
        case Alignment::CENTER:
            result |= DT_CENTER;
            break;
        }
    }
    return result;
}

void CDrawCommon::DrawWindowText(CRect rect, LPCTSTR lpszString, COLORREF color, Alignment align, bool draw_back_ground, bool multi_line)
{
    m_pDC->SetTextColor(color);
    if (!draw_back_ground)
        m_pDC->SetBkMode(TRANSPARENT);
    m_pDC->SelectObject(m_pfont);
    CSize text_size = m_pDC->GetTextExtent(lpszString);

    auto format = DrawCommonHelper::ProccessTextFormat(rect, text_size, align, multi_line);

    if (draw_back_ground)
        m_pDC->FillSolidRect(rect, m_back_color);
    m_pDC->DrawText(lpszString, rect, format);
}


void CDrawCommon::SetDrawRect(CRect rect)
{
    CRgn rgn;
    rgn.CreateRectRgnIndirect(rect);
    m_pDC->SelectClipRgn(&rgn);
}

void CDrawCommon::SetDrawRect(CDC* pDC, CRect rect)
{
    CRgn rgn;
    rgn.CreateRectRgnIndirect(rect);
    pDC->SelectClipRgn(&rgn);
}

void CDrawCommon::DrawBitmap(CBitmap& bitmap, CPoint start_point, CSize size, StretchMode stretch_mode)
{
    CDC memDC;

    //获取图像实际大小
    BITMAP bm;
    GetObject(bitmap, sizeof(BITMAP), &bm);

    memDC.CreateCompatibleDC(m_pDC);
    memDC.SelectObject(&bitmap);
    // 以下两行避免图片失真
    m_pDC->SetStretchBltMode(HALFTONE);
    m_pDC->SetBrushOrg(0, 0);
    CSize draw_size;
    if (size.cx == 0 || size.cy == 0)       //如果指定的size为0，则使用位图的实际大小绘制
    {
        draw_size = CSize(bm.bmWidth, bm.bmHeight);
    }
    else
    {
        draw_size = size;
        if (stretch_mode == StretchMode::FILL)
        {
            SetDrawRect(CRect(start_point, draw_size));
            float w_h_radio, w_h_radio_draw;        //图像的宽高比、绘制大小的宽高比
            w_h_radio = static_cast<float>(bm.bmWidth) / bm.bmHeight;
            w_h_radio_draw = static_cast<float>(size.cx) / size.cy;
            if (w_h_radio > w_h_radio_draw)     //如果图像的宽高比大于绘制区域的宽高比，则需要裁剪两边的图像
            {
                int image_width;        //按比例缩放后的宽度
                image_width = bm.bmWidth * draw_size.cy / bm.bmHeight;
                start_point.x -= ((image_width - draw_size.cx) / 2);
                draw_size.cx = image_width;
            }
            else
            {
                int image_height;       //按比例缩放后的高度
                image_height = bm.bmHeight * draw_size.cx / bm.bmWidth;
                start_point.y -= ((image_height - draw_size.cy) / 2);
                draw_size.cy = image_height;
            }
        }
        else if (stretch_mode == StretchMode::FIT)
        {
            draw_size = CSize(bm.bmWidth, bm.bmHeight);
            float w_h_radio, w_h_radio_draw;        //图像的宽高比、绘制大小的宽高比
            w_h_radio = static_cast<float>(bm.bmWidth) / bm.bmHeight;
            w_h_radio_draw = static_cast<float>(size.cx) / size.cy;
            if (w_h_radio > w_h_radio_draw)     //如果图像的宽高比大于绘制区域的宽高比
            {
                draw_size.cy = draw_size.cy * size.cx / draw_size.cx;
                draw_size.cx = size.cx;
                start_point.y += ((size.cy - draw_size.cy) / 2);
            }
            else
            {
                draw_size.cx = draw_size.cx * size.cy / draw_size.cy;
                draw_size.cy = size.cy;
                start_point.x += ((size.cx - draw_size.cx) / 2);
            }
        }
    }

    m_pDC->StretchBlt(start_point.x, start_point.y, draw_size.cx, draw_size.cy, &memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    memDC.DeleteDC();
}

void CDrawCommon::DrawBitmap(UINT bitmap_id, CPoint start_point, CSize size, StretchMode stretch_mode)
{
    CBitmap bitmap;
    bitmap.LoadBitmap(bitmap_id);
    DrawBitmap(bitmap, start_point, size, stretch_mode);
}

void CDrawCommon::DrawBitmap(HBITMAP hbitmap, CPoint start_point, CSize size, StretchMode stretch_mode)
{
    CBitmap bitmap;
    if (!bitmap.Attach(hbitmap))
        return;
    DrawBitmap(bitmap, start_point, size, stretch_mode);
    bitmap.Detach();
}

void CDrawCommon::DrawIcon(HICON hIcon, CPoint start_point, CSize size)
{
    if (m_pDC->GetSafeHdc() == NULL)
        return;
    if (size.cx == 0 || size.cy == 0)
        ::DrawIconEx(m_pDC->GetSafeHdc(), start_point.x, start_point.y, hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_DEFAULTSIZE);
    else
        ::DrawIconEx(m_pDC->GetSafeHdc(), start_point.x, start_point.y, hIcon, size.cx, size.cy, 0, NULL, DI_NORMAL);
}

void CDrawCommon::BitmapStretch(CImage* pImage, CImage* ResultImage, CSize size)
{
    if (pImage->IsDIBSection())
    {
        // 取得 pImage 的 DC
        CDC* pImageDC1 = CDC::FromHandle(pImage->GetDC()); // Image 因為有自己的 DC, 所以必須使用 FromHandle 取得對應的 DC

        CBitmap* bitmap1 = pImageDC1->GetCurrentBitmap();
        BITMAP bmpInfo;
        bitmap1->GetBitmap(&bmpInfo);

        // 建立新的 CImage
        ResultImage->Create(size.cx, size.cy, bmpInfo.bmBitsPixel);
        CDC* ResultImageDC = CDC::FromHandle(ResultImage->GetDC());

        // 當 Destination 比較小的時候, 會根據 Destination DC 上的 Stretch Blt mode 決定是否要保留被刪除點的資訊
        ResultImageDC->SetStretchBltMode(HALFTONE); // 使用最高品質的方式
        ::SetBrushOrgEx(ResultImageDC->m_hDC, 0, 0, NULL); // 調整 Brush 的起點

        // 把 pImage 畫到 ResultImage 上面
        StretchBlt(*ResultImageDC, 0, 0, size.cx, size.cy, *pImageDC1, 0, 0, pImage->GetWidth(), pImage->GetHeight(), SRCCOPY);
        // pImage->Draw(*ResultImageDC,0,0,StretchWidth,StretchHeight,0,0,pImage->GetWidth(),pImage->GetHeight());

        pImage->ReleaseDC();
        ResultImage->ReleaseDC();
    }
}

void CDrawCommon::FillRect(CRect rect, COLORREF color)
{
    m_pDC->FillSolidRect(rect, color);
}

void CDrawCommon::FillRectWithBackColor(CRect rect)
{
    m_pDC->FillSolidRect(rect, m_back_color);
}

void CDrawCommon::DrawRectOutLine(CRect rect, COLORREF color, int width, bool dot_line)
{
    CPen aPen, * pOldPen;
    aPen.CreatePen((dot_line ? PS_DOT : PS_SOLID), width, color);
    pOldPen = m_pDC->SelectObject(&aPen);
    CBrush* pOldBrush{ dynamic_cast<CBrush*>(m_pDC->SelectStockObject(NULL_BRUSH)) };

    rect.DeflateRect(width / 2, width / 2);
    m_pDC->Rectangle(rect);
    m_pDC->SelectObject(pOldPen);
    m_pDC->SelectObject(pOldBrush);       // Restore the old brush
    aPen.DeleteObject();
}

void CDrawCommon::GetRegionFromImage(CRgn& rgn, CBitmap& cBitmap, int threshold)
{
    CDC memDC;

    memDC.CreateCompatibleDC(NULL);
    CBitmap* pOldMemBmp = NULL;
    pOldMemBmp = memDC.SelectObject(&cBitmap);

    //创建总的窗体区域，初始region为0
    rgn.CreateRectRgn(0, 0, 0, 0);

    BITMAP bit;
    cBitmap.GetBitmap(&bit);//取得位图参数，这里要用到位图的长和宽
    int y;
    for (y = 0; y < bit.bmHeight; y++)
    {
        CRgn rgnTemp; //保存临时region
        int iX = 0;
        do
        {
            //跳过透明色找到下一个非透明色的点.
            while (iX < bit.bmWidth && GetColorBritness(memDC.GetPixel(iX, y)) <= threshold)
                iX++;
            int iLeftX = iX; //记住这个起始点

                             //寻找下个透明色的点
            while (iX < bit.bmWidth && GetColorBritness(memDC.GetPixel(iX, y)) > threshold)
                ++iX;

            //创建一个包含起点与重点间高为1像素的临时“region”
            rgnTemp.CreateRectRgn(iLeftX, y, iX, y + 1);
            rgn.CombineRgn(&rgn, &rgnTemp, RGN_OR);

            //删除临时"region",否则下次创建时和出错
            rgnTemp.DeleteObject();
        } while (iX < bit.bmWidth);
    }
    memDC.DeleteDC();
}

int CDrawCommon::GetColorBritness(COLORREF color)
{
    return (GetRValue(color) + GetGValue(color) + GetBValue(color)) / 3;
}

void CDrawCommon::DrawLine(CPoint start_point, int height, COLORREF color)
{
    CPen aPen, * pOldPen;
    aPen.CreatePen(PS_SOLID, 1, color);
    pOldPen = m_pDC->SelectObject(&aPen);
    CBrush* pOldBrush{ dynamic_cast<CBrush*>(m_pDC->SelectStockObject(NULL_BRUSH)) };

    m_pDC->MoveTo(start_point); //移动到起始点，默认是从下向上画
    m_pDC->LineTo(CPoint(start_point.x, start_point.y - height));
    m_pDC->SelectObject(pOldPen);
    m_pDC->SelectObject(pOldBrush);       // Restore the old brush
    aPen.DeleteObject();
}

UINT DrawCommonHelper::ProccessTextFormat(CRect rect, CSize text_length, Alignment align, bool multi_line) noexcept
{
    UINT result; // CDC::DrawText()函数的文本格式
    if (multi_line)
        result = DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX;
    else
        result = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;

    if (text_length.cx > rect.Width()) //如果文本宽度超过了矩形区域的宽度，设置了居中时左对齐
    {
        if (align == Alignment::RIGHT)
            result |= DT_RIGHT;
    }
    else
    {
        switch (align)
        {
        case Alignment::RIGHT:
            result |= DT_RIGHT;
            break;
        case Alignment::CENTER:
            result |= DT_CENTER;
            break;
        }
    }
    return result;
}

auto DrawCommonHelper::GetArgb32BitmapInfo(CRect rect) noexcept
    -> ::BITMAPINFO
{
    LONG width = std::abs(rect.Width());
    LONG height = std::abs(rect.Height());
    return GetArgb32BitmapInfo(width, height);
}

auto DrawCommonHelper::GetArgb32BitmapInfo(LONG width, LONG height) noexcept
    -> ::BITMAPINFO
{
    BITMAPINFO result;
    memset(&result, 0, sizeof(BITMAPINFO));
    result.bmiHeader.biSize = sizeof(result.bmiHeader);
    //保证是自上而下
    result.bmiHeader.biWidth = -static_cast<LONG>(width);
    result.bmiHeader.biHeight = -static_cast<LONG>(height);
    result.bmiHeader.biPlanes = 1;
    result.bmiHeader.biBitCount = 32;
    result.bmiHeader.biCompression = BI_RGB;
    return result;
}

SizeWrapper::SizeWrapper(LONG width = 0, LONG height = 0)
{
    SetWidth(width);
    SetHeight(height);
}

SizeWrapper::SizeWrapper(SIZE size)
    : m_content{size} {}

SIZE* SizeWrapper::GetSizePointer()
{
    return &m_content;
}

LONG SizeWrapper::GetWidth() const noexcept
{
    return m_content.cx;
}
LONG SizeWrapper::GetHeight() const noexcept
{
    return m_content.cy;
}
void SizeWrapper::SetWidth(LONG width) noexcept
{
    m_content.cx = width;
}
void SizeWrapper::SetHeight(LONG height) noexcept
{
    m_content.cy = height;
}

bool D2D1DrawCommon::CheckSupport()
{
    bool result = false;
    auto d2d1_hmodule = ::LoadLibrary(_T("D2d1.dll"));
    if (d2d1_hmodule)
    {
        ::FreeLibrary(d2d1_hmodule);
    }
    return result;
}

DrawCommonBuffer::DrawCommonBuffer(HWND hwnd, CRect rect)
    : m_update_window_info{0}, m_target_hwnd{hwnd}
{
    m_size.SetWidth(std::abs(rect.Width()));
    m_size.SetHeight(std::abs(rect.Height()));

    BITMAPINFO bitmap_info = DrawCommonHelper::GetArgb32BitmapInfo(rect);
    {
        auto pp_bitmap_for_show_data = reinterpret_cast<void**>(&m_p_display_bitmap);
        m_mem_display_dc.CreateCompatibleDC(NULL);
        m_display_hbitmap = ::CreateDIBSection(m_mem_display_dc, &bitmap_info, DIB_RGB_COLORS, pp_bitmap_for_show_data, NULL, 0);
        m_old_display_bitmap = m_mem_display_dc.SelectObject(m_display_hbitmap);
    }

    m_update_window_info.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
    // m_update_window_info.hdcSrc = NULL;
    // m_update_window_info.pptDst = NULL;
    // m_update_window_info.psize = NULL;
    m_update_window_info.hdcSrc = m_mem_display_dc;
    // m_update_window_info.pptSrc = 在析构函数中填写;
    // m_update_window_info.crKey = 0;
    m_update_window_info.pblend = GetDefaultBlendFunctionPointer();
    m_update_window_info.dwFlags = ULW_ALPHA;
    // m_update_window_info.prcDirty = NULL;
}

DrawCommonBuffer::~DrawCommonBuffer()
{
    POINT start_location{0, 0};
    m_update_window_info.pptSrc = &start_location;
    ::UpdateLayeredWindowIndirect(m_target_hwnd, &m_update_window_info);

    m_mem_display_dc.SelectObject(m_old_display_bitmap);
    ::DeleteObject(m_display_hbitmap);
    m_mem_display_dc.DeleteDC();
}

BYTE* DrawCommonBuffer::GetData()
{
    return m_p_display_bitmap;
}

auto DrawCommonBuffer::GetDefaultBlendFunctionPointer()
    -> const ::PBLENDFUNCTION
{
    static ::BLENDFUNCTION result{
        AC_SRC_OVER,
        0,
        0xFF,
        AC_SRC_ALPHA};
    return &result;
}

HDC DrawCommonBuffer::GetDC()
{
    return m_mem_display_dc;
}