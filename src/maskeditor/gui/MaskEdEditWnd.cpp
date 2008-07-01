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
#include "../core/ISegmentation.h"

using HuginBase::ImageCache;
using namespace std;

BEGIN_EVENT_TABLE(MaskEdEditWnd, wxScrolledWindow)
    EVT_LEFT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_LEFT_UP(MaskEdEditWnd::OnLeftMouseButtonUp)
    EVT_RIGHT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_RIGHT_UP(MaskEdEditWnd::OnRightMouseButtonUp)
    EVT_MOTION(MaskEdEditWnd::OnMotion)
    EVT_PAINT(MaskEdEditWnd::OnPaint)
END_EVENT_TABLE()

MaskEdEditWnd::MaskEdEditWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name), m_scale(1.0), m_active(0)
{
    
}

MaskEdEditWnd::~MaskEdEditWnd() 
{
    m_bimgs.clear();
}

void MaskEdEditWnd::init()
{
    m_bimgs.clear();
    m_imgfiles.clear();
    m_selected.clear();
    m_brushstroke.clear();
}

void MaskEdEditWnd::setEditMode(MaskEdEditMode_t edmode)
{
    m_edmode = edmode;
}

void MaskEdEditWnd::loadImage(const wxString &filename)
{
    loadImage(string(filename.mb_str()));
}
void MaskEdEditWnd::loadImage(const string &filename)
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
            wxBitmap *mask = MaskMgr::getInstance()->getSegmentation(filename)->getMaskBitmap();
            img_temp.SetMaskFromImage(mask->ConvertToImage(), 0, 0, 0);
            img_temp.Rescale(img->width()*m_scale, img->height()*m_scale);
            m_bimgs.push_back(new wxBitmap(img_temp));
        } 
        SetScrollbars( 1, 1, m_bimgs[0]->GetWidth(), m_bimgs[0]->GetHeight());
    }
}

void MaskEdEditWnd::loadImages(const vector<string> &filesv)
{
    copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
    //creates local cache of images
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
    {
        loadImage(*it);
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
    //dc.SetDeviceOrigin(-x, -y);
    //dc.SetUserScale(m_scale, m_scale);
    for(vector<wxBitmap*>::iterator it = m_bimgs.begin(); it != m_bimgs.end(); it++, i++)
    {
        //if(!m_selected[i]) continue;
        dc.DrawBitmap(**it, -x, -y, true); 
        
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
    if(m_poly.pt.size() > 0) {
        wxPoint *pts = new wxPoint[m_poly.pt.size()];
        dc.SetPen(*wxRED);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        //copy(m_poly.pt.begin(), m_poly.pt.end(), pts);
        int i = 0;
        for(vector<wxPoint>::iterator it = m_poly.pt.begin(); it != m_poly.pt.end(); it++,i++)
        {
            pts[i].x = it->x * m_scale - x;
            pts[i].y = it->y * m_scale - y;
        }
        dc.DrawPolygon(m_poly.pt.size(), pts);
    }
}

void MaskEdEditWnd::OnMouseButtonDown(wxMouseEvent &event)
{
    m_brushstroke.clear();
    event.Skip();
}

void MaskEdEditWnd::OnLeftMouseButtonUp(wxMouseEvent &event)
{
    if(m_edmode == ME_BSTROKE)
    {
        m_brushstroke.label = ISegmentation::FGND;
        m_MaskEdCmdHist.addCommand(new BrushStrokeCmd(MaskMgr::getInstance(), m_brushstroke, m_active));
        m_brushstroke.clear();
    }
    else if(m_edmode == ME_POLY) {
        int x, y;
        x = GetScrollPos(wxSB_HORIZONTAL);
        y = GetScrollPos(wxSB_VERTICAL);
        wxPoint pos = event.GetPosition()+wxPoint(x,y);
        m_poly.add(wxPoint(pos.x/m_scale, pos.y/m_scale));
    }
    event.Skip();
    Refresh();
}

void MaskEdEditWnd::OnRightMouseButtonUp(wxMouseEvent &event)
{
    if(m_edmode == ME_BSTROKE)
    {
        m_brushstroke.label = ISegmentation::BKGND;
        m_MaskEdCmdHist.addCommand(new BrushStrokeCmd(MaskMgr::getInstance(), m_brushstroke, m_active));
        m_brushstroke.clear();
    }
    else if(m_edmode == ME_POLY) {
        //end of creating polygon
        m_poly.label = ISegmentation::FGND;
        m_MaskEdCmdHist.addCommand(new PolygonCmd(MaskMgr::getInstance(), m_poly, m_active));
        reloadImages();
        m_poly.clear();
    }
    event.Skip();
    Refresh();
}
void MaskEdEditWnd::OnMotion(wxMouseEvent &event)
{
    if(event.Dragging() && !event.MiddleIsDown() && m_bimgs.size() > 0 && m_edmode == ME_BSTROKE)
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

void MaskEdEditWnd::reloadImages()
{
    m_bimgs.clear();
    for(vector<string>::iterator it = m_imgfiles.begin(); it != m_imgfiles.end(); it++)
        loadImage(*it);
}   

void MaskEdEditWnd::zoom(float scale, wxRect region)
{
    m_scale = scale;
    reloadImages();
    //SetScrollbars( 1, 1, m_bimgs[0]->GetWidth()*m_scale, m_bimgs[0]->GetHeight()*m_scale);
    Refresh();
}

float MaskEdEditWnd::getZoomLevel() const
{
    return m_scale;
}
