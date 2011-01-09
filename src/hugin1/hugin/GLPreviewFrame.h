// -*- c-basic-offset: 4 -*-
/** @file GLPreviewFrame.h
 *
 *  @author James Legg and Pablo d'Angelo <pablo.dangelo@web.de>
 *  @author Darko Makreshanski
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
class GLPreview;
class GLOverview;
class ViewState;
class wxSpinEvent;
class wxChoice;
#if wxCHECK_VERSION(2,9,1)
//forward declaration for wxInfoBar works only for wxGTK
//for other systems wxInfoBar is defined as preprocessor macro and not as class
//class wxInfoBar;
#include <wx/infobar.h>
#endif

class MeshManager;

class ToolHelper;
class PreviewToolHelper;
class PanosphereOverviewToolHelper;
class PlaneOverviewToolHelper;
class Tool;
class PreviewTool;
class PreviewCropTool;
class OverviewDragTool;
class PanosphereOverviewCameraTool;
class PlaneOverviewCameraTool;
class PreviewDragTool;
class PreviewIdentifyTool;
class PreviewDifferenceTool;
class PreviewPanoMaskTool;
class PreviewControlPointTool;
class PreviewLayoutLinesTool;

class PanosphereOverviewProjectionGridTool;
class PreviewProjectionGridTool;

class PanosphereOverviewOutlinesTool;
class PlaneOverviewOutlinesTool;

class GLPreviewFrame;

class GLwxAuiManager;
class GLwxAuiFloatingFrame;

#include "common/utils.h"
#include <wx/string.h>
#include <wx/frame.h>
#include <wx/aui/aui.h>

#include <iostream>

// the image toggle buttons need a special event handler to trap mouse enter and
// leave events.
class ImageToogleButtonEventHandler : public wxEvtHandler
{
public:
    ImageToogleButtonEventHandler(unsigned int image_number,
                                  PreviewIdentifyTool **identify_tool,
                                  wxToolBarToolBase* identify_toolbutton_in,
                                  PT::Panorama * m_pano);
    void OnChange(wxCommandEvent &e);
protected:
    void OnEnter(wxMouseEvent & e);
    void OnLeave(wxMouseEvent & e);
private:
    DECLARE_EVENT_TABLE()
    unsigned int image_number;
    PreviewIdentifyTool **identify_tool;
    wxToolBarToolBase *identify_toolbutton;
    PT::Panorama * m_pano;
};


/**
 * subclass for a floating frame of the dock manager
 */
class GLwxAuiFloatingFrame : public wxAuiFloatingFrame {
public:
    GLwxAuiFloatingFrame(wxWindow* parent,
                   GLwxAuiManager* owner_mgr,
                   const wxAuiPaneInfo& pane,
                   wxWindowID id = wxID_ANY,
                   long style = wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION |
//                                wxFRAME_NO_TASKBAR | 
                                wxFRAME_FLOAT_ON_PARENT | 
                                wxCLIP_CHILDREN
                   ) : wxAuiFloatingFrame(parent, (wxAuiManager*) owner_mgr, pane, id, style) {}


    void OnActivate(wxActivateEvent& evt);
    void OnMoveFinished();
//    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()

};

/**
 * customized subclass of the dock manager, created just for the purpose to create a workaround for the bug that exists while using wxAUI and OpenGL
 * the bug is similar to the one described here http://www.kirix.com/forums/viewtopic.php?f=15&t=175
 */
class GLwxAuiManager : public wxAuiManager {
public:
    GLwxAuiManager(GLPreviewFrame* frame, GLPreview * preview, GLOverview * overview) : frame(frame), preview(preview), overview(overview) {}
    GLwxAuiFloatingFrame* CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& p);
    GLPreviewFrame * getPreviewFrame() {return frame;}
    GLPreview * getGLPreview() {return preview;}
    GLOverview * getGLOverview() {return overview;}

    bool ProcessDockResult(wxAuiPaneInfo& target,
                                   const wxAuiPaneInfo& new_pos);

