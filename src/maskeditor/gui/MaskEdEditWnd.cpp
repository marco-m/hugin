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
#include "MaskMgr.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
using HuginBase::ImageCache;
using namespace std;

const int POLY_DRAW_VWIDTH = 4;

BEGIN_EVENT_TABLE(MaskEdEditWnd, wxScrolledWindow)
    //EVT_LEFT_DCLICK(MaskEdEditWnd::OnLeftMouseButtonDClick)
    EVT_LEFT_DOWN(MaskEdEditWnd::OnLeftMouseButtonDown)
    EVT_LEFT_UP(MaskEdEditWnd::OnLeftMouseButtonUp)
    //EVT_RIGHT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_RIGHT_UP(MaskEdEditWnd::OnRightMouseButtonUp)
    EVT_MOTION(MaskEdEditWnd::OnMotion)
    EVT_PAINT(MaskEdEditWnd::OnPaint)
    EVT_ERASE_BACKGROUND(MaskEdEditWnd::OnEraseBackground)
END_EVENT_TABLE()

MaskEdEditWnd::MaskEdEditWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name), m_scale(1.0), m_active(0),
                     m_max_width(0), m_max_height(0), m_bShowOverlappedRect(false), m_ptSelected(-1)
{
    
}

MaskEdEditWnd::~MaskEdEditWnd() 
{
    m_bimgs.clear();
}

void MaskEdEditWnd::init()
{
    m_bShowOverlappedRect = false;
    m_bimgs.clear();
    m_imgfiles.clear();
    m_selected.clear();
    m_brushstroke.clear();
    m_max_width = 0;
    m_max_height = 0;
    m_canvas_size.clear();
    m_pos.clear();
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
    if(filename == "")
        return;
    
    ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
    HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
    if(!img) 
        return;
    vigra::ImageImportInfo info(filename.c_str());
    vigra::Size2D sz = info.getCanvasSize();
    vigra::Diff2D pos = info.getPosition();
    m_canvas_size.push_back(wxPoint(sz.width(), sz.height()));
    m_pos.push_back(wxPoint(pos.x, pos.y));
    m_selected.push_back(true);
    m_max_width = sz.width() > m_max_width ? sz.width() : m_max_width;
    m_max_height = sz.height() > m_max_height ? sz.height() : m_max_height;
    if (img) {
        m_max_width = img->width() > m_max_width ? img->width() : m_max_width;
        m_max_height = img->height() > m_max_height ? img->height() : m_max_height;

        wxImage img_temp(img->width(),
                   img->height(),
                   (unsigned char *) img->data(),
                   true);
        m_wximgs.push_back(new wxImage(img->width(),
                   img->height(),
                   (unsigned char *) img->data(),
                   true));
        wxBitmap *mask = MaskMgr::getInstance()->getSegmentation(filename)->getMaskBitmap();
        img_temp.SetMaskFromImage(mask->ConvertToImage(), 0, 0, 0);
        //img_temp.Rescale(img->width()*m_scale, img->height()*m_scale);
        m_bimgs.push_back(new wxBitmap(img_temp));
    } 
    SetScrollbars( 1, 1, m_max_width, m_max_height);
}

void MaskEdEditWnd::loadImage(const vector<string> &filesv)
{
    //remove_copy_if(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles), );
    copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
    //creates local cache of images
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
    {
        loadImage(*it);
    }
}

void MaskEdEditWnd::findOverlappingRect(int i, int j, wxRect &rect)
{
    wxRect rectA, rectB;
    
    //rectA is the leftmost rectangle
    if(m_pos[i].x < m_pos[j].x) {
        rectA = wxRect(m_pos[i], wxSize(m_bimgs[i]->GetWidth(), m_bimgs[i]->GetHeight()));
        rectB = wxRect(m_pos[j], wxSize(m_bimgs[j]->GetWidth(), m_bimgs[j]->GetHeight()));
    } else {
        rectA = wxRect(m_pos[j], wxSize(m_bimgs[j]->GetWidth(), m_bimgs[j]->GetHeight()));
        rectB = wxRect(m_pos[i], wxSize(m_bimgs[i]->GetWidth(), m_bimgs[i]->GetHeight()));
    }
    
    //set upper-left corner
    rect.x = rectB.x;
    if(rectB.y < rectA.y)
        rect.y = rectA.y;
    else
        rect.y = rectB.y;

    //set bottom-right corner
    wxPoint brA = rectA.GetBottomRight();
    wxPoint brB = rectB.GetBottomRight();
    wxPoint br;
    if(brB.y < brA.y) {
        br.y = brB.y;
    } else {
        br.y = brA.y;
    }
    if(brB.x < brA.x)
        br.x = brB.x;
    else
        br.x = brA.x;
    rect.SetBottomRight(br);
}

void MaskEdEditWnd::OnEraseBackground(wxEraseEvent &event)
{
    event.Skip();
}

