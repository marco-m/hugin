// -*- c-basic-offset: 4 -*-

/** @file GLPreviewFrame.cpp
 *
 *  @brief implementation of GLPreviewFrame Class
 *
 *  @author James Legg and Pablo d'Angelo <pablo.dangelo@web.de>
 *  @author Darko Makreshanski
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// use toggle buttons or uncomment check boxes

#ifndef __WXMAC__
#define USE_TOGGLE_BUTTON 1
#endif
//wxMac now has toggle buttons, but you can't overide their colours.

#include <bitset>
#include <limits>
#include <iostream>

#include <config.h>

#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
#include <GL/glew.h>

#include "panoinc_WX.h"

#include "panoinc.h"

#include "base_wx/platform.h"
#include "base_wx/MyProgressDialog.h"
#include "hugin/config_defaults.h"
#include "hugin/GLPreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/GLViewer.h"
#include "hugin/TextKillFocusHandler.h"
// something messed up... temporary fix :-(
#include "hugin_utils/utils.h"
#define DEBUG_HEADER ""
/*#include <vigra_ext/ImageTransforms.h>
*/

extern "C" {
#include <pano13/queryfeature.h>
}

#include "ToolHelper.h"
#include "Tool.h"
#include "PreviewCropTool.h"
#include "PreviewDragTool.h"
#include "PreviewIdentifyTool.h"
#include "PreviewDifferenceTool.h"
#include "PreviewPanoMaskTool.h"
#include "PreviewControlPointTool.h"
#include "PreviewLayoutLinesTool.h"

#include "ProjectionGridTool.h"

#include "OverviewCameraTool.h"
#include "OverviewOutlinesTool.h"

#include <wx/progdlg.h>
#if wxCHECK_VERSION(2, 9, 1)
#include <wx/infobar.h>
#endif

using namespace utils;

// a random id, hope this doesn't break something..
enum {
    ID_TOGGLE_BUT = wxID_HIGHEST+500,
    PROJ_PARAM_NAMES_ID = wxID_HIGHEST+1300,
    PROJ_PARAM_VAL_ID = wxID_HIGHEST+1400,
    PROJ_PARAM_SLIDER_ID = wxID_HIGHEST+1500,
    PROJ_PARAM_RESET_ID = wxID_HIGHEST+1550,
    ID_TOGGLE_BUT_LEAVE = wxID_HIGHEST+1600,
    ID_FULL_SCREEN = wxID_HIGHEST+1710,
    ID_SHOW_ALL = wxID_HIGHEST+1711,
    ID_SHOW_NONE = wxID_HIGHEST+1712,
    ID_UNDO = wxID_HIGHEST+1713,
    ID_REDO = wxID_HIGHEST+1714
};

/** enum, which contains all different toolbar modes */
enum{
    mode_preview=0,
    mode_layout,
    mode_projection,
    mode_drag,
    mode_crop
};

BEGIN_EVENT_TABLE(GLwxAuiFloatingFrame, wxAuiFloatingFrame)
    EVT_ACTIVATE(GLwxAuiFloatingFrame::OnActivate)
//    EVT_CLOSE(GLwxAuiFloatingFrame::OnClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(GLPreviewFrame, wxFrame)
    EVT_CLOSE(GLPreviewFrame::OnClose)
    EVT_SHOW(GLPreviewFrame::OnShowEvent)
    EVT_BUTTON(XRCID("preview_center_tool"), GLPreviewFrame::OnCenterHorizontally)
    EVT_BUTTON(XRCID("preview_fit_pano_tool"), GLPreviewFrame::OnFitPano)
    EVT_BUTTON(XRCID("preview_fit_pano_tool2"), GLPreviewFrame::OnFitPano)
    EVT_BUTTON(XRCID("preview_straighten_pano_tool"), GLPreviewFrame::OnStraighten)
    EVT_BUTTON(XRCID("apply_num_transform"), GLPreviewFrame::OnNumTransform)
    EVT_BUTTON(ID_SHOW_ALL, GLPreviewFrame::OnShowAll)
    EVT_BUTTON(ID_SHOW_NONE, GLPreviewFrame::OnShowNone)
    EVT_CHECKBOX(XRCID("preview_photometric_tool"), GLPreviewFrame::OnPhotometric)
    EVT_TOOL(XRCID("preview_identify_tool"), GLPreviewFrame::OnIdentify)
    EVT_CHECKBOX(XRCID("preview_control_point_tool"), GLPreviewFrame::OnControlPoint)
    EVT_BUTTON(XRCID("preview_autocrop_tool"), GLPreviewFrame::OnAutocrop)
    EVT_NOTEBOOK_PAGE_CHANGED(XRCID("mode_toolbar_notebook"), GLPreviewFrame::OnSelectMode)
    EVT_NOTEBOOK_PAGE_CHANGING(XRCID("mode_toolbar_notebook"), GLPreviewFrame::OnToolModeChanging)
   
    EVT_BUTTON(XRCID("exposure_default_button"), GLPreviewFrame::OnDefaultExposure)
    EVT_SPIN_DOWN(XRCID("exposure_spin"), GLPreviewFrame::OnDecreaseExposure)
    EVT_SPIN_UP(XRCID("exposure_spin"), GLPreviewFrame::OnIncreaseExposure)
    EVT_CHOICE(XRCID("blend_mode_choice"), GLPreviewFrame::OnBlendChoice)
    EVT_CHOICE(XRCID("drag_mode_choice"), GLPreviewFrame::OnDragChoice)
    EVT_CHOICE(XRCID("projection_choice"), GLPreviewFrame::OnProjectionChoice)
    EVT_CHOICE(XRCID("overview_mode_choice"), GLPreviewFrame::OnOverviewModeChoice)
    EVT_TOGGLEBUTTON(XRCID("overview_toggle"), GLPreviewFrame::OnOverviewToggle)
    EVT_CHECKBOX(XRCID("preview_show_grid"), GLPreviewFrame::OnSwitchPreviewGrid)
#ifndef __WXMAC__
    // wxMac does not process these
    EVT_SCROLL_CHANGED(GLPreviewFrame::OnChangeFOV)
    EVT_COMMAND_SCROLL(XRCID("layout_scale_slider"), GLPreviewFrame::OnLayoutScaleChange)
#else
    EVT_SCROLL_THUMBRELEASE(GLPreviewFrame::OnChangeFOV)
    EVT_SCROLL_ENDSCROLL(GLPreviewFrame::OnChangeFOV)
    EVT_COMMAND_SCROLL_THUMBRELEASE(XRCID("layout_scale_slider"), GLPreviewFrame::OnLayoutScaleChange)
    EVT_COMMAND_SCROLL_ENDSCROLL(XRCID("layout_scale_slider"), GLPreviewFrame::OnLayoutScaleChange)
    EVT_COMMAND_SCROLL_THUMBTRACK(XRCID("layout_scale_slider"), GLPreviewFrame::OnLayoutScaleChange)
#endif
    EVT_SCROLL_THUMBTRACK(GLPreviewFrame::OnTrackChangeFOV)
    EVT_TEXT_ENTER(XRCID("pano_text_hfov"), GLPreviewFrame::OnHFOVChanged )
    EVT_TEXT_ENTER(XRCID("pano_text_vfov"), GLPreviewFrame::OnVFOVChanged )
    EVT_TEXT_ENTER(XRCID("pano_val_roi_left"), GLPreviewFrame::OnROIChanged)
    EVT_TEXT_ENTER(XRCID("pano_val_roi_top"), GLPreviewFrame::OnROIChanged)
    EVT_TEXT_ENTER(XRCID("pano_val_roi_right"), GLPreviewFrame::OnROIChanged)
    EVT_TEXT_ENTER(XRCID("pano_val_roi_bottom"), GLPreviewFrame::OnROIChanged)
    EVT_TEXT_ENTER(XRCID("exposure_text"), GLPreviewFrame::OnExposureChanged)
    EVT_COMMAND_RANGE(PROJ_PARAM_VAL_ID,PROJ_PARAM_VAL_ID+PANO_PROJECTION_MAX_PARMS,wxEVT_COMMAND_TEXT_ENTER,GLPreviewFrame::OnProjParameterChanged)
    EVT_BUTTON(PROJ_PARAM_RESET_ID, GLPreviewFrame::OnProjParameterReset)
    EVT_TOOL(ID_FULL_SCREEN, GLPreviewFrame::OnFullScreen)
    EVT_TOOL(ID_UNDO, GLPreviewFrame::OnUndo)
    EVT_TOOL(ID_REDO, GLPreviewFrame::OnRedo)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(ImageToogleButtonEventHandler, wxEvtHandler)
    EVT_ENTER_WINDOW(ImageToogleButtonEventHandler::OnEnter)
    EVT_LEAVE_WINDOW(ImageToogleButtonEventHandler::OnLeave)
#ifdef USE_TOGGLE_BUTTON
    EVT_TOGGLEBUTTON(-1, ImageToogleButtonEventHandler::OnChange)
#else
    EVT_CHECKBOX(-1, ImageToogleButtonEventHandler::OnChange)
#endif    
END_EVENT_TABLE()


void AddLabelToBitmapButton(wxBitmapButton* button, wxString new_label,bool TextBelow=true)
{
    int new_width=0;
    int new_height=0;
    int text_height=0;
    int text_width=0;
    button->GetTextExtent(new_label.append(wxT(" ")), &text_width,&text_height);
    if(TextBelow)
    {
        new_height=23+text_height;
        if(text_width<24)
            new_width=24;
        else
            new_width=text_width;
    }
    else
    {
        new_height=22;
        new_width=24+text_width;
    };
    wxBitmap new_bitmap(new_width,new_height);
    wxMemoryDC dc(new_bitmap);
    dc.SetBackground(wxBrush(button->GetBackgroundColour()));
    dc.Clear();
    if(TextBelow)
    {
        dc.DrawBitmap(button->GetBitmapLabel(),(new_width/2)-11,0,true);
        dc.SetFont(button->GetParent()->GetFont());
        dc.DrawText(new_label,(new_width-text_width)/2,23);
    }
    else
    {
        dc.DrawBitmap(button->GetBitmapLabel(),0,0,true);
        dc.SetFont(button->GetParent()->GetFont());
        dc.DrawText(new_label,24,(22-text_height)/2);
    };
    dc.SelectObject(wxNullBitmap);
    //some fiddeling with mask
    wxImage new_image=new_bitmap.ConvertToImage();
    wxColour bg=button->GetBackgroundColour();
    new_image.SetMaskColour(bg.Red(),bg.Green(),bg.Blue());
    wxBitmap new_bitmap_mask(new_image);
    button->SetBitmapLabel(new_bitmap_mask);
    button->Refresh();
};


GLwxAuiFloatingFrame* GLwxAuiManager::CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& p)
{
    DEBUG_DEBUG("CREATING FLOATING FRAME");
    frame->PauseResize();
    GLwxAuiFloatingFrame* fl_frame = new GLwxAuiFloatingFrame(parent, this, p);
    DEBUG_DEBUG("CREATED FLOATING FRAME");
    return fl_frame;
}

