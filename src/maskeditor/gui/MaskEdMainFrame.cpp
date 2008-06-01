// -*- c-basic-offset: 4 -*-
/** @file MaskEdMainFrame.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id:  MaskEdMainFrame.cpp $
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

#include "MaskEdMainFrame.h"
#include <wx/xrc/xmlres.h>

BEGIN_EVENT_TABLE(MaskEdMainFrame, wxFrame)
    EVT_MENU(XRCID("action_new_project"), MaskEdMainFrame::OnNew)
    EVT_BUTTON(XRCID("action_new_project"), MaskEdMainFrame::OnNew)
    EVT_MENU(XRCID("action_open_project"), MaskEdMainFrame::OnOpen)
    EVT_BUTTON(XRCID("action_open_project"), MaskEdMainFrame::OnOpen)
    EVT_MENU(XRCID("action_load_pto_file"), MaskEdMainFrame::OnLoadPTO)
    EVT_MENU(XRCID("action_save_project"), MaskEdMainFrame::OnSave)
    EVT_BUTTON(XRCID("action_save_project"), MaskEdMainFrame::OnSave)
    EVT_MENU(XRCID("action_use_brush"), MaskEdMainFrame::OnBStroke)
    EVT_MENU(XRCID("action_add_polygon_point"), MaskEdMainFrame::OnAddPoint)
    EVT_MENU(XRCID("action_edit_preferences"), MaskEdMainFrame::OnPreference)
    EVT_MENU(XRCID("action_exit"), MaskEdMainFrame::OnQuit)
    EVT_MENU(XRCID("action_about_maskeditor"), MaskEdMainFrame::OnAbout)
    EVT_MOTION(MaskEdMainFrame::OnMotion)
END_EVENT_TABLE()

MaskEdMainFrame::MaskEdMainFrame(wxWindow *parent)
{
    wxYield();
    
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("MaskEdMainFrame"));
    CreateStatusBar();
    SetStatusText(wxT("Ready"));
}

void MaskEdMainFrame::OnNew(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnOpen(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnSave(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnLoadPTO(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnBStroke(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnAddPoint(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnPreference(wxCommandEvent &event)
{

}
void MaskEdMainFrame::OnQuit(wxCommandEvent &event)
{
    Close(true);
}

void MaskEdMainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxT("Mask Editor"), wxT("About Mask Editor"),
        wxOK | wxICON_INFORMATION, this);
}

void MaskEdMainFrame::OnMotion(wxMouseEvent &event)
{
    if(event.Dragging())
    {
        wxClientDC dc (this);
        wxPen pen(*wxRED, 1);
        dc.SetPen(pen);
        dc.DrawPoint(event.GetPosition());
        dc.SetPen(wxNullPen);
    }
}

