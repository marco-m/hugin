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
#include <vector>
#include <string>
#include "huginapp/ImageCache.h"
#include "MaskEdApp.h"
#include "MaskEdMainFrame.h"
#include "MaskEdPrefDlg.h"
#include <wx/xrc/xmlres.h>
#include <wx/filedlg.h>
#include <wx/filename.h>

using namespace std;
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
    EVT_MENU(XRCID("action_draw_bstroke"), MaskEdMainFrame::OnBStroke)
    EVT_BUTTON(XRCID("action_draw_bstroke"), MaskEdMainFrame::OnBStroke)
    EVT_MENU(XRCID("action_draw_polygon"), MaskEdMainFrame::OnAddPoint)
    EVT_BUTTON(XRCID("action_draw_polygon"), MaskEdMainFrame::OnAddPoint)
    EVT_MENU(XRCID("action_edit_preferences"), MaskEdMainFrame::OnPreference)
    EVT_MENU(XRCID("action_exit"), MaskEdMainFrame::OnQuit)
    EVT_MENU(XRCID("action_about_maskeditor"), MaskEdMainFrame::OnAbout)
    EVT_MENU(XRCID("action_zoom_in"), MaskEdMainFrame::OnZoomIn)
    EVT_BUTTON(XRCID("action_zoom_in"), MaskEdMainFrame::OnZoomIn)
    EVT_MENU(XRCID("action_zoom_out"), MaskEdMainFrame::OnZoomOut)
    EVT_BUTTON(XRCID("action_zoom_out"), MaskEdMainFrame::OnZoomOut)
    EVT_MENU(XRCID("action_set_roi"), MaskEdMainFrame::OnSetROI)
    EVT_BUTTON(XRCID("action_set_roi"), MaskEdMainFrame::OnSetROI)
    EVT_MENU(XRCID("action_show_overlap"), MaskEdMainFrame::OnShowOverlap)
    EVT_BUTTON(XRCID("action_show_overlap"), MaskEdMainFrame::OnShowOverlap)
    EVT_COMBOBOX(XRCID("m_comboSegChoice"), MaskEdMainFrame::OnSegSelUpdate)
    //EVT_SIZE(MaskEdMainFrame::OnSize)
END_EVENT_TABLE()

MaskEdMainFrame::MaskEdMainFrame(wxWindow *parent) : m_scale(1.0)
{
    wxYield();

    wxXmlResource::Get()->LoadFrame(this, parent, wxT("MaskEdMainFrame"));
    
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("main_menubar")));
    wxToolBar *tb = wxXmlResource::Get()->LoadToolBar(this, wxT("main_toolbar"));
    SetToolBar(tb);

#ifdef __WXMSW__
   wxIcon appIcon(MaskEdApp::getMaskEdApp()->getXRCPath() + wxT("data/mask_editor_icon_48x48.ico"), wxBITMAP_TYPE_ICO);
#else
   wxIcon appIcon(MaskEdApp::getMaskEdApp()->getXRCPath() + wxT("data/mask_editor_icon_48x48.png"), wxBITMAP_TYPE_PNG);
#endif
    SetIcon(appIcon);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    wxSize sz = GetClientSize();
    m_MaskEdClientWnd = new MaskEdClientWnd(this, wxID_ANY, wxDefaultPosition,sz);

    topSizer->Add(m_MaskEdClientWnd, 1, wxEXPAND | wxALL | wxFIXED_MINSIZE);

    SetSizerAndFit(topSizer);
    
    MaskMgr::getInstance()->setSegmentationOption(0);
	wxWindow *pWnd = tb;
#ifdef __WXMSW__
	pWnd = this;
#endif
    vector<string> options = MaskMgr::getInstance()->getSegmentationOptions();
    for(vector<string>::iterator it = options.begin(); it != options.end(); it++)
		XRCCTRL(*pWnd, "m_comboSegChoice", wxComboBox)->Append(wxString(it->c_str(), wxConvUTF8));
	XRCCTRL(*pWnd, "m_comboSegChoice", wxComboBox)->SetValue(wxString(options[0].c_str(), wxConvUTF8));
								     
    CreateStatusBar();
    SetStatusText(wxT("Ready"));
}

MaskEdMainFrame::~MaskEdMainFrame()
{
    delete m_MaskEdClientWnd;
    //delete MaskMgr::getInstance();
    //delete ImageCache::getInstance();
}

void MaskEdMainFrame::OnSize(wxSizeEvent &event)
{
    wxSize sz = event.GetSize();
    if(m_MaskEdClientWnd)
        m_MaskEdClientWnd->SetSashPosition(sz.GetHeight()*0.75);
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
    wxString filter = wxT("Image files (*.jpg;*.jpeg;*.tif;*.png;*.bmp)|*.jpg;*.JPG;*.jpeg;*.JPEG;*.tif;*.tiff;*.png;*.bmp|JPEG files (*.jpg;*.jpeg)|*.jpg;*.JPG;*.jpeg;*.JPEG|Tiff files (*.tif;*.tiff)|*.tif|BMP files (*.bmp)|*.bmp|All files (*.*)|*.*");
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

void MaskEdMainFrame::OnZoomIn(wxCommandEvent &event)
{
    m_scale += 0.1;
    m_MaskEdClientWnd->zoom(m_scale);
}

void MaskEdMainFrame::OnZoomOut(wxCommandEvent &event)
{
    if(m_scale <= 0.1) return;
    m_scale -= 0.1;
    m_MaskEdClientWnd->zoom(m_scale);
}

void MaskEdMainFrame::OnSetROI(wxCommandEvent &event)
{

}

void MaskEdMainFrame::OnShowOverlap(wxCommandEvent &event)
{
    m_MaskEdClientWnd->toggleShowOverlappedRect();
}

void MaskEdMainFrame::OnSegSelUpdate(wxCommandEvent &event)
{
    wxWindow *pWnd = GetToolBar();
#ifdef __WXMSW__
    pWnd = this;
#endif
   int ichoice = XRCCTRL(*pWnd, "m_comboSegChoice", wxComboBox)->GetSelection();
   MaskMgr::getInstance()->setSegmentationOption(ichoice);    
}