void GLwxAuiFloatingFrame::OnActivate(wxActivateEvent& evt)
{
    DEBUG_DEBUG("FRAME ACTIVATE");
    GLPreviewFrame * frame = ((GLwxAuiManager*) GetOwnerManager())->getPreviewFrame();
    frame->ContinueResize();
    evt.Skip();
}

void GLwxAuiFloatingFrame::OnMoveFinished()
{
    DEBUG_DEBUG("FRAME ON MOVE FINISHED");
    GLPreviewFrame * frame = ((GLwxAuiManager*) GetOwnerManager())->getPreviewFrame();
    frame->PauseResize();
    wxAuiFloatingFrame::OnMoveFinished();
    DEBUG_DEBUG("FRAME AFTER ON MOVE FINISHED");
}

void GLPreviewFrame::PauseResize()
{
    DEBUG_DEBUG("PAUSE RESIZE");
    GLresize = false;
}

void GLPreviewFrame::ContinueResize()
{
    GLresize = true;
    wxSizeEvent event = wxSizeEvent(wxSize());
    m_GLPreview->Resized(event);
    m_GLOverview->Resized(event);
}


#define PF_STYLE (wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN)
#include <iostream>
GLPreviewFrame::GLPreviewFrame(wxFrame * frame, PT::Panorama &pano)
    : wxFrame(frame,-1, _("Fast Panorama preview"), wxDefaultPosition, wxDefaultSize,
              PF_STYLE),
      m_pano(pano)
#if !wxCHECK_VERSION(2, 9, 1)
    ,
      m_projectionStatusPushed(false)
