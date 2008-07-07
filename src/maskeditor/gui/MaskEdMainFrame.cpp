// -*- c-basic-offset: 4 -*-
/** @file MaskEdMainFrame.cpp
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
#include "MaskEdMainFrame.h"
#include "MaskEdPrefDlg.h"
#include <wx/xrc/xmlres.h>
#include <wx/filedlg.h>
#include <wx/filename.h>

using HuginBase::ImageCache;

BEGIN_EVENT_TABLE(MaskEdMainFrame, wxFrame)
    EVT_MENU(XRCID("action_new_project"), MaskEdMainFrame::OnNewProject)
    EVT_BUTTON(XRCID("action_new_project"), MaskEdMainFrame::OnNewProject)
    EVT_MENU(XRCID("action_open_project"), MaskEdMainFrame::OnOpenProject)
    EVT_BUTTON(XRCID("action_open_project"), MaskEdMainFrame::OnOpenProject)
    EVT_MENU(XRCID("action_load_pto_file"), MaskEdMainFrame::OnLoadPTO)
    EVT_MENU(XRCID("action_load_images"), MaskEdMainFrame::OnLoadImages)
    EVT_MENU(XRCID("action_save_project"), MaskEdMainFrame::OnSaveProject)
    EVT_BUTTON(XRCID("action_save_project"), MaskEdMainFrame::OnSaveProject)
    EVT_MENU(XRCID("action_save_project_as"), MaskEdMainFrame::OnSaveProjectAs)
    EVT_MENU(XRCID("action_save_image"), MaskEdMainFrame::OnSaveImage)
    EVT_MENU(XRCID("action_save_mask"), MaskEdMainFrame::OnSaveMask)
    EVT_MENU(XRCID("action_use_brush"), MaskEdMainFrame::OnBStroke)
    EVT_MENU(XRCID("action_add_polygon_point"), MaskEdMainFrame::OnAddPoint)
    EVT_MENU(XRCID("action_edit_preferences"), MaskEdMainFrame::OnPreference)
    EVT_MENU(XRCID("action_exit"), MaskEdMainFrame::OnQuit)
    EVT_MENU(XRCID("action_about_maskeditor"), MaskEdMainFrame::OnAbout)
    EVT_MENU(XRCID("action_zoom_in"), MaskEdMainFrame::OnzoomIn)
    EVT_BUTTON(XRCID("action_zoom_in"), MaskEdMainFrame::OnzoomIn)
    EVT_MENU(XRCID("action_zoom_out"), MaskEdMainFrame::OnzoomOut)
    EVT_BUTTON(XRCID("action_zoom_out"), MaskEdMainFrame::OnzoomOut)
END_EVENT_TABLE()

MaskEdMainFrame::MaskEdMainFrame(wxWindow *parent) : m_scale(1.0)
{
    wxYield();
    
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("MaskEdMainFrame"));
    
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("main_menubar")));

    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, wxT("main_toolbar")));
    
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    m_MaskEdClientWnd = new MaskEdClientWnd(this);

    topSizer->Add(m_MaskEdClientWnd, 1, wxEXPAND | wxALL | wxFIXED_MINSIZE);

    SetSizerAndFit(topSizer);
    
    MaskMgr::getInstance()->setSegmentationOption(0);

    CreateStatusBar();
    SetStatusText(wxT("Ready"));
    
}

MaskEdMainFrame::~MaskEdMainFrame()
{
    delete m_MaskEdClientWnd;
    //delete MaskMgr::getInstance();
    //delete ImageCache::getInstance();
}

void MaskEdMainFrame::OnNewProject(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnOpenProject(wxCommandEvent &event)
{
    wxString filter = wxT("Mask Project File (*.mep)|*.mep|All files (*.*)|*.*");
    wxFileDialog dialog(this, wxT("Open Project"), wxEmptyString,
        wxEmptyString, filter, wxOPEN);

    if(dialog.ShowModal() == wxID_OK)
    {
        m_MaskEdClientWnd->loadProject(dialog.GetPath());
    }
}

void MaskEdMainFrame::OnLoadImages(wxCommandEvent &event)
{
    wxString filter = wxT("JPEG files (*.jpg;*.jpeg)|*.jpg;*.JPG;*.jpeg;*.JPEG|tiff files (*.tif)|*.tif|All files (*.*)|*.*");
    wxFileDialog dialog(this, wxT("Open Image"), wxEmptyString,
        wxEmptyString, filter, wxOPEN | wxFD_MULTIPLE);
    if(dialog.ShowModal() == wxID_OK)
    {
        wxArrayString paths;
        dialog.GetPaths(paths);
        m_MaskEdClientWnd->loadImages(paths);
        this->Refresh();
    }
}

void MaskEdMainFrame::OnSaveProject(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnSaveProjectAs(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnSaveImage(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnSaveMask(wxCommandEvent &event)
{
    //wxString filter = wxT("JPEG files (*.jpg;*.jpeg)|*.jpg;*.JPG;*.jpeg;*.JPEG|tiff files (*.tif)|*.tif|All files (*.*)|*.*");
    wxFileDialog dialog(this, wxT("Save Mask"), wxEmptyString,
        wxEmptyString, wxEmptyString, wxSAVE);
    if(dialog.ShowModal() == wxID_OK)
    {
        m_MaskEdClientWnd->saveMask(0, dialog.GetPath());
    }

}

void MaskEdMainFrame::OnLoadPTO(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnBStroke(wxCommandEvent &event)
{
    m_MaskEdClientWnd->setEditMode(ME_BSTROKE);
}

void MaskEdMainFrame::OnAddPoint(wxCommandEvent &event)
{
    m_MaskEdClientWnd->setEditMode(ME_POLY);
}

void MaskEdMainFrame::OnPreference(wxCommandEvent &event)
{
    //wxDialog dlg;
    //wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("MaskEdPrefDlg"));
    //dlg.ShowModal();
    MaskEdPrefDlg prefdlg(this);
    prefdlg.ShowModal();
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

void MaskEdMainFrame::OnzoomIn(wxCommandEvent &event)
{
    m_scale += 0.1;
    m_MaskEdClientWnd->zoom(m_scale);
}

void MaskEdMainFrame::OnzoomOut(wxCommandEvent &event)
{
    if(m_scale <= 0.1) return;
    m_scale -= 0.1;
    m_MaskEdClientWnd->zoom(m_scale);
}