private:
    GLPreviewFrame * frame;
    GLPreview * preview;
    GLOverview * overview;
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
    
    void MakePreviewTools(PreviewToolHelper * helper);
    void MakePanosphereOverviewTools(PanosphereOverviewToolHelper * helper);
    void MakePlaneOverviewTools(PlaneOverviewToolHelper* helper);
    
    void SetImageButtonColour(unsigned int image_nr, unsigned char red,
                              unsigned char green, unsigned char blue);
    void SetStatusMessage(wxString message);
    /** fills the blend wxChoice with all valid blend modes and restore the last used one
     */
    void FillBlendChoice();
    /** loads the layout of the OpenGL windows and restores it */
    void LoadOpenGLLayout();

    GLwxAuiManager* getAuiManager() {return m_mgr;}
    GLPreview* getPreview() {return m_GLPreview;}
    GLOverview* getOverview() {return m_GLOverview;}

    void PauseResize();
    void ContinueResize();
    bool CanResize() {return GLresize;}

    /** Display an updated version of the preview images.
     *  Redraws happen automatically when the panorama changes, and when the 
     *  preview's internal real time sliders are used. This is only needed
     *  occasionally, such as when a image finishes loading and its place holder
     *  can be replaced with the real image.
     */
    void redrawPreview();
    /** set status if projection hints should be shown or not*/
    void SetShowProjectionHints(bool new_value);

protected:

    bool GLresize;

    void OnClose(wxCloseEvent& e);
    void OnShowEvent(wxShowEvent& e);

    void OnOverviewToggle(wxCommandEvent& e);

    void OnCenterHorizontally(wxCommandEvent & e);
    void OnFitPano(wxCommandEvent& e);
    void OnStraighten(wxCommandEvent & e);
    void OnShowAll(wxCommandEvent & e);
    void OnShowNone(wxCommandEvent & e);
    void OnPhotometric(wxCommandEvent & e);
    void OnIdentify(wxCommandEvent &e);
    void OnAutocrop(wxCommandEvent &e);
    void OnControlPoint(wxCommandEvent &e);
    void OnNumTransform(wxCommandEvent & e);
    void OnChangeFOV(wxScrollEvent & e);
    void OnTrackChangeFOV(wxScrollEvent & e);
    void OnExposureChanged(wxCommandEvent & e);
    void OnProjParameterChanged(wxCommandEvent & e);
    /** event handler for reset projection parameters */
    void OnProjParameterReset(wxCommandEvent & e);
    /** event handler for switch on/off grid on preview */
    void OnSwitchPreviewGrid(wxCommandEvent & e);

    void OnDefaultExposure( wxCommandEvent & e );
    void OnDecreaseExposure( wxSpinEvent & e );
    void OnIncreaseExposure( wxSpinEvent & e );

    void OnBlendChoice(wxCommandEvent & e);
    void OnDragChoice(wxCommandEvent & e);
    void DragChoiceLayout( int index );
    void OnProjectionChoice(wxCommandEvent & e);
    void OnOverviewModeChoice(wxCommandEvent & e);
    /** event handler for changed roi */
    void OnROIChanged(wxCommandEvent & e);
    void OnHFOVChanged(wxCommandEvent & e);
    void OnVFOVChanged(wxCommandEvent & e);
    /** event handler when user hides the infobar */
    void OnHideProjectionHints(wxCommandEvent &e);
    // No HDR display yet
    // void OnOutputChoice(wxCommandEvent & e);
    // update tools according to blend mode choice
    void updateBlendMode();
    // update the panorama display
    void updatePano();
    /** event handler for full screen */
    void OnFullScreen(wxCommandEvent &e);
    /** event handler for undo */
    void OnUndo(wxCommandEvent &e);
    /** event handler for redo */
    void OnRedo(wxCommandEvent &e);
    /** event handler for selection of new mode */
    void OnSelectMode(wxNotebookEvent &e);
    /** event handler for blocking changing mode when panorama contains no images*/
    void OnToolModeChanging(wxNotebookEvent &e);
    /** event handler for change scale of layout mode */
    void OnLayoutScaleChange(wxScrollEvent &e);