#endif
{


	DEBUG_TRACE("");

    // initialize pointer
    preview_helper = NULL;
    panosphere_overview_helper = NULL;
    plane_overview_helper = NULL;
    crop_tool = NULL;
    drag_tool = NULL;
    overview_drag_tool = NULL;
    identify_tool = NULL ;
    panosphere_overview_identify_tool = NULL;
    plane_overview_identify_tool = NULL;
    difference_tool = NULL;
    plane_difference_tool = NULL;
    panosphere_difference_tool = NULL;
    pano_mask_tool = NULL;

    m_mode = -1;
    m_oldProjFormat = -1;
    // add a status bar
    CreateStatusBar(3);
    int widths[3] = {-3, 150, 150};
    SetStatusWidths(3, widths);
    SetStatusText(wxT(""),1);
    SetStatusText(wxT(""),2);
    wxConfigBase * config = wxConfigBase::Get();

    wxPanel *tool_panel = wxXmlResource::Get()->LoadPanel(this,wxT("mode_panel"));
    m_tool_notebook = XRCCTRL(*this,"mode_toolbar_notebook",wxNotebook);
    m_ToolBar_Identify = XRCCTRL(*this,"preview_mode_toolbar",wxToolBar);
    AddLabelToBitmapButton(XRCCTRL(*this,"preview_center_tool",wxBitmapButton),_("Center"));
    AddLabelToBitmapButton(XRCCTRL(*this,"preview_fit_pano_tool",wxBitmapButton),_("Fit"));
    AddLabelToBitmapButton(XRCCTRL(*this,"preview_straighten_pano_tool",wxBitmapButton),_("Straighten"));
    AddLabelToBitmapButton(XRCCTRL(*this,"preview_fit_pano_tool2",wxBitmapButton),_("Fit"));
    AddLabelToBitmapButton(XRCCTRL(*this,"preview_autocrop_tool",wxBitmapButton),_("Autocrop"));


    m_topsizer = new wxBoxSizer( wxVERTICAL );

    wxPanel * toggle_panel = new wxPanel(this);
    wxBoxSizer * toggle_panel_sizer = new wxBoxSizer(wxHORIZONTAL);
    toggle_panel->SetSizer(toggle_panel_sizer);

    wxPanel *overview_toggle_panel = wxXmlResource::Get()->LoadPanel(toggle_panel,wxT("overview_toggle_panel"));
    toggle_panel_sizer->Add(overview_toggle_panel, 0, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, 0);

    bool overview_hidden;
    config->Read(wxT("/GLPreviewFrame/overview_hidden"), &overview_hidden, false);
    m_OverviewToggle = XRCCTRL(*this, "overview_toggle", wxToggleButton);
    if (overview_hidden) {
        m_OverviewToggle->SetValue(false);
    } else {
        m_OverviewToggle->SetValue(true);
    }

    m_ToggleButtonSizer = new wxStaticBoxSizer(
        new wxStaticBox(toggle_panel, -1, _("displayed images")),
    wxHORIZONTAL );

	m_ButtonPanel = new wxScrolledWindow(toggle_panel, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	// Set min height big enough to display scrollbars as well
    m_ButtonPanel->SetSizeHints(20, 42);
	//Horizontal scroll bars only
	m_ButtonPanel->SetScrollRate(10, 0);
    m_ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_ButtonPanel->SetAutoLayout(true);
	m_ButtonPanel->SetSizer(m_ButtonSizer);

    wxPanel *panel = new wxPanel(toggle_panel);
    wxBitmap bitmap;
    bitmap.LoadFile(huginApp::Get()->GetXRCPath()+wxT("data/preview_show_all.png"),wxBITMAP_TYPE_PNG);
    wxBitmapButton * select_all = new wxBitmapButton(panel,ID_SHOW_ALL,bitmap);
    bitmap.LoadFile(huginApp::Get()->GetXRCPath()+wxT("data/preview_show_none.png"),wxBITMAP_TYPE_PNG);
    wxBitmapButton * select_none = new wxBitmapButton(panel,ID_SHOW_NONE,bitmap);
    
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(select_all,5,wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM);
    sizer->Add(select_none,5,wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM);
    panel->SetSizer(sizer);
    m_ToggleButtonSizer->Add(panel, 0, wxALIGN_CENTER_VERTICAL);
    m_ToggleButtonSizer->Add(m_ButtonPanel, 1, wxEXPAND | wxADJUST_MINSIZE | wxALIGN_CENTER_VERTICAL, 0);
    AddLabelToBitmapButton(select_all,_("All"),false);
    AddLabelToBitmapButton(select_none,_("None"), false);

    toggle_panel_sizer->Add(m_ToggleButtonSizer, wxEXPAND);

    m_topsizer->Add(tool_panel, 0, wxEXPAND | wxALL, 2);
    m_topsizer->Add(toggle_panel, 0, wxEXPAND | wxADJUST_MINSIZE | wxBOTTOM, 5);

#if wxCHECK_VERSION(2, 9, 1)
    m_infoBar = new wxInfoBar(this);
    m_topsizer->Add(m_infoBar, 0, wxEXPAND);
#endif

    //create panel that will hold gl canvases
    wxPanel * vis_panel = new wxPanel(this);

    wxPanel * preview_panel = new wxPanel(vis_panel);
    wxPanel * overview_panel = new wxPanel(vis_panel);

    // create our Viewers
    int args[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0};
    m_GLPreview = new GLPreview(preview_panel, pano, args, this);
    m_GLOverview = new GLOverview(overview_panel, pano, args, this, m_GLPreview->GetContext());
    m_GLOverview->SetMode(GLOverview::PANOSPHERE);
    m_GLOverview->SetActive(!overview_hidden);

    // set the AUI manager to our panel
    m_mgr = new GLwxAuiManager(this, m_GLPreview, m_GLOverview);
    m_mgr->SetManagedWindow(vis_panel);
    
    //create the sizer for the preview
    wxFlexGridSizer * flexSizer = new wxFlexGridSizer(2,0,5,5);
    flexSizer->AddGrowableCol(0);
    flexSizer->AddGrowableRow(0);

    //overview sizer
    wxBoxSizer * overview_sizer = new wxBoxSizer(wxVERTICAL);


    flexSizer->Add(m_GLPreview,
                  1,        // not vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    m_VFOVSlider = new wxSlider(preview_panel, -1, 1,
                                1, 180,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_VERTICAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                _("VFOV"));
    m_VFOVSlider->SetLineSize(2);
    m_VFOVSlider->SetPageSize(10);
    m_VFOVSlider->SetTickFreq(5,0);
    m_VFOVSlider->SetToolTip(_("drag to change the vertical field of view"));

    flexSizer->Add(m_VFOVSlider, 0, wxEXPAND);

    m_HFOVSlider = new wxSlider(preview_panel, -1, 1,
                                1, 360,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_HORIZONTAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                _("HFOV"));
    m_HFOVSlider->SetPageSize(10);
    m_HFOVSlider->SetLineSize(2);
    m_HFOVSlider->SetTickFreq(5,0);

    m_HFOVSlider->SetToolTip(_("drag to change the horizontal field of view"));

    m_HFOVText = XRCCTRL(*this, "pano_text_hfov" ,wxTextCtrl);
    DEBUG_ASSERT(m_HFOVText);
    m_HFOVText->PushEventHandler(new TextKillFocusHandler(this));
    m_VFOVText = XRCCTRL(*this, "pano_text_vfov" ,wxTextCtrl);
    DEBUG_ASSERT(m_VFOVText);
    m_VFOVText->PushEventHandler(new TextKillFocusHandler(this));

    m_ROILeftTxt = XRCCTRL(*this, "pano_val_roi_left", wxTextCtrl);
    DEBUG_ASSERT(m_ROILeftTxt);
    m_ROILeftTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROIRightTxt = XRCCTRL(*this, "pano_val_roi_right", wxTextCtrl);
    DEBUG_ASSERT(m_ROIRightTxt);
    m_ROIRightTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROITopTxt = XRCCTRL(*this, "pano_val_roi_top", wxTextCtrl);
    DEBUG_ASSERT(m_ROITopTxt);
    m_ROITopTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROIBottomTxt = XRCCTRL(*this, "pano_val_roi_bottom", wxTextCtrl);
    DEBUG_ASSERT(m_ROIBottomTxt);
    m_ROIBottomTxt->PushEventHandler(new TextKillFocusHandler(this));


    flexSizer->Add(m_HFOVSlider, 0, wxEXPAND);

    wxPanel *overview_command_panel = wxXmlResource::Get()->LoadPanel(overview_panel,wxT("overview_command_panel"));
    m_OverviewModeChoice = XRCCTRL(*this, "overview_mode_choice", wxChoice);
    m_OverviewModeChoice->Append(_("Panosphere"));
    m_OverviewModeChoice->Append(_("Mosaic plane"));
    m_OverviewModeChoice->SetSelection(0);
    overview_command_panel->SetSize(0,0,200,20,wxSIZE_AUTO_WIDTH);

    overview_sizer->Add(overview_command_panel, 0, wxEXPAND);
    overview_sizer->Add(m_GLOverview, 1, wxEXPAND);

    m_previewGrid = XRCCTRL(*this, "preview_show_grid", wxCheckBox);
    bool showGrid;
    config->Read(wxT("/GLPreviewFrame/showPreviewGrid"),&showGrid,true);
    m_previewGrid->SetValue(showGrid);

    preview_panel->SetSizer(flexSizer);
    overview_panel->SetSizer(overview_sizer);

    m_mgr->AddPane(preview_panel, 
        wxAuiPaneInfo(
            ).Name(wxT("preview")
            ).MinSize(300,200
            ).CloseButton(false
            ).CaptionVisible(false
            ).Caption(_("Preview")
            ).Floatable(false
            ).Dockable(false
            ).Center(
            )
        );

    m_mgr->AddPane(overview_panel, 
        wxAuiPaneInfo(
            ).Name(wxT("overview")
            ).MinSize(300,200
            ).CloseButton(false
            ).CaptionVisible(
            ).Caption(_("Overview")
            ).FloatingSize(100,100
            ).FloatingPosition(500,500
            ).Dockable(true
            ).PinButton(
            ).Left(
            ).Show(!overview_hidden
            )
        );


    m_topsizer->Add(vis_panel, 1, wxEXPAND);


    m_ProjectionChoice = XRCCTRL(*this,"projection_choice",wxChoice);

    /* populate with all available projection types */
    int nP = panoProjectionFormatCount();
    for(int n=0; n < nP; n++) {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj)) {
            wxString str2(proj.name, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        }
    }
    m_ProjectionChoice->SetSelection(2);

    //////////////////////////////////////////////////////
    // Blend mode
    // remaining blend mode should be added after OpenGL context has been created
    // see FillBlendMode()
    m_differenceIndex = -1;
    // create choice item
    m_BlendModeChoice = XRCCTRL(*this,"blend_mode_choice",wxChoice);
    m_BlendModeChoice->Append(_("normal"));
    m_BlendModeChoice->SetSelection(0);

    m_DragModeChoice = XRCCTRL(*this, "drag_mode_choice", wxChoice);
    m_DragModeChoice->Append(_("normal"));
    m_DragModeChoice->Append(_("mosaic"));
    m_DragModeChoice->SetSelection(0);
    // default drag mode
    GLPreviewFrame::DragChoiceLayout(0);

    // TODO implement hdr display in OpenGL, if possible?
    // Disabled until someone can figure out HDR display in OpenGL.
    /*
    //////////////////////////////////////////////////////
    // LDR, HDR
    blendModeSizer->Add(new wxStaticText(this, -1, _("Output:")),
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width

    m_choices[0] = _("LDR");
    m_choices[1] = _("HDR");
    m_outputModeChoice = new wxChoice(this, ID_OUTPUTMODE_CHOICE,
                                      wxDefaultPosition, wxDefaultSize,
                                      2, m_choices);
    m_outputModeChoice->SetSelection(0);
    blendModeSizer->Add(m_outputModeChoice,
                        0,
                        wxALL | wxALIGN_CENTER_VERTICAL,
                        5);
    */
    
    /////////////////////////////////////////////////////
    // exposure
    m_defaultExposureBut = XRCCTRL(*this, "exposure_default_button", wxBitmapButton);

    m_exposureTextCtrl = XRCCTRL(*this, "exposure_text", wxTextCtrl);
    m_exposureSpinBut = XRCCTRL(*this, "exposure_spin", wxSpinButton); 
    m_exposureSpinBut->SetValue(0);

    m_projection_panel = XRCCTRL(*this, "projection_panel", wxPanel);
    m_projParamSizer = new wxBoxSizer(wxHORIZONTAL);

    wxBitmapButton * resetProjButton=new wxBitmapButton(m_projection_panel, PROJ_PARAM_RESET_ID, 
        wxArtProvider::GetBitmap(wxART_REDO));
    resetProjButton->SetToolTip(_("Resets the projections parameters to their default values."));
    m_projParamSizer->Add(resetProjButton, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

    m_projParamNamesLabel.resize(PANO_PROJECTION_MAX_PARMS);
    m_projParamTextCtrl.resize(PANO_PROJECTION_MAX_PARMS);
    m_projParamSlider.resize(PANO_PROJECTION_MAX_PARMS);

    for (int i=0; i < PANO_PROJECTION_MAX_PARMS; i++) {

        m_projParamNamesLabel[i] = new wxStaticText(m_projection_panel, PROJ_PARAM_NAMES_ID+i, _("param:"));
        m_projParamSizer->Add(m_projParamNamesLabel[i],
                        0,        // not vertically strechable
                        wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width
        m_projParamTextCtrl[i] = new wxTextCtrl(m_projection_panel, PROJ_PARAM_VAL_ID+i, _("0"),
                                    wxDefaultPosition, wxSize(35,-1), wxTE_PROCESS_ENTER);
        m_projParamSizer->Add(m_projParamTextCtrl[i],
                        0,        // not vertically strechable
                        wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width

        m_projParamSlider[i] = new wxSlider(m_projection_panel, PROJ_PARAM_SLIDER_ID+i, 0, -90, 90);
        m_projParamSizer->Add(m_projParamSlider[i],
                        1,        // not vertically strechable
                        wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL , // draw border all around
                        5);       // border width
    }

    m_projection_panel->GetSizer()->Add(m_projParamSizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

    // do not show projection param sizer
    m_projection_panel->GetSizer()->Show(m_projParamSizer, false, true);

    // the initial size as calculated by the sizers
    this->SetSizer( m_topsizer );
    m_topsizer->SetSizeHints( this );

    // set the minimize icon
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    m_pano.addObserver(this);

    RestoreFramePosition(this, wxT("GLPreviewFrame"));
    
#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_GLPreview->GetBackgroundColour());
#endif

    if (config->Read(wxT("/GLPreviewFrame/isShown"), 0l) != 0) {
        Show();
    }
    wxAcceleratorEntry entries[3];
    entries[0].Set(wxACCEL_NORMAL,WXK_F11,ID_FULL_SCREEN);
    entries[1].Set(wxACCEL_CTRL,(int)'Z',ID_UNDO);
    entries[2].Set(wxACCEL_CTRL,(int)'R',ID_REDO);
    wxAcceleratorTable accel(3, entries);
    SetAcceleratorTable(accel);
#ifdef __WXGTK__
    // set explicit focus to button panel, otherwise the hotkey F11 is not right processed
    m_ButtonPanel->SetFocus();
#endif

     // tell the manager to "commit" all the changes just made
    m_mgr->Update();
}

void GLPreviewFrame::LoadOpenGLLayout()
{
    PauseResize();
    wxString OpenGLLayout=wxConfig::Get()->Read(wxT("/GLPreviewFrame/OpenGLLayout"));
    if(!OpenGLLayout.IsEmpty())
    {
        m_mgr->LoadPerspective(OpenGLLayout,true);
    };
    ContinueResize();
};

GLPreviewFrame::~GLPreviewFrame()
{
    DEBUG_TRACE("dtor writing config");
    wxConfigBase * config = wxConfigBase::Get();
    wxSize sz = GetClientSize();

    StoreFramePosition(this, wxT("GLPreviewFrame"));

    if ( (!this->IsIconized()) && (! this->IsMaximized()) && this->IsShown()) {
        config->Write(wxT("/GLPreviewFrame/isShown"), 1l);
    } else {
        config->Write(wxT("/GLPreviewFrame/isShown"), 0l);
    }

    config->Write(wxT("/GLPreviewFrame/blendMode"), m_BlendModeChoice->GetSelection());
    config->Write(wxT("/GLPreviewFrame/OpenGLLayout"), m_mgr->SavePerspective());
    config->Write(wxT("/GLPreviewFrame/overview_hidden"), !(m_OverviewToggle->GetValue()));
    config->Write(wxT("/GLPreviewFrame/showPreviewGrid"), m_previewGrid->GetValue());
    
    // delete all of the tools. When the preview is never used we never get an
    // OpenGL context and therefore don't create the tools.
    if (crop_tool)
    {
        preview_helper->DeactivateTool(crop_tool); delete crop_tool;
        preview_helper->DeactivateTool(drag_tool); delete drag_tool;
        preview_helper->DeactivateTool(identify_tool); delete identify_tool;
        preview_helper->DeactivateTool(difference_tool); delete difference_tool;
        preview_helper->DeactivateTool(pano_mask_tool); delete pano_mask_tool;
    }
    if (panosphere_overview_identify_tool) {
        panosphere_overview_helper->DeactivateTool(overview_drag_tool); delete overview_drag_tool;
        panosphere_overview_helper->DeactivateTool(panosphere_overview_identify_tool); delete panosphere_overview_identify_tool;
        panosphere_overview_helper->DeactivateTool(panosphere_difference_tool); delete panosphere_difference_tool;
    }
    if (plane_overview_identify_tool) {
        plane_overview_helper->DeactivateTool(plane_overview_identify_tool); delete plane_overview_identify_tool;
        plane_overview_helper->DeactivateTool(plane_difference_tool); delete plane_difference_tool;
    }
    m_HFOVText->PopEventHandler(true);
    m_VFOVText->PopEventHandler(true);
    m_ROILeftTxt->PopEventHandler(true);
    m_ROIRightTxt->PopEventHandler(true);
    m_ROITopTxt->PopEventHandler(true);
    m_ROIBottomTxt->PopEventHandler(true);
    for (int i=0; i < m_ToggleButtons.size(); i++)
    {
        m_ToggleButtons[i]->PopEventHandler();
        m_ToggleButtons[i]->PopEventHandler();
        m_ToggleButtons[i]->PopEventHandler();
    }
    m_pano.removeObserver(this);

     // deinitialize the frame manager
     m_mgr->UnInit();
     if (m_mgr) {
        delete m_mgr;
     }

    DEBUG_TRACE("dtor end");
}


bool GLwxAuiManager::ProcessDockResult(wxAuiPaneInfo& target,
                                   const wxAuiPaneInfo& new_pos)
{
//    std::cout << "target: " << std::bitset<std::numeric_limits<unsigned int>::digits>(target.state) << std::endl;
//    std::cout << "target: " << target.dock_direction << " " << target.dock_layer << " " << target.dock_row << " " << target.dock_pos << " " << target.state << std::endl;
//    std::cout << "newpos: " << std::bitset<std::numeric_limits<unsigned int>::digits>(new_pos.state) << std::endl;
//    std::cout << "newpos: " << new_pos.dock_direction << " " << new_pos.dock_layer << " " << new_pos.dock_row << " " << new_pos.dock_pos << " " << new_pos.state << std::endl;
    return wxAuiManager::ProcessDockResult(target, new_pos);
}


/**
* Update tools and GUI elements according to blend mode choice
*/
void GLPreviewFrame::updateBlendMode()
{
    if (m_BlendModeChoice != NULL)
    {
        int index=m_BlendModeChoice->GetSelection();
        if(index==0)
        {
            // normal mode
            if (preview_helper != NULL 
                && difference_tool != NULL)
            {
                preview_helper->DeactivateTool(difference_tool);
            };

            if (panosphere_overview_helper != NULL 
                && panosphere_difference_tool != NULL)
            {
                panosphere_overview_helper->DeactivateTool(panosphere_difference_tool);
            };

            if (plane_overview_helper != NULL 
                && plane_difference_tool != NULL)
            {
                plane_overview_helper->DeactivateTool(plane_difference_tool);
            };


        }
        else
        {
            if(index==m_differenceIndex)
            {
                // difference mode
                if (preview_helper != NULL 
                    && identify_tool != NULL 
                    && difference_tool != NULL
                    && m_ToolBar_Identify != NULL )
                {
                    preview_helper->DeactivateTool(identify_tool);
                    m_ToolBar_Identify->ToggleTool(XRCID("preview_identify_tool"), false);
                    preview_helper->ActivateTool(difference_tool);
                    CleanButtonColours();
                };

                // difference mode
                if (panosphere_overview_helper != NULL 
                    && panosphere_overview_identify_tool != NULL 
                    && panosphere_difference_tool != NULL)
                {
                    panosphere_overview_helper->DeactivateTool(panosphere_overview_identify_tool);
                    panosphere_overview_helper->ActivateTool(panosphere_difference_tool);
                };

                // difference mode
                if (plane_overview_helper != NULL 
                    && plane_overview_identify_tool != NULL 
                    && plane_difference_tool != NULL)
                {
                    plane_overview_helper->DeactivateTool(plane_overview_identify_tool);
                    plane_overview_helper->ActivateTool(plane_difference_tool);
                };

            }
            else
            {
                DEBUG_WARN("Unknown blend mode selected");
            };
        }
    }
}

void GLPreviewFrame::panoramaChanged(Panorama &pano)
{
    const PanoramaOptions & opts = pano.getOptions();

    wxString projection;
    m_ProjectionChoice->SetSelection(opts.getProjection());
    m_VFOVSlider->Enable( opts.fovCalcSupported(opts.getProjection()) );
    
    // No HDR display yet.
    /*
    m_outputModeChoice->SetSelection(opts.outputMode);
    if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
        m_exposureTextCtrl->Hide();
        m_defaultExposureBut->Hide();
        m_decExposureBut->Hide();
        m_incExposureBut->Hide();
    } else {
        m_exposureTextCtrl->Show();
        m_defaultExposureBut->Show();
        m_decExposureBut->Show();
        m_incExposureBut->Show();
    }*/
    m_exposureTextCtrl->SetValue(wxString(doubleToString(opts.outputExposureValue,2).c_str(), wxConvLocal));

    bool activeImgs = pano.getActiveImages().size() > 0;

    // TODO: enable display of parameters and set their limits, if projection has some.

    int nParam = opts.m_projFeatures.numberOfParameters;
    bool relayout = false;
    // if the projection format has changed
    if (opts.getProjection() != m_oldProjFormat) {
        DEBUG_DEBUG("Projection format changed");
        if (nParam) {
            // show parameters and update labels.
            m_projection_panel->GetSizer()->Show(m_projParamSizer, true, true);
            int i;
            for (i=0; i < nParam; i++) {
                const pano_projection_parameter * pp = & (opts.m_projFeatures.parm[i]);
                wxString str2(pp->name, wxConvLocal);
                str2 = wxGetTranslation(str2);
                m_projParamNamesLabel[i]->SetLabel(str2);
                m_projParamSlider[i]->SetRange(utils::roundi(pp->minValue), utils::roundi(pp->maxValue));
            }
            for(;i < PANO_PROJECTION_MAX_PARMS; i++) {
                m_projParamNamesLabel[i]->Hide();
                m_projParamSlider[i]->Hide();
                m_projParamTextCtrl[i]->Hide();
            }
            relayout = true;
        } else {
            m_projection_panel->GetSizer()->Show(m_projParamSizer, false, true);
            relayout = true;
        }
    }
    if (nParam) {
        // display new values
        std::vector<double> params = opts.getProjectionParameters();
        assert((int) params.size() == nParam);
        for (int i=0; i < nParam; i++) {
            wxString val = wxString(doubleToString(params[i],1).c_str(), wxConvLocal);
            m_projParamTextCtrl[i]->SetValue(wxString(val.wc_str(), wxConvLocal));
            m_projParamSlider[i]->SetValue(utils::roundi(params[i]));
        }
    }
    if (relayout) {
        m_projection_panel->Layout();
        Refresh();
    }
    SetStatusText(wxString::Format(wxT("%.1f x %.1f"), opts.getHFOV(), opts.getVFOV()),2);
    m_HFOVSlider->SetValue(roundi(opts.getHFOV()));
    m_VFOVSlider->SetValue(roundi(opts.getVFOV()));
    std::string val;
    val = doubleToString(opts.getHFOV(),1);
    m_HFOVText->SetValue(wxString(val.c_str(), wxConvLocal));
    val = doubleToString(opts.getVFOV(),1);
    m_VFOVText->SetValue(wxString(val.c_str(), wxConvLocal));
    m_VFOVText->Enable(opts.fovCalcSupported(opts.getProjection()));

    m_oldProjFormat = opts.getProjection();
    
    // Check if autocrop is usable on this projection.
    bool hasActiveImages = pano.getActiveImages().size() > 0;
    XRCCTRL(*this,"preview_autocrop_tool",wxBitmapButton)->Enable(
        hasActiveImages && opts.fovCalcSupported(opts.getProjection()));
    m_ROILeftTxt->SetValue(wxString::Format(wxT("%d"), opts.getROI().left() ));
    m_ROIRightTxt->SetValue(wxString::Format(wxT("%d"), opts.getROI().right() ));
    m_ROITopTxt->SetValue(wxString::Format(wxT("%d"), opts.getROI().top() ));
    m_ROIBottomTxt->SetValue(wxString::Format(wxT("%d"), opts.getROI().bottom() ));
    
    ShowProjectionWarnings();
}

void GLPreviewFrame::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    bool dirty = false;

    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrButtons = m_ToggleButtons.size();

//    m_displayedImgs.clear();

    // remove items for nonexisting images
    for (int i=nrButtons-1; i>=(int)nrImages; i--)
    {
        m_ButtonSizer->Detach(m_ToggleButtonPanel[i]);
        // Image toggle buttons have three event handlers on the stack which
        // must be removed before the buttons get destroyed.
        m_ToggleButtons[i]->PopEventHandler();
        m_ToggleButtons[i]->PopEventHandler();
        m_ToggleButtons[i]->PopEventHandler();
        delete m_ToggleButtons[i];
        delete m_ToggleButtonPanel[i];
        m_ToggleButtons.pop_back();
        m_ToggleButtonPanel.pop_back();
        delete toogle_button_event_handlers[i*3];
        delete toogle_button_event_handlers[i*3+1];
        delete toogle_button_event_handlers[i*3+2];
        toogle_button_event_handlers.pop_back();
        toogle_button_event_handlers.pop_back();
        toogle_button_event_handlers.pop_back();
        dirty = true;
    }

    //change overview mode to mosaic plane if any tr parameter is non zero
    if (m_GLOverview->GetMode() == GLOverview::PANOSPHERE) {
        bool hasTrNonZero = false;
//        for (unsigned int i = 0 ; i < m_pano.getNrOfImages() ; i++) {
        for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it) {
        unsigned int i = *it;
            if (pano.getSrcImage(i).getX() != 0) hasTrNonZero = true;
            if (pano.getSrcImage(i).getY() != 0) hasTrNonZero = true;
            if (pano.getSrcImage(i).getZ() != 0) hasTrNonZero = true;
        }
        if (hasTrNonZero) {
            m_GLOverview->SetMode(GLOverview::PLANE);
            m_OverviewModeChoice->SetSelection(1);
        }
    }

    // add buttons
    if ( nrImages >= nrButtons ) {
        for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrButtons) {
                unsigned int imgNr = *it;
                // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
                //put wxToggleButton in a wxPanel because 
                //on Windows the background colour of wxToggleButton can't be changed
                wxPanel *pan = new wxPanel(m_ButtonPanel);
                wxBoxSizer * siz = new wxBoxSizer(wxHORIZONTAL);
                pan->SetSizer(siz);
#ifdef USE_TOGGLE_BUTTON
                wxToggleButton * but = new wxToggleButton(pan,
                                                          ID_TOGGLE_BUT + *it,
                                                          wxString::Format(wxT("%d"),*it),
                                                          wxDefaultPosition, wxDefaultSize,
                                                          wxBU_EXACTFIT);
#else
                wxCheckBox * but = new wxCheckBox(pan,
                                                  ID_TOGGLE_BUT + *it,
                                                  wxString::Format(wxT("%d"),*it));
#endif
                siz->Add(but,0,wxALL | wxADJUST_MINSIZE,5);
                // for the identification tool to work, we need to find when the
                // mouse enters and exits the button. We use a custom event
                // handler, which will also toggle the images:
                ImageToogleButtonEventHandler * event_handler = new
                    ImageToogleButtonEventHandler(*it, &identify_tool,
                        m_ToolBar_Identify->FindById(XRCID("preview_identify_tool")),
                        &m_pano);
                toogle_button_event_handlers.push_back(event_handler);
                but->PushEventHandler(event_handler);

                ImageToogleButtonEventHandler * ov_event_handler = new
                    ImageToogleButtonEventHandler(*it, &panosphere_overview_identify_tool,
                        m_ToolBar_Identify->FindById(XRCID("preview_identify_tool")),
                        &m_pano);
                toogle_button_event_handlers.push_back(ov_event_handler);
                but->PushEventHandler(ov_event_handler);

                ImageToogleButtonEventHandler * pl_ov_event_handler = new
                    ImageToogleButtonEventHandler(*it, &plane_overview_identify_tool,
                        m_ToolBar_Identify->FindById(XRCID("preview_identify_tool")),
                        &m_pano);
                toogle_button_event_handlers.push_back(pl_ov_event_handler);
                but->PushEventHandler(pl_ov_event_handler);

                wxSize sz = but->GetSize();
//                but->SetSize(res.GetWidth(),sz.GetHeight());
                // HACK.. set fixed width. that should work
                // better than all that stupid dialogunit stuff, that
                // breaks on wxWin 2.5
                but->SetSize(20, sz.GetHeight());
                but->SetValue(true);
                m_ButtonSizer->Add(pan,
                                   0,
                                   wxLEFT | wxTOP | wxADJUST_MINSIZE,
                                   0);
                m_ToggleButtons.push_back(but);
                m_ToggleButtonPanel.push_back(pan);
                dirty = true;
            }
        }
    }

    // update existing items
    UIntSet displayedImages = m_pano.getActiveImages();
    for (unsigned i=0; i < nrImages; i++) {
        m_ToggleButtons[i]->SetValue(set_contains(displayedImages, i));
        wxFileName tFilename(wxString (pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
        m_ToggleButtons[i]->SetToolTip(tFilename.GetFullName());
    }

    if (dirty) {
		m_ButtonSizer->SetVirtualSizeHints(m_ButtonPanel);
		// Layout();
		DEBUG_INFO("New m_ButtonPanel width: " << (m_ButtonPanel->GetSize()).GetWidth());
		DEBUG_INFO("New m_ButtonPanel Height: " << (m_ButtonPanel->GetSize()).GetHeight());
    }

    if(nrImages==0)
    {
        SetMode(mode_preview);
        m_tool_notebook->ChangeSelection(mode_preview);
    };
    for(size_t i=1; i<m_tool_notebook->GetPageCount();i++)
    {
        m_tool_notebook->GetPage(i)->Enable(nrImages!=0);
    };
}

void GLPreviewFrame::redrawPreview()
{
    Refresh();
}

void GLPreviewFrame::OnShowEvent(wxShowEvent& e)
{

    DEBUG_TRACE("OnShow");
    bool toggle_on = m_OverviewToggle->GetValue();
    wxAuiPaneInfo &inf = m_mgr->GetPane(_("overview"));
    if (inf.IsOk()) {
        if (e.IsShown()) {
            if (!inf.IsShown() && toggle_on ) {
                inf.Show();
                m_mgr->Update();
            }
        } else {
            if (inf.IsFloating() && inf.IsShown()) {
                DEBUG_DEBUG("hiding overview float");
                inf.Hide();
                m_mgr->Update();
            }
        }
    }

}

void GLPreviewFrame::OnOverviewToggle(wxCommandEvent& e)
{
    DEBUG_TRACE("overview toggle");
    bool toggle_on = m_OverviewToggle->GetValue();
    wxAuiPaneInfo &inf = m_mgr->GetPane(_("overview"));
    if (inf.IsOk()) {
        if (inf.IsShown() && !toggle_on) {
            inf.Hide();
            m_GLOverview->SetActive(false);
            m_mgr->Update();
        } else if (!(inf.IsShown() && toggle_on)) {
            inf.Show();
            m_GLOverview->SetActive(true);
            m_mgr->Update();
        }
    }
}

void GLPreviewFrame::OnSwitchPreviewGrid(wxCommandEvent & e)
{
    if(m_previewGrid->GetValue())
    {
        preview_helper->ActivateTool(preview_projection_grid);
    }
    else
    {
        preview_helper->DeactivateTool(preview_projection_grid);
    }
    m_GLPreview->Refresh();
}

void GLPreviewFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_TRACE("OnClose")
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("hiding");
    } else {
        DEBUG_DEBUG("closing");
        this->Destroy();
    }
}

