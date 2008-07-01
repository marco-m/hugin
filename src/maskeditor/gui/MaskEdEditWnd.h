// -*- c-basic-offset: 4 -*-
/** @file MaskEdEditWnd.h
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

#ifndef MASKEDEDITWND_H
#define MASKEDEDITWND_H

#include <wx/wx.h>
#include <vector>
#include "../core/BrushStroke.h"
#include "../core/MaskPoly.h"
#include "../core/MaskEdCommand.h"

enum MaskEdEditMode_t { ME_BSTROKE = 1, ME_POLY };

class MaskEdEditWnd : public wxScrolledWindow
{
    std::vector<wxBitmap*>  m_bimgs;            //local cache
    std::vector<std::string> m_imgfiles;
    std::vector<bool>       m_selected;         //index of selected images
    int                     m_active;           //index of image on which mask is drawn
    BrushStroke             m_brushstroke;
    MaskPoly                m_poly;
    float                   m_scale;
    int                     m_edmode;           //editing mode

    AppBase::CommandHistory<AppBase::Command<std::string> >   m_MaskEdCmdHist;

public:
    MaskEdEditWnd(wxWindow *parent,
                     wxWindowID winid = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxScrolledWindowStyle | wxDOUBLE_BORDER,
                     const wxString& name = wxPanelNameStr);
    ~MaskEdEditWnd();

    void init();
    void loadImage(const wxString &filename);
    void loadImage(const std::string &filename);
    void loadImages(const std::vector<std::string> &filesv);
    void reloadImages();

    void setDisplayImages(std::vector<bool> selection);
    void zoom(float scale = 1.0, wxRect region = wxRect());
    float getZoomLevel() const;
    
    void setEditMode(MaskEdEditMode_t edmode);

    void OnPaint(wxPaintEvent &event);
    void OnMouseButtonDown(wxMouseEvent &event);
    void OnLeftMouseButtonUp(wxMouseEvent &event);
    void OnRightMouseButtonUp(wxMouseEvent &event);
    void OnMotion(wxMouseEvent &event);
    

    DECLARE_EVENT_TABLE()
};

#endif