private:

    /** The dock manager */
    GLwxAuiManager * m_mgr;

    void SetMode(int newMode);
    PT::Panorama & m_pano;

    GLPreview * m_GLPreview;
    GLOverview * m_GLOverview;

    ViewState* m_view_state;

    int m_mode;
    int non_layout_blend_mode;
    wxToolBar* m_ToolBar_Identify;
    wxNotebook* m_tool_notebook;
    wxPanel* m_projection_panel;
    wxSlider * m_HFOVSlider;
    wxSlider * m_VFOVSlider;
    wxTextCtrl * m_HFOVText;
    wxTextCtrl * m_VFOVText;
    wxTextCtrl * m_ROILeftTxt;
    wxTextCtrl * m_ROIRightTxt;
    wxTextCtrl * m_ROITopTxt;
    wxTextCtrl * m_ROIBottomTxt;
    wxChoice * m_BlendModeChoice;
    wxChoice * m_DragModeChoice;
    wxChoice * m_ProjectionChoice;
    wxChoice * m_OverviewModeChoice;
    // No HDR display yet.
    // wxChoice * m_outputModeChoice;
    wxTextCtrl * m_exposureTextCtrl;
    wxBitmapButton * m_defaultExposureBut;
    wxSpinButton * m_exposureSpinBut;
    wxCheckBox * m_previewGrid;
#if wxCHECK_VERSION(2, 9, 1)
    /// Bar for context sensitive projection information.
    wxInfoBar * m_infoBar;
#else
    // True if the status bar text has been replaced with projection information
    bool m_projectionStatusPushed;
#endif

    wxString m_choices[3];
    int m_oldProjFormat;
    // index of difference mode
    int m_differenceIndex;

	  wxScrolledWindow * m_ButtonPanel;
	  wxBoxSizer * m_ButtonSizer;
	  wxStaticBoxSizer * m_ToggleButtonSizer;

    wxBoxSizer * m_topsizer;
    wxBoxSizer * m_projParamSizer;
    std::vector<wxStaticText *> m_projParamNamesLabel;
    std::vector<wxTextCtrl *>   m_projParamTextCtrl;
    std::vector<wxSlider *>     m_projParamSlider;

#ifdef USE_TOGGLE_BUTTON
    std::vector<wxToggleButton *> m_ToggleButtons;
#else
    std::vector<wxCheckBox *> m_ToggleButtons;
#endif
    std::vector<wxPanel *> m_ToggleButtonPanel;
    std::vector<ImageToogleButtonEventHandler *> toogle_button_event_handlers;

    wxToggleButton * m_OverviewToggle;
    
    DECLARE_EVENT_TABLE()

    // tools
    PreviewToolHelper *preview_helper;
    
    PreviewCropTool *crop_tool;
    PreviewDragTool *drag_tool;

    PreviewIdentifyTool *identify_tool;
    PreviewIdentifyTool *panosphere_overview_identify_tool;
    PreviewIdentifyTool *plane_overview_identify_tool;

    PreviewDifferenceTool *difference_tool;
    PreviewDifferenceTool *plane_difference_tool;
    PreviewDifferenceTool *panosphere_difference_tool;
    
    PreviewControlPointTool *preview_control_point_tool;
    PreviewControlPointTool *panosphere_control_point_tool;
    PreviewControlPointTool *plane_control_point_tool;

    PreviewPanoMaskTool *pano_mask_tool;    

    bool m_showProjectionHints;
    PreviewLayoutLinesTool *m_preview_layoutLinesTool;
    PreviewLayoutLinesTool *m_panosphere_layoutLinesTool;
    PreviewLayoutLinesTool *m_plane_layoutLinesTool;

    PanosphereOverviewProjectionGridTool * overview_projection_grid;
    PreviewProjectionGridTool * preview_projection_grid;

    PanosphereOverviewToolHelper *panosphere_overview_helper;

    PlaneOverviewToolHelper *plane_overview_helper;

    OverviewDragTool *overview_drag_tool;

    PanosphereOverviewCameraTool *panosphere_overview_camera_tool;
    PlaneOverviewCameraTool *plane_overview_camera_tool;
    
    PanosphereOverviewOutlinesTool *overview_outlines_tool;
    PlaneOverviewOutlinesTool *plane_overview_outlines_tool;

    void TurnOffTools(std::set<Tool*> tools);
    
    void CleanButtonColours();
    /** Tell the user anything suspicious about the projection choice.
     * If nothing is suspicious, any previous message is removed.
     * In wxWidgets 2.9, this appears as an wxInfoBar. Older versions do not
     * have this. so the status bar is used instead.
     */
    void ShowProjectionWarnings();
};


#endif // _GLPREVIEWFRAME_H