#if 0
// need to add the wxChoice somewhere
void PreviewFrame::OnProjectionChanged()
{
    PanoramaOptions opt = m_pano.getOptions();
    int lt = m_ProjectionChoice->GetSelection();
    wxString Ip;
    switch ( lt ) {
    case PanoramaOptions::RECTILINEAR:       Ip = _("Rectilinear"); break;
    case PanoramaOptions::CYLINDRICAL:       Ip = _("Cylindrical"); break;
    case PanoramaOptions::EQUIRECTANGULAR:   Ip = _("Equirectangular"); break;
    }
    opt.projectionFormat = (PanoramaOptions::ProjectionFormat) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Projection changed: "  << lt << ":" << Ip )


}
#endif

void GLPreviewFrame::OnCenterHorizontally(wxCommandEvent & e)
{
    if (m_pano.getActiveImages().size() == 0) return;

    GlobalCmdHist::getInstance().addCommand(
        new PT::CenterPanoCmd(m_pano)
        );
    // fit pano afterwards
    OnFitPano(e);
}

void GLPreviewFrame::OnStraighten(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    GlobalCmdHist::getInstance().addCommand(
        new PT::StraightenPanoCmd(m_pano)
        );
    if (m_pano.getOptions().getHFOV() > 359) {
        // adjust canvas size for 360 deg panos
        OnFitPano(e);
    } else {
        // also center non 360 deg panos
        OnCenterHorizontally(e);
    }
}