void MaskEdEditWnd::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    DoPrepareDC(dc);
    int x,y;
    x = 0;//GetScrollPos(wxSB_HORIZONTAL);
    y = 0;//GetScrollPos(wxSB_VERTICAL);
    int i = m_bimgs.size() - 1;
    int lastDrawn = -1;
    //dc.SetDeviceOrigin(-x, -y);
    dc.SetUserScale(m_scale, m_scale);
    for(vector<wxBitmap*>::reverse_iterator it = m_bimgs.rbegin(); it != m_bimgs.rend(); it++, i--)
    {
        if(!m_selected[i]) continue;
        dc.DrawBitmap(**it, -x + m_pos[i].x, -y + m_pos[i].y, true); 
        if(m_bShowOverlappedRect && lastDrawn > -1) {
            wxRect rect;
            findOverlappingRect(lastDrawn, i, rect);
            wxPen pen = dc.GetPen();
            wxBrush brush = dc.GetBrush();
            dc.SetPen(*wxRED);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(rect);
            dc.SetPen(pen);
            dc.SetBrush(brush);
        }
        lastDrawn = i;
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
    if(m_poly.size() > 0) {
        wxPoint *pts = new wxPoint[m_poly.size()];
        dc.SetPen(*wxRED);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        //copy(m_poly.pt.begin(), m_poly.pt.end(), pts);
        int i = 0;
        for(vector<PixelCoord>::iterator it = m_poly.begin(); it != m_poly.end(); it++,i++)
        {
            pts[i].x = it->x /* m_scale - x*/ + m_pos[m_active].x;
            pts[i].y = it->y /* m_scale - y*/ + m_pos[m_active].y;
            int offset = POLY_DRAW_VWIDTH/2;
            if(m_poly.isMouseOver(*it))
                offset *= 2;
            dc.DrawRectangle(pts[i].x - offset, pts[i].y - offset,
                offset * 2, offset * 2);
        }
        dc.DrawPolygon(m_poly.size(), pts);
    }
}

void MaskEdEditWnd::OnLeftMouseButtonDown(wxMouseEvent &event)
{
    //m_brushstroke.clear();
    wxPoint pos = event.GetPosition(); 
    int x, y;
    x = GetScrollPos(wxSB_HORIZONTAL);
    y = GetScrollPos(wxSB_VERTICAL);
    
    wxPoint newPt = event.GetPosition() + wxPoint(x, y) - m_pos[m_active];
    pos.x /= m_scale;
    pos.y /= m_scale;
    if(m_edmode == ME_POLY) {
        m_ptSelected = m_poly.findVertex(PixelCoord(pos.x, pos.y));
    }
    event.Skip();
}

void MaskEdEditWnd::OnLeftMouseButtonDClick(wxMouseEvent &event)
{
    //if(m_bimgs.empty()) return;
    //if(m_edmode == ME_POLY) {
    //    //end of creating polygon
    //    m_poly.label = ISegmentation::FGND;
    //    m_MaskEdCmdHist.addCommand(new PolygonCmd(MaskMgr::getInstance(), m_poly, m_active));
    //    //reloadImages();
    //    updateMask(m_active);
    //    m_poly.clear();
    //}
    //event.Skip();
    //Refresh();
}

void MaskEdEditWnd::OnLeftMouseButtonUp(wxMouseEvent &event)
{
    if(m_bimgs.empty()) return;
    if(m_edmode == ME_BSTROKE)
    {
        m_brushstroke.label = ISegmentation::FGND;
        m_MaskEdCmdHist.addCommand(new BrushStrokeCmd(MaskMgr::getInstance(), m_brushstroke, m_active));
        //reloadImages();
        updateMask(m_active);
        m_brushstroke.clear();
    }
    else if(m_edmode == ME_POLY) {
        if(m_ptSelected == -1) {
            int x, y;
            x = GetScrollPos(wxSB_HORIZONTAL);
            y = GetScrollPos(wxSB_VERTICAL);
            wxPoint pos = event.GetPosition()+wxPoint(x,y)-m_pos[m_active];
            //m_poly.add(PixelCoord(pos.x/m_scale, pos.y/m_scale));
            m_MaskEdCmdHist.addCommand(new PolyVertexAddCmd(&m_poly, pos.x/m_scale, pos.y/m_scale));
        } else {
            m_ptSelected = -1;
        }
    }
    event.Skip();
    Refresh();
}

