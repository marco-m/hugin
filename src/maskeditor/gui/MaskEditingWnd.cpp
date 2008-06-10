#include "huginapp/ImageCache.h"
#include "MaskEditingWnd.h"
#include "MaskEdClientWnd.h"

using HuginBase::ImageCache;

BEGIN_EVENT_TABLE(MaskEditingWnd, wxScrolledWindow)
    EVT_SCROLL(MaskEditingWnd::OnScrollEvt)
    EVT_SCROLL_CHANGED(MaskEditingWnd::OnScrollEvt)
    EVT_PAINT(MaskEditingWnd::OnPaint)
END_EVENT_TABLE()

MaskEditingWnd::MaskEditingWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name), m_bimg(0)
{
    
}

MaskEditingWnd::~MaskEditingWnd() 
{
    delete m_bimg;
}
void MaskEditingWnd::LoadImage(const wxString &filename)
{
    if(filename != wxT(""))
    {
        ImageCache::EntryPtr e = ImageCache::getInstance().getImage(std::string(filename.mb_str()));
        HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
        if (img) {
            if(m_bimg != NULL)
                delete m_bimg;
            m_bimg = new wxBitmap( wxImage(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true));
        } 
        SetScrollbars( 1, 1, m_bimg->GetWidth(), m_bimg->GetHeight());
    }

}
void MaskEditingWnd::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    int x,y;
    x = GetScrollPos(wxSB_HORIZONTAL);
    y = GetScrollPos(wxSB_VERTICAL);
    if(m_bimg)
      dc.DrawBitmap(*m_bimg, -x, -y);
    
}

void MaskEditingWnd::OnScrollEvt(wxScrollEvent &event)
{
    Refresh();
    event.Skip();
}