void GLPreviewFrame::OnFitPano(wxCommandEvent & e)
{
    if (m_pano.getActiveImages().size() == 0) return;

    DEBUG_TRACE("");
    PanoramaOptions opt = m_pano.getOptions();

    double hfov, height;
    m_pano.fitPano(hfov, height);
    opt.setHFOV(hfov);
    opt.setHeight(roundi(height));

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );

    DEBUG_INFO ( "new fov: [" << opt.getHFOV() << " "<< opt.getVFOV() << "] => height: " << opt.getHeight() );
}

void GLPreviewFrame::OnShowAll(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    DEBUG_ASSERT(m_pano.getNrOfImages() == m_ToggleButtons.size());
    UIntSet displayedImgs;
    for (unsigned int i=0; i < m_pano.getNrOfImages(); i++) {
        displayedImgs.insert(i);
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetActiveImagesCmd(m_pano, displayedImgs)
        );
}

void GLPreviewFrame::OnShowNone(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    DEBUG_ASSERT(m_pano.getNrOfImages() == m_ToggleButtons.size());
    for (unsigned int i=0; i < m_pano.getNrOfImages(); i++) {
        m_ToggleButtons[i]->SetValue(false);
    }
    UIntSet displayedImgs;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetActiveImagesCmd(m_pano, displayedImgs)
        );
}

void GLPreviewFrame::OnNumTransform(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    wxString text;
    double y;
    double p;
    double r;
    double x;
    double z;

    int index = m_DragModeChoice->GetSelection();
    switch (index) {
        case 0: //normal
            text = XRCCTRL(*this,"input_yaw",wxTextCtrl)->GetValue();
            if(!utils::stringToDouble(std::string(text.mb_str(wxConvLocal)), y))
            {
                wxBell();
                wxMessageBox(_("Yaw value must be numeric."),_("Warning"),wxOK | wxICON_ERROR,this);
                return;
            }
            text = XRCCTRL(*this,"input_pitch",wxTextCtrl)->GetValue();
            if(!utils::stringToDouble(std::string(text.mb_str(wxConvLocal)), p))
            {
                wxBell();
                wxMessageBox(_("Pitch value must be numeric."),_("Warning"),wxOK | wxICON_ERROR,this);
                return;
            }
            text = XRCCTRL(*this,"input_roll",wxTextCtrl)->GetValue();
            if(!utils::stringToDouble(std::string(text.mb_str(wxConvLocal)), r))
            {
                wxBell();
                wxMessageBox(_("Roll value must be numeric."),_("Warning"),wxOK | wxICON_ERROR,this);
                return;
            }
            GlobalCmdHist::getInstance().addCommand(
                    new PT::RotatePanoCmd(m_pano, y, p, r)
                );
            break;
        case 1: //mosaic
            text = XRCCTRL(*this,"input_x",wxTextCtrl)->GetValue();
            if(!utils::stringToDouble(std::string(text.mb_str(wxConvLocal)), x))
            {
                wxBell();
                wxMessageBox(_("X value must be numeric."),_("Warning"),wxOK | wxICON_ERROR,this);
                return;
            }
            text = XRCCTRL(*this,"input_y",wxTextCtrl)->GetValue();
            if(!utils::stringToDouble(std::string(text.mb_str(wxConvLocal)), y))
            {
                wxBell();
                wxMessageBox(_("Y value must be numeric."),_("Warning"),wxOK | wxICON_ERROR,this);
                return;
            }
            text = XRCCTRL(*this,"input_z",wxTextCtrl)->GetValue();
            if(!utils::stringToDouble(std::string(text.mb_str(wxConvLocal)), z))
            {
                wxBell();
                wxMessageBox(_("Z value must be numeric."),_("Warning"),wxOK | wxICON_ERROR,this);
                return;
            }
            GlobalCmdHist::getInstance().addCommand(
                    new PT::TranslatePanoCmd(m_pano, x, y, z)
                );
            break;
    }
}

void GLPreviewFrame::OnExposureChanged(wxCommandEvent & e)
{
    PanoramaOptions opts = m_pano.getOptions();
    // exposure
    wxString text = m_exposureTextCtrl->GetValue();
    DEBUG_INFO ("target exposure = " << text.mb_str(wxConvLocal) );
    double p = 0;
    if (text != wxT("")) {
        if (!str2double(text, p)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
    }
    opts.outputExposureValue = p;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opts )
                                           );
}

void GLPreviewFrame::OnProjParameterChanged(wxCommandEvent & e)
{
    PanoramaOptions opts = m_pano.getOptions();
    int nParam = opts.m_projFeatures.numberOfParameters;
    std::vector<double> para = opts.getProjectionParameters();
    for (int i = 0; i < nParam; i++) {
        if (e.GetEventObject() == m_projParamTextCtrl[i]) {
            wxString text = m_projParamTextCtrl[i]->GetValue();
            DEBUG_INFO ("param " << i << ":  = " << text.mb_str(wxConvLocal) );
            double p = 0;
            if (text != wxT("")) {
                if (!str2double(text, p)) {
                    wxLogError(_("Value must be numeric."));
                    return;
                }
            }
            para[i] = p;
        }
    }
    opts.setProjectionParameters(para);
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opts )
                                           );
}

void GLPreviewFrame::OnProjParameterReset(wxCommandEvent &e)
{
    PanoramaOptions opts=m_pano.getOptions();
    opts.resetProjectionParameters();
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd(m_pano, opts)
        );
};

void GLPreviewFrame::OnChangeFOV(wxScrollEvent & e)
{
    DEBUG_TRACE("");

    PanoramaOptions opt = m_pano.getOptions();

    if (e.GetEventObject() == m_HFOVSlider) {
        DEBUG_DEBUG("HFOV changed (slider): " << e.GetInt() << " == " << m_HFOVSlider->GetValue());
        opt.setHFOV(e.GetInt());
    } else if (e.GetEventObject() == m_VFOVSlider) {
        DEBUG_DEBUG("VFOV changed (slider): " << e.GetInt());
        opt.setVFOV(e.GetInt());
    } else if (e.GetEventObject() == XRCCTRL(*this,"layout_scale_slider",wxSlider)) {
        DEBUG_DEBUG("Layout scale changed (slider): " << e.GetInt());
        GLPreviewFrame::OnLayoutScaleChange(e);
    } else {
        int nParam = opt.m_projFeatures.numberOfParameters;
        std::vector<double> para = opt.getProjectionParameters();
        for (int i = 0; i < nParam; i++) {
            if (e.GetEventObject() == m_projParamSlider[i]) {
                // update
                para[i] = e.GetInt();
            }
        }
        opt.setProjectionParameters(para);
		opt.setHFOV(m_HFOVSlider->GetValue());
		opt.setVFOV(m_VFOVSlider->GetValue());
    }

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );    
}

