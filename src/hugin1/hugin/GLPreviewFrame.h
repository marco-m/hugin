// -*- c-basic-offset: 4 -*-
/** @file GLPreviewFrame.h
 *
 *  @author James Legg and Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _GLPREVIEWFRAME_H
#define _GLPREVIEWFRAME_H

class GLRenderer;
class wxToolBar;
class wxToggleButton;
class wxCheckBox;
class wxTextCtrl;
class wxBitmapButton;
class wxSpinButton;
class wxScrolledWindow;
class wxBoxSizer;
class wxStaticBoxSizer;
class wxStaticText;
class wxSlider;
class GLViewer;
class wxSpinEvent;
class wxChoice;

class PreviewToolHelper;
class PreviewTool;
class PreviewCropTool;
class PreviewDragTool;
class PreviewIdentifyTool;
class PreviewDifferenceTool;
class PreviewPanoMaskTool;

#include "common/utils.h"
#include <wx/string.h>
#include <wx/frame.h>

// the image toggle buttons need a special event handler to trap mouse enter and
// leave events.
class ImageToogleButtonEventHandler : public wxEvtHandler
{
public:
    ImageToogleButtonEventHandler(unsigned int image_number,
                                  PreviewIdentifyTool **identify_tool,
                                  unsigned int identify_tool_id,
                                  wxToolBar *tool_bar,
                                  PT::Panorama * m_pano);
    void OnChange(wxCommandEvent &e);
protected:
    void OnEnter(wxMouseEvent & e);
    void OnLeave(wxMouseEvent & e);
private:
    DECLARE_EVENT_TABLE()
    unsigned int image_number;
    PreviewIdentifyTool **identify_tool;
    unsigned int identify_tool_id;
    wxToolBar *tool_bar;
    PT::Panorama * m_pano;
};

/** The OpenGL preview frame
 *
 *  Contains the GLViewer and various controls for it.
 *
 *  it is not created with XRC, because it is highly dynamic, buttons
 *  have to be added etc.
 */
class GLPreviewFrame : public wxFrame, public PT::PanoramaObserver, public utils::MultiProgressDisplay
{
public:

    /** ctor.
     */
    GLPreviewFrame(wxFrame * frame, PT::Panorama &pano);

    /** dtor.
     */
    virtual ~GLPreviewFrame();

    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet &changed);
    
    void updateProgressDisplay();
    
    void MakeTools(PreviewToolHelper * helper);
    void SetImageButtonColour(unsigned int image_nr, unsigned char red,
                              unsigned char green, unsigned char blue);
    void SetStatusMessage(wxString message);
protected:
    void OnClose(wxCloseEvent& e);

    void OnCenterHorizontally(wxCommandEvent & e);
    void OnFitPano(wxCommandEvent& e);
    void OnStraighten(wxCommandEvent & e);
    void OnShowAll(wxCommandEvent & e);
    void OnShowNone(wxCommandEvent & e);
    void OnPhotometric(wxCommandEvent & e);
    void OnCrop(wxCommandEvent & e);
    void OnDrag(wxCommandEvent & e);
    void OnIdentify(wxCommandEvent &e);
    void OnNumTransform(wxCommandEvent & e);
    void OnChangeFOV(wxScrollEvent & e);
    void OnTrackChangeFOV(wxScrollEvent & e);
    void OnTextCtrlChanged(wxCommandEvent & e);

    void OnDefaultExposure( wxCommandEvent & e );
    void OnDecreaseExposure( wxSpinEvent & e );
    void OnIncreaseExposure( wxSpinEvent & e );

    void OnBlendChoice(wxCommandEvent & e);
    void OnProjectionChoice(wxCommandEvent & e);
    // No HDR display yet
    // void OnOutputChoice(wxCommandEvent & e);

    // update the panorama display
    void updatePano();
private:

    PT::Panorama & m_pano;

    GLViewer * m_GLViewer;
    wxToolBar * m_ToolBar;
    int drag_tool_id, crop_tool_id, identify_tool_id;
    wxSlider * m_HFOVSlider;
    wxSlider * m_VFOVSlider;
    wxChoice * m_BlendModeChoice;
    wxChoice * m_ProjectionChoice;
    // No HDR display yet.
    // wxChoice * m_outputModeChoice;
    wxTextCtrl * m_exposureTextCtrl;
    wxBitmapButton * m_defaultExposureBut;
    wxSpinButton * m_exposureSpinBut;

    wxString m_choices[3];
    int m_oldProjFormat;

	  wxScrolledWindow * m_ButtonPanel;
	  wxBoxSizer * m_ButtonSizer;
	  wxStaticBoxSizer * m_ToggleButtonSizer;

    wxBoxSizer * m_topsizer;
    wxStaticBoxSizer * m_projParamSizer;
    std::vector<wxStaticText *> m_projParamNamesLabel;
    std::vector<wxTextCtrl *>   m_projParamTextCtrl;
    std::vector<wxSlider *>     m_projParamSlider;

#ifdef USE_TOGGLE_BUTTON
    std::vector<wxToggleButton *> m_ToggleButtons;
#else
    std::vector<wxCheckBox *> m_ToggleButtons;
#endif
    std::vector<ImageToogleButtonEventHandler *> toogle_button_event_handlers;
    DECLARE_EVENT_TABLE()
    
    // tools
    PreviewToolHelper *helper;
    PreviewCropTool *crop_tool;
    PreviewDragTool *drag_tool;
    PreviewIdentifyTool *identify_tool;
    PreviewDifferenceTool *difference_tool;
    PreviewPanoMaskTool *pano_mask_tool;
    void TurnOffTools(std::set<PreviewTool*> tools);
    void CleanButtonColours();
};


#endif // _GLPREVIEWFRAME_H
