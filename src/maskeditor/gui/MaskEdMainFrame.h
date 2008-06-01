// -*- c-basic-offset: 4 -*-
/** @file MaskEdMainFrame.h
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id: MaskEdMainFrame.h  $
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


class MaskEdMainFrame : public wxFrame 
{
public:
    MaskEdMainFrame(wxWindow *parent);
    
    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnLoadPTO(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnBStroke(wxCommandEvent& event);
    void OnAddPoint(wxCommandEvent& event);
    void OnPreference(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void OnMotion(wxMouseEvent& event);

    DECLARE_EVENT_TABLE();
};
#endif