void GLPreviewFrame::OnTrackChangeFOV(wxScrollEvent & e)
{
    DEBUG_TRACE("");
    DEBUG_TRACE("fov change " << e.GetInt());
    PanoramaOptions opt = m_pano.getOptions();

    if (e.GetEventObject() == m_HFOVSlider) {
        opt.setHFOV(e.GetInt());
    } else if (e.GetEventObject() == m_VFOVSlider) {
        opt.setVFOV(e.GetInt());
    } else {
        int nParam = opt.m_projFeatures.numberOfParameters;
        std::vector<double> para = opt.getProjectionParameters();
        for (int i = 0; i < nParam; i++) {
            if (e.GetEventObject() == m_projParamSlider[i]) {
                // update
                para[i] = e.GetInt();
            }
        }
        opt.setProjectionParameters(para);
    }
    // we only actually update the panorama fully when the mouse is released.
    // As we are dragging it we don't want to create undo events, but we would
    // like to update the display, so we change the GLViewer's ViewState and
    // request a redraw.
    m_GLPreview->m_view_state->SetOptions(&opt);
    m_GLPreview->Refresh();
}

void GLPreviewFrame::OnBlendChoice(wxCommandEvent & e)
{
    if (e.GetEventObject() == m_BlendModeChoice)
    {
        updateBlendMode();
    }
    else
    {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}

void GLPreviewFrame::OnDragChoice(wxCommandEvent & e)
{
    if (e.GetEventObject() == m_DragModeChoice)
    {
        if (drag_tool) {
        
		    int index = m_DragModeChoice->GetSelection();
            switch (index) {
                case 0: //normal
                    drag_tool->setDragMode(PreviewDragTool::drag_mode_normal);
//		    		overview_drag_tool->setDragMode(PreviewDragTool::drag_mode_normal);
                    break;
                case 1: //mosaic
                    drag_tool->setDragMode(PreviewDragTool::drag_mode_mosaic);
//		    		overview_drag_tool->setDragMode(PreviewDragTool::drag_mode_mosaic);
                    break;
            }
            // adjust the layout
            GLPreviewFrame::DragChoiceLayout(index);
        }
    }
    else
    {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}

void GLPreviewFrame::OnOverviewModeChoice( wxCommandEvent & e)
{
    int choice = m_OverviewModeChoice->GetSelection();
    if (m_GLOverview->GetMode() == GLOverview::PLANE) {
        if (choice == 0) {
            unsigned int nr = m_pano.getNrOfImages();
            bool allowed = true;
            for (unsigned int i = 0 ; i < nr ; i++) {
                if (m_pano.getSrcImage(i).getX() != 0) allowed = false;
                if (m_pano.getSrcImage(i).getY() != 0) allowed = false;
                if (m_pano.getSrcImage(i).getZ() != 0) allowed = false;
            }
            if (allowed) {
                m_GLOverview->SetMode(GLOverview::PANOSPHERE);
            } else {
                wxMessageDialog dialog(this, 
                _("Switching to panosphere overview mode requires that all images have zero XYZ parameters. Do you want to set all XYZ parameters to zero for all images?"),   
                _("Reset XYZ parameters?"), wxYES_NO);
                if (dialog.ShowModal() == wxID_YES) {

                    UIntSet imgs;
                    Panorama newPan = m_pano.duplicate();
                    unsigned int nr = newPan.getNrOfImages();
                    for (unsigned int i = 0 ; i < nr ; i++) {
                        SrcPanoImage img = newPan.getSrcImage(i);
                        img.setX(0);
                        img.setY(0);
                        img.setZ(0);
                        newPan.setSrcImage(i,img);
                        imgs.insert(i);
                    }
                    GlobalCmdHist::getInstance().addCommand(
                        new PT::UpdateImagesVariablesCmd(m_pano, imgs, newPan.getVariables())
                    );
                    m_GLOverview->SetMode(GLOverview::PANOSPHERE);

                }
            }
        }
    } else {
        if (choice == 1) {
            m_GLOverview->SetMode(GLOverview::PLANE);
        }
    }
    m_GLOverview->m_visualization_state->ForceRequireRedraw();
    m_GLOverview->m_visualization_state->SetDirtyViewport();
}

void GLPreviewFrame::DragChoiceLayout( int index )
{
            // visibility of controls based on selected drag mode
            XRCCTRL(*this,"label_yaw",wxStaticText)->Show(index==0);
            XRCCTRL(*this,"input_yaw",wxTextCtrl)->Show(index==0);
            XRCCTRL(*this,"label_pitch",wxStaticText)->Show(index==0);
            XRCCTRL(*this,"input_pitch",wxTextCtrl)->Show(index==0);
            XRCCTRL(*this,"label_roll",wxStaticText)->Show(index==0);
            XRCCTRL(*this,"input_roll",wxTextCtrl)->Show(index==0);
            XRCCTRL(*this,"label_x",wxStaticText)->Show(index==1);
            XRCCTRL(*this,"input_x",wxTextCtrl)->Show(index==1);
            XRCCTRL(*this,"label_y",wxStaticText)->Show(index==1);
            XRCCTRL(*this,"input_y",wxTextCtrl)->Show(index==1);
            XRCCTRL(*this,"label_z",wxStaticText)->Show(index==1);
            XRCCTRL(*this,"input_z",wxTextCtrl)->Show(index==1);
            XRCCTRL(*this,"apply_num_transform",wxButton)->Show(1);
            // redraw layout to compress empty space
            XRCCTRL(*this,"label_yaw",wxStaticText)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"input_yaw",wxTextCtrl)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"label_pitch",wxStaticText)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"input_pitch",wxTextCtrl)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"label_roll",wxStaticText)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"input_roll",wxTextCtrl)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"label_x",wxStaticText)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"input_x",wxTextCtrl)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"label_y",wxStaticText)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"input_y",wxTextCtrl)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"label_z",wxStaticText)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"input_z",wxTextCtrl)->GetContainingSizer()->Layout();
            XRCCTRL(*this,"apply_num_transform",wxButton)->GetContainingSizer()->Layout();
}

void GLPreviewFrame::OnDefaultExposure( wxCommandEvent & e )
{
    if (m_pano.getNrOfImages() > 0) {
        PanoramaOptions opt = m_pano.getOptions();
        opt.outputExposureValue = calcMeanExposure(m_pano);
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                               );
    }
}

void GLPreviewFrame::OnIncreaseExposure( wxSpinEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    opt.outputExposureValue = opt.outputExposureValue + 1.0/3;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                            );
}

void GLPreviewFrame::OnDecreaseExposure( wxSpinEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    opt.outputExposureValue = opt.outputExposureValue - 1.0/3;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                           );
}

void GLPreviewFrame::OnProjectionChoice( wxCommandEvent & e )
{
    if (e.GetEventObject() == m_ProjectionChoice) {
        PanoramaOptions opt = m_pano.getOptions();
        int lt = m_ProjectionChoice->GetSelection();
        wxString Ip;
        opt.setProjection( (PanoramaOptions::ProjectionFormat) lt );
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                            );
        DEBUG_DEBUG ("Projection changed: "  << lt);
        m_projection_panel->Layout();
        Refresh();
    } else {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}

/* We don't have an OpenGL hdr display yet
void GLPreviewFrame::OnOutputChoice( wxCommandEvent & e)
{
    if (e.GetEventObject() == m_outputModeChoice) {
        PanoramaOptions opt = m_pano.getOptions();
        int lt = m_outputModeChoice->GetSelection();
        wxString Ip;
        opt.outputMode = ( (PanoramaOptions::OutputMode) lt );
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                               );

    } else {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}
*/

/** update the display */
void GLPreviewFrame::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<ProgressTask>::iterator it = tasks.begin();
         it != tasks.end(); ++it)
    {
        wxString cMsg;
        if (it->getProgress() > 0) {
            cMsg.Printf(wxT("%s [%3.0f%%]: %s "),
                        wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        100 * it->getProgress(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str());
        } else {
            cMsg.Printf(wxT("%s %s"),wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str());
        }
        // append to main message
        if (it == tasks.begin()) {
            msg = cMsg;
        } else {
            msg.Append(wxT(" | "));
            msg.Append(cMsg);
        }
    }
//    wxStatusBar *m_statbar = GetStatusBar();
    //DEBUG_TRACE("Statusmb : " << msg.mb_str(wxConvLocal));
    //m_statbar->SetStatusText(msg,0);

#ifdef __WXMSW__
    UpdateWindow(NULL);
#else
    // This is a bad call.. we just want to repaint the window, instead we will
    // process user events as well :( Unfortunately, there is not portable workaround...
//    wxYield();
#endif
}

void GLPreviewFrame::SetStatusMessage(wxString message)
{
    SetStatusText(message, 0);
}


void GLPreviewFrame::OnPhotometric(wxCommandEvent & e)
{
    m_GLPreview->SetPhotometricCorrect(e.IsChecked());
}

void GLPreviewFrame::MakePreviewTools(PreviewToolHelper *preview_helper_in)
{
    // create the tool objects.
    // we delay this until we have an OpenGL context so that they are free to
    // create texture objects and display lists before they are used.
    preview_helper = preview_helper_in;
    crop_tool = new PreviewCropTool(preview_helper);
    drag_tool = new PreviewDragTool(preview_helper);
    identify_tool = new PreviewIdentifyTool(preview_helper, this);
    difference_tool = new PreviewDifferenceTool(preview_helper);
    pano_mask_tool = new PreviewPanoMaskTool(preview_helper);
    preview_control_point_tool = new PreviewControlPointTool(preview_helper);
    m_preview_layoutLinesTool = new PreviewLayoutLinesTool(preview_helper);

    preview_projection_grid = new PreviewProjectionGridTool(preview_helper);
    if(m_previewGrid->GetValue())
    {
        preview_helper->ActivateTool(preview_projection_grid);
    };

//    
    // activate tools that are always active.
    preview_helper->ActivateTool(pano_mask_tool);
    // update the blend mode which activates some tools
    updateBlendMode();
    // update toolbar
    SetMode(mode_preview);
}

void GLPreviewFrame::MakePanosphereOverviewTools(PanosphereOverviewToolHelper *panosphere_overview_helper_in)
{
    panosphere_overview_helper = panosphere_overview_helper_in;
    overview_drag_tool = new OverviewDragTool(panosphere_overview_helper);
    panosphere_overview_camera_tool = new PanosphereOverviewCameraTool(panosphere_overview_helper);
    panosphere_overview_helper->ActivateTool(panosphere_overview_camera_tool);
    panosphere_overview_identify_tool = new PreviewIdentifyTool(panosphere_overview_helper, this);
    overview_projection_grid = new PanosphereOverviewProjectionGridTool(panosphere_overview_helper);
    panosphere_overview_helper->ActivateTool(overview_projection_grid);
    overview_outlines_tool = new PanosphereOverviewOutlinesTool(panosphere_overview_helper, m_GLPreview);
    panosphere_overview_helper->ActivateTool(overview_outlines_tool);
    panosphere_difference_tool = new PreviewDifferenceTool(panosphere_overview_helper);

    m_panosphere_layoutLinesTool = new PreviewLayoutLinesTool(panosphere_overview_helper);
    panosphere_control_point_tool = new PreviewControlPointTool(panosphere_overview_helper);



}

