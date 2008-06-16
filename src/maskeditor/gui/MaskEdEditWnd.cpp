// -*- c-basic-offset: 4 -*-
/** @file MaskEdEditWnd.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "huginapp/ImageCache.h"
#include "MaskEdEditWnd.h"
#include "MaskEdClientWnd.h"

using HuginBase::ImageCache;
using namespace std;

BEGIN_EVENT_TABLE(MaskEdEditWnd, wxScrolledWindow)
    EVT_LEFT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_LEFT_UP(MaskEdEditWnd::OnLeftMouseButtonUp)
    EVT_RIGHT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_RIGHT_UP(MaskEdEditWnd::OnLeftMouseButtonUp)
    EVT_MOTION(MaskEdEditWnd::OnMotion)
    EVT_PAINT(MaskEdEditWnd::OnPaint)
END_EVENT_TABLE()

MaskEdEditWnd::MaskEdEditWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name), m_scale(1.0)
{
    
}

MaskEdEditWnd::~MaskEdEditWnd() 
{
    m_bimgs.clear();
}

void MaskEdEditWnd::Init()
{
    m_bimgs.clear();
    m_imgfiles.clear();
    m_selected.clear();
    m_brushstroke.clear();
}

void MaskEdEditWnd::LoadImage(const wxString &filename)
{
    LoadImage(string(filename.mb_str()));
}
void MaskEdEditWnd::LoadImage(const string &filename)
{
    if(filename != "")
    {
        ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
        HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
        if (img) {
            wxImage img_temp(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true);
            img_temp.Rescale(img->width()*m_scale, img->height()*m_scale);
            m_bimgs.push_back(new wxBitmap(img_temp));
        } 
        SetScrollbars( 1, 1, m_bimgs[0]->GetWidth(), m_bimgs[0]->GetHeight());
    }
}

void MaskEdEditWnd::LoadImages(vector<string> &filesv)
{
    copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
    //creates local cache of images
    for(vector<string>::iterator it = filesv.begin(); it != filesv.end(); it++)
    {
        LoadImage(*it);
        /*ImageCache::EntryPtr e = ImageCache::getInstance().getImage(*it);
        HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
        if (img) {
            m_bimgs.push_back(new wxBitmap( wxImage(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true)));
        } */
    }
}

void MaskEdEditWnd::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    //DoPrepareDC(dc);
    int x,y;
    x = GetScrollPos(wxSB_HORIZONTAL);
    y = GetScrollPos(wxSB_VERTICAL);
    int i = 0;
    for(vector<wxBitmap*>::iterator it = m_bimgs.begin(); it != m_bimgs.end(); it++, i++)
    {
        //if(!m_selected[i]) continue;
        dc.DrawBitmap(**it, -x, -y);

        //ignore this case for the timebeing
        ////drawing is not really needed here but just in case the window is refershed
        ////while the brushstroke is drawn
        //if(m_brushstroke.pt.size() > 1)
        //{
        //    std::vector<wxPoint>::iterator it = m_brushstroke.pt.begin();
        //    wxPoint pt = *it + wxPoint(-x,-y);
        //    wxPen pen(*wxRED, 1);
        //    dc.SetPen(pen);
        //    for(it++; it != m_brushstroke.pt.end(); it++)
        //    {
        //        dc.DrawLine(pt, *it + wxPoint(-x,-y));
        //        pt = *it + wxPoint(-x,-y);
        //    }
        //    dc.SetPen(wxNullPen);
        //}
    }
}

void MaskEdEditWnd::OnMouseButtonDown(wxMouseEvent &event)
{
    m_brushstroke.pt.clear();
    event.Skip();
}

void MaskEdEditWnd::OnLeftMouseButtonUp(wxMouseEvent &event)
{
    //GlobalCmdHist::getInstance()->addCommand(new MaskMgrAddBrushStroke(m_brushstroke));
    m_brushstroke.pt.clear();
    event.Skip();
    Refresh();
}

void MaskEdEditWnd::OnRightMouseButtonUp(wxMouseEvent &event)
{
    //background
    m_brushstroke.pt.clear();
    event.Skip();
    Refresh();
}
void MaskEdEditWnd::OnMotion(wxMouseEvent &event)
{
    if(event.Dragging() && !event.MiddleIsDown() && m_bimgs.size() > 0)
    {
        int x, y;
        x = GetScrollPos(wxSB_HORIZONTAL);
        y = GetScrollPos(wxSB_VERTICAL);
        if(m_brushstroke.pt.size() > 0)
        {
            wxClientDC dc(this);
            //DoPrepareDC(dc);
            wxPen *pen;
            if(event.LeftIsDown())
                pen = new wxPen(*wxRED, 1);
            else 
                pen = new wxPen(*wxBLUE, 1);
            dc.SetPen(*pen);
            dc.DrawLine(m_brushstroke.pt.back() - wxPoint(x, y), event.GetPosition());
            dc.SetPen(wxNullPen);
        }
        m_brushstroke.pt.push_back(event.GetPosition() + wxPoint(x, y));
        
    }
}

void MaskEdEditWnd::ReloadImages()
{
    m_bimgs.clear();
    for(vector<string>::iterator it = m_imgfiles.begin(); it != m_imgfiles.end(); it++)
        LoadImage(*it);
}   

void MaskEdEditWnd::Zoom(float scale, wxRect region)
{
    m_scale = scale;
    ReloadImages();
    Refresh();
}

float MaskEdEditWnd::GetZoomLevel() const
{
    return m_scale;
}