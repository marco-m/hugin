// -*- c-basic-offset: 4 -*-
/** @file MaskEdMainFrame.h
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

#ifndef _MASKEDMAINFRAME_H
#define _MASKEDMAINFRAME_H

#include <wx/wx.h>
#include "MaskEdClientWnd.h"
#include "MaskEdEditWnd.h"
#include "MaskEdPreviewWnd.h"

class MaskEdMainFrame : public wxFrame 
{
    float               m_scale;
    MaskEdClientWnd*    m_MaskEdClientWnd;         //area for editing and previewing input images 

public:
    MaskEdMainFrame(wxWindow *parent);
    ~MaskEdMainFrame();

    void OnNewProject(wxCommandEvent& event);
    void OnOpenProject(wxCommandEvent& event);
    void OnLoadPTO(wxCommandEvent& event);
    void OnLoadImages(wxCommandEvent& event);
    void OnSaveProject(wxCommandEvent& event);
    void OnSaveProjectAs(wxCommandEvent& event);
    void OnSaveImage(wxCommandEvent& event);
    void OnSaveMask(wxCommandEvent& event);
    void OnBStroke(wxCommandEvent& event);
    void OnAddPoint(wxCommandEvent& event);
    void OnPreference(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnShowOverlap(wxCommandEvent& event);

    void OnMotion(wxMouseEvent& event);
    //void OnPaint(wxPaintEvent& event);

    DECLARE_EVENT_TABLE();
};
#endif