void GLPreviewFrame::MakePlaneOverviewTools(PlaneOverviewToolHelper *plane_overview_helper_in)
{
    plane_overview_helper = plane_overview_helper_in;
    plane_overview_identify_tool = new PreviewIdentifyTool(plane_overview_helper, this);
    plane_overview_camera_tool = new PlaneOverviewCameraTool(plane_overview_helper);
    plane_overview_helper->ActivateTool(plane_overview_camera_tool);
    plane_difference_tool = new PreviewDifferenceTool(plane_overview_helper);

    plane_overview_outlines_tool = new PlaneOverviewOutlinesTool(plane_overview_helper, m_GLPreview);
    plane_overview_helper->ActivateTool(plane_overview_outlines_tool);

    m_plane_layoutLinesTool = new PreviewLayoutLinesTool(plane_overview_helper);
    plane_control_point_tool = new PreviewControlPointTool(plane_overview_helper);

}

void GLPreviewFrame::OnIdentify(wxCommandEvent & e)
{
    SetStatusText(wxT(""), 0); // blank status text as it refers to an old tool.
    if (e.IsChecked())
    {
        m_BlendModeChoice->SetSelection(0);
        preview_helper->DeactivateTool(difference_tool);
        panosphere_overview_helper->DeactivateTool(panosphere_difference_tool);
        plane_overview_helper->DeactivateTool(plane_difference_tool);
        TurnOffTools(preview_helper->ActivateTool(identify_tool));
        TurnOffTools(panosphere_overview_helper->ActivateTool(panosphere_overview_identify_tool));
        TurnOffTools(plane_overview_helper->ActivateTool(plane_overview_identify_tool));
    } else {
        preview_helper->DeactivateTool(identify_tool);
        panosphere_overview_helper->DeactivateTool(panosphere_overview_identify_tool);
        plane_overview_helper->DeactivateTool(plane_overview_identify_tool);
        CleanButtonColours();
    }
    m_GLPreview->Refresh();
    m_GLOverview->Refresh();
}

void GLPreviewFrame::OnControlPoint(wxCommandEvent & e)
{
    SetStatusText(wxT(""), 0); // blank status text as it refers to an old tool.
    if (e.IsChecked())
    {
        TurnOffTools(preview_helper->ActivateTool(preview_control_point_tool));
        TurnOffTools(panosphere_overview_helper->ActivateTool(panosphere_control_point_tool));
        TurnOffTools(plane_overview_helper->ActivateTool(plane_control_point_tool));
    } else {
        preview_helper->DeactivateTool(preview_control_point_tool);
        panosphere_overview_helper->DeactivateTool(panosphere_control_point_tool);
        plane_overview_helper->DeactivateTool(plane_control_point_tool);
    }
    m_GLPreview->Refresh();
    m_GLOverview->Refresh();
}

void GLPreviewFrame::TurnOffTools(std::set<Tool*> tools)
{
    std::set<Tool*>::iterator i;
    for (i = tools.begin(); i != tools.end(); i++)
    {
        if (*i == crop_tool)
        {
            // cover up the guidelines
            m_GLPreview->Refresh();
        } else if (*i == drag_tool)
        {
            // cover up its boxes
            m_GLPreview->Refresh();
        } else if (*i == identify_tool)
        {
            // disabled the identify tool, toggle its button off.
            m_ToolBar_Identify->ToggleTool(XRCID("preview_identify_tool"), false);
            // cover up its indicators and restore normal button colours.
            m_GLPreview->Refresh();
            m_GLOverview->Refresh();
            CleanButtonColours();
        } else if (*i == preview_control_point_tool)
        {
            // disabled the control point tool.
            XRCCTRL(*this,"preview_control_point_tool",wxCheckBox)->SetValue(false);
            // cover up the control point lines.
            m_GLPreview->Refresh();
            m_GLOverview->Refresh();
        }
    }
}

void GLPreviewFrame::SetImageButtonColour(unsigned int image_nr,
                                          unsigned char red,
                                          unsigned char green,
                                          unsigned char blue)
{
    // 0, 0, 0 indicates we want to go back to the system colour.
    // TODO: Maybe we should test this better on different themes.
    // On OS X, the background colour is ignored on toggle buttons, but not
    // checkboxes.
    if (red || green || blue)
    {
        // the identify tool wants us to highlight an image button in the given
        // colour, to match up with the display in the preview.
#if defined __WXMSW__
        // on windows change the color of the surrounding wxPanel
        m_ToggleButtonPanel[image_nr]->SetBackgroundColour(wxColour(red, green, blue));
#else
        // change the color of the wxToggleButton 
        m_ToggleButtons[image_nr]->SetBackgroundStyle(wxBG_STYLE_COLOUR);
        m_ToggleButtons[image_nr]->SetBackgroundColour(
                                                    wxColour(red, green, blue));
        // black should be visible on the button's vibrant colours.
        m_ToggleButtons[image_nr]->SetForegroundColour(wxColour(0, 0, 0));
#endif
    } else {
        // return to the normal colour
#if defined __WXMSW__
        m_ToggleButtonPanel[image_nr]->SetBackgroundColour(this->GetBackgroundColour());
#else
        m_ToggleButtons[image_nr]->SetBackgroundStyle(wxBG_STYLE_SYSTEM);
        m_ToggleButtons[image_nr]->SetBackgroundColour(wxNullColour);
        m_ToggleButtons[image_nr]->SetForegroundColour(wxNullColour);
#endif
    }
#if defined __WXMSW__
    m_ToggleButtonPanel[image_nr]->Refresh();
#else
    m_ToggleButtons[image_nr]->Refresh();
#endif
}

void GLPreviewFrame::CleanButtonColours()
{
    // when we turn off the identification tool, any buttons that were coloured
    // to match the image in the preview should be given back the system themed
    // colours.
    unsigned int nr_images = m_pano.getNrOfImages();
    for (unsigned image = 0; image < nr_images; image++)
    {
#if defined __WXMSW__
        m_ToggleButtonPanel[image]->SetBackgroundColour(this->GetBackgroundColour());
        m_ToggleButtonPanel[image]->Refresh();
#else
        m_ToggleButtons[image]->SetBackgroundStyle(wxBG_STYLE_SYSTEM);
        m_ToggleButtons[image]->SetBackgroundColour(wxNullColour);
        m_ToggleButtons[image]->SetForegroundColour(wxNullColour);
        m_ToggleButtons[image]->Refresh();
#endif
    }
}

ImageToogleButtonEventHandler::ImageToogleButtonEventHandler(
                                  unsigned int image_number_in,
                                  PreviewIdentifyTool **identify_tool_in,
                                  wxToolBarToolBase* identify_toolbutton_in,
                                  PT::Panorama * m_pano_in)
{
    image_number = image_number_in;
    identify_tool = identify_tool_in;
    identify_toolbutton = identify_toolbutton_in;
    m_pano = m_pano_in;
}

void ImageToogleButtonEventHandler::OnEnter(wxMouseEvent & e)
{
    // When using the identify tool, we want to identify image locations when
    // the user moves the mouse over the image buttons, but only if the image
    // is being shown.
    if ( identify_toolbutton->IsToggled()
        && m_pano->getActiveImages().count(image_number))
    {
        (*identify_tool)->ShowImageNumber(image_number);
    }
    e.Skip();
}

void ImageToogleButtonEventHandler::OnLeave(wxMouseEvent & e)
{
    // if the mouse left one of the image toggle buttons with the identification
    // tool active, we should stop showing the image indicator for that button.
    if ( identify_toolbutton->IsToggled()
        && m_pano->getActiveImages().count(image_number))
    {
        (*identify_tool)->StopShowingImages();
    }
    e.Skip();
}

void ImageToogleButtonEventHandler::OnChange(wxCommandEvent & e)
{
    // the user is turning on or off an image using its button. We want to turn
    // the indicators on and off if appropriate correctly to. We use OnEnter
    // and OnLeave for the indicators, but these only work when the image is
    // showing, so we are carefull of the order:
    UIntSet activeImages = m_pano->getActiveImages();
    wxMouseEvent null_event;
    if (e.IsChecked()) {
        activeImages.insert(image_number);
      	GlobalCmdHist::getInstance().addCommand(
            new PT::SetActiveImagesCmd(*m_pano, activeImages)
        );
        OnEnter(null_event);
    } else {
        OnLeave(null_event);
        activeImages.erase(image_number);
      	GlobalCmdHist::getInstance().addCommand(
            new PT::SetActiveImagesCmd(*m_pano, activeImages)
        );
    }
}

/** call this method only with existing OpenGL context */
void GLPreviewFrame::FillBlendChoice()
{
    if(PreviewDifferenceTool::CheckOpenGLCanDifference())
        m_differenceIndex=m_BlendModeChoice->Append(_("difference"));
    // update size
    m_BlendModeChoice->InvalidateBestSize();
    m_BlendModeChoice->GetParent()->Layout();
    Refresh();
    // get blend mode last state
    unsigned int oldMode = wxConfigBase::Get()->Read(wxT("/GLPreviewFrame/blendMode"), 0l);
    // limit old state to max available states
    if (oldMode >= m_BlendModeChoice->GetCount())
    {
        oldMode = 0;
    }
    m_BlendModeChoice->SetSelection(oldMode);
    updateBlendMode();
};

void GLPreviewFrame::OnAutocrop(wxCommandEvent &e)
{
    DEBUG_INFO("Dirty ROI Calc\n");
    if (m_pano.getActiveImages().size() == 0) return;

    ProgressReporterDialog progress(2, _("Autocrop"), _("Calculating optimal crop"),this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
    progress.increaseProgress(1);
    progress.Pulse();
    
    vigra::Rect2D newROI;
    vigra::Size2D newSize;
    
    m_pano.calcOptimalROI(newROI, newSize);
    
    PanoramaOptions opt = m_pano.getOptions();
    
    DEBUG_INFO (   "ROI: left: " << opt.getROI().left()
                << " top: " << opt.getROI().top()
                << " right: " << opt.getROI().right()
                << " bottom: " << opt.getROI().bottom() << " before update");
    
    //set the ROI - fail if the right/bottom is zero, meaning all zero
    if(newROI.right() != 0 && newROI.bottom() != 0)
    {
        opt.setROI(newROI);

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd(m_pano, opt )
            );
    }
    PanoramaOptions opt2 = m_pano.getOptions();
    DEBUG_INFO (   "ROI: left: " << opt2.getROI().left()
                << " top: " << opt2.getROI().top()
                << " right: " << opt2.getROI().right()
                << " bottom: " << opt2.getROI().bottom() << " after update");
}

