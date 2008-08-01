// -*- c-basic-offset: 4 -*-
/** @file MaskEdClientWnd.h
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
#ifndef MASKEDCLIENTWND_H
#define MASKEDCLIENTWND_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include "MaskEdEditWnd.h"
#include "MaskEdPreviewWnd.h"

class MaskEdClientWnd : public wxSplitterWindow
{
    MaskEdEditWnd       *m_MaskEdEditWnd;
    MaskEdPreviewWnd    *m_MaskEdPreviewWnd;

public:
    MaskEdClientWnd(wxWindow *parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxSP_3D|wxSP_LIVE_UPDATE,
                     const wxString& name = wxT("splitter"));
    ~MaskEdClientWnd();

    MaskEdEditWnd * getMaskEdEditWnd();
    MaskEdPreviewWnd * getMaskEdPreviewWnd();

    void loadProject(const wxString &filename);
    void saveProject();                         //save the currently loaded project
    void saveProjectAs(const wxString &filename);
    void saveMask(int i, const wxString &filename);
    void loadImage(const wxArrayString &filenames);

    void zoom(float scale = 1.0, wxRect region = wxRect());
    float getZoomLevel() const;
    wxString getFile() const;
    void setEditMode(MaskEdEditMode_t edmode);
    void toggleShowOverlappedRect();

    void undo();
    void redo();
    
};

#endif
