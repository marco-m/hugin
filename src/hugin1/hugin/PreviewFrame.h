// -*- c-basic-offset: 4 -*-
/** @file PreviewFrame.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _PREVIEWFRAME_H
#define _PREVIEWFRAME_H

class PreviewPanel;
class wxToolBar;
class wxToggleButton;
class wxCheckBox;
class MaskEdEditWnd;
/** The image preview frame
 *
 *  Contains the ImagePreviewPanel and various controls for it.
 *
 *  it is not created with XRC, because it is highly dynamic, buttons
 *  have to be added etc.
 */
class PreviewFrame : public wxFrame, public PT::PanoramaObserver, public utils::MultiProgressDisplay
{
public:

    /** ctor.
     */
    PreviewFrame(wxFrame * frame, PT::Panorama &pano);

    /** dtor.
     */
    virtual ~PreviewFrame();

    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet &changed);
    void OnUpdate(wxCommandEvent& event);
    
    void updateProgressDisplay();
    void initPreviewMode();
    void initMaskEditorMode();
    void setPreviewMode();
    void setMaskEditorMode();

protected:
    void ForceUpdate();

    void OnClose(wxCloseEvent& e);

    void OnMaskEditor(wxCommandEvent & e);
    void OnExitMaskEditor(wxCommandEvent &e);
    void OnChangeDisplayedImgs(wxCommandEvent & e);
    void OnAutoPreviewToggle(wxCommandEvent & e);
    void OnCenterHorizontally(wxCommandEvent & e);
    void OnFitPano(wxCommandEvent& e);
    void OnStraighten(wxCommandEvent & e);
    void OnShowAll(wxCommandEvent & e);
    void OnShowNone(wxCommandEvent & e);
    void OnNumTransform(wxCommandEvent & e);
    void OnChangeFOV(wxScrollEvent & e);
    void OnTextCtrlChanged(wxCommandEvent & e);

    void OnDefaultExposure( wxCommandEvent & e );
    void OnDecreaseExposure( wxSpinEvent & e );
    void OnIncreaseExposure( wxSpinEvent & e );

    void OnBlendChoice(wxCommandEvent & e);
    void OnProjectionChoice(wxCommandEvent & e);
    void OnOutputChoice(wxCommandEvent & e);

    // update the panorama display
    void updatePano();

    // mask editor functionalities
    void OnBStroke(wxCommandEvent& event);
    void OnAddPoint(wxCommandEvent& event);
    //void OnPreference(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnSetROI(wxCommandEvent& event);
    void OnShowOverlap(wxCommandEvent& event);
    void OnSegSelUpdate(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
private:
    enum tMode { PREVIEW_MODE, MASKEDITOR_MODE };
    float          m_scale;

    PT::Panorama & m_pano;

    MaskEdEditWnd *m_MaskEdEditWnd;
    PreviewPanel * m_PreviewPanel;
    wxToolBar * m_ToolBar;
    wxSlider * m_HFOVSlider;
    wxSlider * m_VFOVSlider;
    wxChoice * m_BlendModeChoice;
    wxChoice * m_ProjectionChoice;
    wxChoice * m_outputModeChoice;
    wxTextCtrl * m_exposureTextCtrl;
    wxBitmapButton * m_defaultExposureBut;
    wxSpinButton * m_exposureSpinBut;

    wxString m_choices[3];
    int m_oldProjFormat;
    tMode m_mode;
//    wxButton * m_updatePreview;
//    wxCheckBox * m_autoCB;

	wxScrolledWindow * m_ButtonPanel;
	wxBoxSizer * m_ButtonSizer;
	wxStaticBoxSizer * m_ToggleButtonSizer;

    wxBoxSizer * m_topsizer;
    wxFlexGridSizer * m_flexSizer;
    wxStaticBoxSizer * m_blendModeSizer;
    wxStaticBoxSizer * m_projParamSizer;
    std::vector<wxStaticText *> m_projParamNamesLabel;
    std::vector<wxTextCtrl *>   m_projParamTextCtrl;
    std::vector<wxSlider *>     m_projParamSlider;

#ifdef USE_TOGGLE_BUTTON
    std::vector<wxToggleButton *> m_ToggleButtons;
#else
    std::vector<wxCheckBox *> m_ToggleButtons;
#endif

    DECLARE_EVENT_TABLE()
};



#endif // _PREVIEWFRAME_H