void GLPreviewFrame::OnFullScreen(wxCommandEvent & e)
{
    ShowFullScreen(!IsFullScreen(), wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
};

void GLPreviewFrame::OnUndo(wxCommandEvent &e)
{
    wxCommandEvent dummy(wxEVT_COMMAND_MENU_SELECTED, XRCID("ID_EDITUNDO"));
    m_parent->GetEventHandler()->AddPendingEvent(dummy);
};

void GLPreviewFrame::OnRedo(wxCommandEvent &e)
{
    wxCommandEvent dummy(wxEVT_COMMAND_MENU_SELECTED, XRCID("ID_EDITREDO"));
    m_parent->GetEventHandler()->AddPendingEvent(dummy);
};

void GLPreviewFrame::SetMode(int newMode)
{
    if(m_mode==newMode)
        return;
    SetStatusText(wxT(""), 0); // blank status text as it refers to an old tool.
    switch(m_mode)
    {
        case mode_preview:
            // switch off identify and show cp tool
            preview_helper->DeactivateTool(identify_tool);
            panosphere_overview_helper->DeactivateTool(panosphere_overview_identify_tool);
            plane_overview_helper->DeactivateTool(plane_overview_identify_tool);
            CleanButtonColours();
            m_ToolBar_Identify->ToggleTool(XRCID("preview_identify_tool"),false);
            preview_helper->DeactivateTool(preview_control_point_tool);
            panosphere_overview_helper->DeactivateTool(panosphere_control_point_tool);
            plane_overview_helper->DeactivateTool(plane_control_point_tool);
            XRCCTRL(*this,"preview_control_point_tool",wxCheckBox)->SetValue(false);
            break;
        case mode_layout:
            // disable layout mode.
            preview_helper->DeactivateTool(m_preview_layoutLinesTool);
            panosphere_overview_helper->DeactivateTool(m_panosphere_layoutLinesTool);
            plane_overview_helper->DeactivateTool(m_plane_layoutLinesTool);
            m_GLPreview->SetLayoutMode(false);
            m_GLOverview->SetLayoutMode(false);
            // Switch the panorama mask back on.
            preview_helper->ActivateTool(pano_mask_tool);
            //restore blend mode
            m_BlendModeChoice->SetSelection(non_layout_blend_mode);
            updateBlendMode();
            break;
        case mode_projection:
            break;
        case mode_drag:
            preview_helper->DeactivateTool(drag_tool);
            panosphere_overview_helper->DeactivateTool(overview_drag_tool);
            break;
        case mode_crop:
            preview_helper->DeactivateTool(crop_tool);
            break;
    };
    m_mode=newMode;
    wxScrollEvent dummy;
    switch(m_mode)
    {
        case mode_preview:
            break;
        case mode_layout:
            //save blend mode setting, set to normal for layout mode
            non_layout_blend_mode=m_BlendModeChoice->GetSelection();
            m_BlendModeChoice->SetSelection(0);
            updateBlendMode();
            // turn off things not used in layout mode.
            preview_helper->DeactivateTool(pano_mask_tool);
            m_GLPreview->SetLayoutMode(true);
            m_GLOverview->SetLayoutMode(true);
            preview_helper->ActivateTool(m_preview_layoutLinesTool);
            panosphere_overview_helper->ActivateTool(m_panosphere_layoutLinesTool);
            plane_overview_helper->ActivateTool(m_plane_layoutLinesTool);
            // we need to update the meshes after switch to layout mode
            // otherwise the following update of scale has no meshes to scale
            m_GLPreview->Update();
            OnLayoutScaleChange(dummy);
            break;
        case mode_projection:
            break;
        case mode_drag:
            TurnOffTools(preview_helper->ActivateTool(drag_tool));
            TurnOffTools(panosphere_overview_helper->ActivateTool(overview_drag_tool));
            break;
        case mode_crop:
            TurnOffTools(preview_helper->ActivateTool(crop_tool));
            break;
    };
    m_GLPreview->Refresh();
};

void GLPreviewFrame::OnSelectMode(wxNotebookEvent &e)
{
    if(m_mode!=-1)
        SetMode(e.GetSelection());
};

void GLPreviewFrame::OnToolModeChanging(wxNotebookEvent &e)
{
    if(m_pano.getNrOfImages()==0 && e.GetOldSelection()==0)
    {
        wxBell();
        e.Veto();
    };
};

void GLPreviewFrame::OnROIChanged ( wxCommandEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    long left, right, top, bottom;
    if (!m_ROITopTxt->GetValue().ToLong(&top)) {
        wxLogError(_("Top needs to be an integer bigger than 0"));
        return;
    }
    if (!m_ROILeftTxt->GetValue().ToLong(&left)) {
        wxLogError(_("left needs to be an integer bigger than 0"));
        return;
    }
    if (!m_ROIRightTxt->GetValue().ToLong(&right)) {
        wxLogError(_("right needs to be an integer bigger than 0"));
        return;
    }
    if (!m_ROIBottomTxt->GetValue().ToLong(&bottom)) {
        wxLogError(_("bottom needs to be an integer bigger than 0"));
        return;
    }
    // make sure that left is really to the left of right
    if(left>=right) {
        wxLogError(_("left boundary must be smaller than right"));
		// TODO: would be nice if the previous value would be restored
        return;
    }
    // make sure that top is really higher than bottom
    if(top>=bottom) {
        wxLogError(_("top boundary must be smaller than bottom"));
		// TODO: would be nice if the previous value would be restored
        return;
    }


    opt.setROI(vigra::Rect2D(left, top, right, bottom));
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                           );
};

void GLPreviewFrame::OnHFOVChanged ( wxCommandEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();


    wxString text = m_HFOVText->GetValue();
    DEBUG_INFO ("HFOV = " << text.mb_str(wxConvLocal) );
    if (text == wxT("")) {
        return;
    }

    double hfov;
    if (!str2double(text, hfov)) {
        wxLogError(_("Value must be numeric."));
        return;
    }

    if ( hfov <=0 || hfov > opt.getMaxHFOV()) {
        wxLogError(wxString::Format(
            _("Invalid HFOV value. Maximum HFOV for this projection is %lf."),
            opt.getMaxHFOV()));
        hfov=opt.getMaxHFOV();
    }
    opt.setHFOV(hfov);
    // recalculate panorama height...
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );

    DEBUG_INFO ( "new hfov: " << hfov )
};

void GLPreviewFrame::OnVFOVChanged ( wxCommandEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();

    wxString text = m_VFOVText->GetValue();
    DEBUG_INFO ("VFOV = " << text.mb_str(wxConvLocal) );
    if (text == wxT("")) {
        return;
    }

    double vfov;
    if (!str2double(text, vfov)) {
        wxLogError(_("Value must be numeric."));
        return;
    }

    if ( vfov <=0 || vfov > opt.getMaxVFOV()) {
        wxLogError(wxString::Format(
            _("Invalid VFOV value. Maximum VFOV for this projection is %lf."),
            opt.getMaxVFOV()));
        vfov = opt.getMaxVFOV();
    }
    opt.setVFOV(vfov);
    // recalculate panorama height...
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );

    DEBUG_INFO ( "new vfov: " << vfov )
};

void GLPreviewFrame::OnLayoutScaleChange(wxScrollEvent &e)
{
    if(m_mode==mode_layout)
    {
        double scale_factor=XRCCTRL(*this,"layout_scale_slider",wxSlider)->GetValue();
        m_GLPreview->SetLayoutScale(10.0 - sqrt(scale_factor));
        m_GLOverview->SetLayoutScale(10.0 - sqrt(scale_factor));
        m_GLPreview->Refresh();
        m_GLOverview->Refresh();
    };
};

void GLPreviewFrame::ShowProjectionWarnings()
{
    PanoramaOptions opts = m_pano.getOptions();
    double hfov = opts.getHFOV();
    double vfov = opts.getVFOV();
    double maxfov = hfov > vfov ? hfov : vfov;
    wxString message;
    // If this is set to true, offer rectilinear as an alternative if it fits.
    bool rectilinear_option = false;
    switch (opts.getProjection()) {
        case HuginBase::PanoramaOptions::RECTILINEAR:
            if (maxfov > 120.0) {
                            // wide rectilinear image
                message = _("With a wide field of view, panoramas with rectilinear projection get very stretched towards the edges.\n");
                if (vfov < 110) {
                    message += _("Since the field of view is only very wide in the horizontal direction, try a cylindrical projection instead.");
                } else {
                    message += _("For a very wide panorama, try equirectangular projection instead.");
                }
                message += _(" You could also try Panini projection.");
            }
            break;
        case HuginBase::PanoramaOptions::CYLINDRICAL:
            if (vfov > 120.0) {
                message = _("With a wide vertical field of view, panoramas with cylindrical projection get very stretched at the top and bottom.\nAn equirectangular projection would fit the same content in less vertical space.");
            } else rectilinear_option = true;
            break;
        case HuginBase::PanoramaOptions::EQUIRECTANGULAR:
            if (vfov < 110.0 && hfov > 120.0)
            {
                message = _("Since the vertical field of view is not too wide, you could try setting the panorama projection to cylindrical.\nCylindrical projection preserves vertical lines, unlike equirectangular.");
            } else rectilinear_option = true;
            break;
        case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
            if (maxfov < 280.0) {
                rectilinear_option = true;
                message = _("Stereographic projection is conformal, unlike this Fisheye panorama projection.\nA conformal projection preserves angles around a point, which often makes it easier on the eye.");
            }
            break;
        case HuginBase::PanoramaOptions::STEREOGRAPHIC:
            if (maxfov > 300.0) {
                message = _("Panoramas with stereographic projection and a very wide field of view stretch the image around the edges a lot.\nThe Fisheye panorama projection compresses it, so you can fit in a wide field of view and still have a reasonable coverage of the middle.");
            } else rectilinear_option = true;
            break;
        default:
            rectilinear_option = true;
    }
    if (rectilinear_option && maxfov < 110.0) {
        message = _("Setting the panorama to rectilinear projection would keep the straight lines straight.");
    }
    if (message.IsEmpty()) {
        // no message needed.
#if wxCHECK_VERSION(2, 9, 1)
        m_infoBar->Dismiss();
#else
        if (m_projectionStatusPushed) {
            m_projectionStatusPushed = false;
            GetStatusBar()->PopStatusText();
        }
#endif
    } else {
#if wxCHECK_VERSION(2, 9, 1)
        /** @todo If the projection information bar was closed manually, don't show any more messages there.
         * It should probably be stored as a configuration setting so it persits
         * until "Load defaults" is selected on the preferences window.
         */
        m_infoBar->ShowMessage(message, wxICON_INFORMATION);
#else
        if (m_projectionStatusPushed) {
            GetStatusBar()->PopStatusText();
        }
        /** @todo The message doesn't really fit in the status bar, so we should have some other GUI arrangement or remove this feature.
         * On my system, the status bar remains too short to contain the two
         * lines of text in the message.
         */
        GetStatusBar()->PushStatusText(message);
        m_projectionStatusPushed = true;
#endif
    }
};
