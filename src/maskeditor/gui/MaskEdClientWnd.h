// -*- c-basic-offset: 4 -*-
/** @file MaskEdClientWnd.h
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Rev$
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
#ifndef MASKEDCLIENTWND_H
#define MASKEDCLIENTWND_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include "MaskEditingWnd.h"
#include "MaskEdPreviewWnd.h"

class MaskEdClientWnd : public wxSplitterWindow
{
    MaskEditingWnd  *m_MaskEditingWnd;
    MaskEdPreviewWnd  *m_MaskEdPreviewWnd;
    wxString         m_Filename;
public:
    MaskEdClientWnd(wxWindow *parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxSP_3D|wxSP_LIVE_UPDATE,
                     const wxString& name = wxT("splitter"));
    void LoadFile(const wxString &filename);
    wxString GetFile() const;
    void Draw();
};

#endif