void MaskEdEditWnd::OnRightMouseButtonUp(wxMouseEvent &event)
{
    if(m_bimgs.empty()) return;
    if(m_edmode == ME_BSTROKE) {
        m_brushstroke.label = ISegmentation::BKGND;
        m_MaskEdCmdHist.addCommand(new BrushStrokeCmd(MaskMgr::getInstance(), m_brushstroke, m_active));
        //reloadImages();
        updateMask(m_active);
        m_brushstroke.clear();
    } else if(m_edmode == ME_POLY) {
        //end of creating polygon
        m_poly.label = ISegmentation::FGND;
        m_MaskEdCmdHist.addCommand(new PolygonCmd(MaskMgr::getInstance(), m_poly, m_active));
        //reloadImages();
        updateMask(m_active);
        m_poly.clear();
    }
    event.Skip();
    Refresh();
}
void MaskEdEditWnd::OnMotion(wxMouseEvent &event)
{
    if(m_bimgs.empty()) return;
    wxPoint pos = event.GetPosition(); 
    int x, y;
    x = GetScrollPos(wxSB_HORIZONTAL);
    y = GetScrollPos(wxSB_VERTICAL);
    
    wxPoint newPt = event.GetPosition() + wxPoint(x, y) - m_pos[m_active];
    newPt.x /= m_scale;
    newPt.y /= m_scale;

    ((wxFrame*)(GetParent()->GetParent()))->GetStatusBar()->SetStatusText(wxString::Format(_T("(%d,%d) (%d, %d)"),pos.x, pos.y, newPt.x, newPt.y));
    
    if(event.Dragging() && !event.MiddleIsDown() && m_bimgs.size() > 0)
    {
        if(m_edmode == ME_BSTROKE) {
            if(m_brushstroke.pt.size() > 0) {
                wxClientDC dc(this);
                DoPrepareDC(dc);
                dc.SetUserScale(m_scale, m_scale);
                wxPen oldpen = dc.GetPen();
                wxPen *pen;

                if(event.LeftIsDown())
                    pen = new wxPen(*wxRED, 1);
                else 
                    pen = new wxPen(*wxBLUE, 1);
                dc.SetPen(*pen);
                wxPoint lastPt(m_brushstroke.pt.back().x, m_brushstroke.pt.back().y);
                //dc.DrawLine(lastPt - wxPoint(x, y) + m_pos[m_active], event.GetPosition());
                dc.DrawLine(lastPt, newPt);
                dc.SetPen(oldpen);
            }
            m_brushstroke.pt.push_back(PixelCoord(newPt.x, newPt.y));
        } else if(event.LeftIsDown() && m_ptSelected != -1) {
            m_poly.pt[m_ptSelected] = PixelCoord(newPt.x, newPt.y);
            Refresh();
        }
    } else {
        int index = m_poly.findVertex(PixelCoord(newPt.x, newPt.y));
        if(index != -1) {
            Refresh(); //TODO: do blitting rather than refreshing
            /*wxClientDC dc(this);
            DoPrepareDC(dc);
            dc.SetUserScale(m_scale, m_scale);
            dc.DrawRectangle(m_poly.pt[index].x - POLY_DRAW_VWIDTH, m_poly.pt[index].y - POLY_DRAW_VWIDTH, 
                POLY_DRAW_VWIDTH * 2, POLY_DRAW_VWIDTH * 2);*/
        }
    }
}

//void MaskEdEditWnd::reloadImages()
//{
//    m_bimgs.clear();
//    for(vector<string>::iterator it = m_imgfiles.begin(); it != m_imgfiles.end(); it++)
//        loadImage(*it);
//}   

void MaskEdEditWnd::zoom(float scale, wxRect region)
{
    m_scale = scale;
    //reloadImages();
    SetScrollbars( 1, 1, m_max_width*m_scale, m_max_height*m_scale);
    Refresh();
}

float MaskEdEditWnd::getZoomLevel() const
{
    return m_scale;
}

void MaskEdEditWnd::setDisplayImages(const std::map<int, std::pair<std::string, bool> > &selection)
{
    
}

void MaskEdEditWnd::setSelectedImage(int index, bool state)
{
    m_selected[index] = state;
    this->Refresh();
}

void MaskEdEditWnd::toggleShowOverlappedRect()
{
    m_bShowOverlappedRect = m_bShowOverlappedRect ? false : true;
    Refresh();
}

void MaskEdEditWnd::undo()
{
    m_MaskEdCmdHist.undo();
    updateMask(m_active);
    Refresh();
    //reloadImages();
}

void MaskEdEditWnd::redo()
{
    m_MaskEdCmdHist.redo();
    updateMask(m_active);
    Refresh();
    //reloadImages();
}

void MaskEdEditWnd::updateMask(int nimgId)
{
    wxBitmap *mask = MaskMgr::getInstance()->getSegmentation(m_imgfiles[nimgId])->getMaskBitmap();
    wxImage img = *m_wximgs[nimgId];
    img.SetMaskFromImage(mask->ConvertToImage(), 0, 0, 0);
    delete m_bimgs[nimgId];
    m_bimgs[nimgId] = new wxBitmap(img);
}

void MaskEdEditWnd::updateMask(string &imgId)
{
   //wxBitmap *mask = MaskMgr::getInstance()->getSegmentation(imgId)->getMaskBitmap();
   vector<string>::iterator it = find(m_imgfiles.begin(), m_imgfiles.end(), imgId);
   if(it != m_imgfiles.end())
   {
        int index = it - m_imgfiles.begin();
        updateMask(index);
        /*wxImage img = *m_wximgs[index];
        img.SetMaskFromImage(mask->ConvertToImage(), 0, 0, 0);
        delete m_bimgs[index];
        m_bimgs[index] = new wxBitmap(img);*/
   }
}

