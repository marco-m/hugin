// -*- c-basic-offset: 4 -*-
/**  @file RawImport.h
 *
 *  @brief Definition of dialog and functions to import RAW images to project file
 *
 *  @author T. Modes
 *
 */

/*  This is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _RAW_IMPORT_H
#define _RAW_IMPORT_H

#include "panoinc_WX.h"
#include "panoinc.h"

/** Dialog for raw import */
class RawImportDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings and position */
    RawImportDialog(wxWindow *parent, HuginBase::Panorama* pano);
    /** destructor, saves position */
    ~RawImportDialog();

protected:
    /** called when dialog is finished and does the conversion */
    void OnOk(wxCommandEvent & e);
    /** event handler for adding image(s) */
    void OnAddImages(wxCommandEvent& e);
    /** event handler for removing selected image from list */
    void OnRemoveImage(wxCommandEvent& e);
    /** event handler to mark current selected image as reference image for white balance */
    void OnSetWBReference(wxCommandEvent& e);
    /** event handler called when dialog is shown */
    void OnInitDialog(wxInitDialogEvent& e);
    /** event handler for selection of exe and history stack in dialog */
    void OnSelectDCRAWExe(wxCommandEvent& e);
    void OnSelectRTExe(wxCommandEvent& e);
    void OnSelectRTHistoryStack(wxCommandEvent& e);
    void OnSelectDarktableExe(wxCommandEvent& e);

private:
    /** fill list with image names */
    void FillImageList();
    /** show dialog for selection of exe or history stack */
    void OnSelectFile(wxCommandEvent& e, const wxString& caption, const wxString& filter, const char* id);
    HuginBase::Panorama* m_pano;
    wxArrayString m_rawImages;
    wxArrayString m_images;
    std::string m_camera;
    int m_refImg = -1;

    DECLARE_EVENT_TABLE()
};

#endif //_RAW_IMPORT_H
