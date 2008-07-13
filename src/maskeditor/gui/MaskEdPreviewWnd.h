// -*- c-basic-offset: 4 -*-
/** @file MaskEdPreviewWnd.h
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
#ifndef MASKEDPREVIEWWND_H
#define MASKEDPREVIEWWND_H

#include <wx/wx.h>
#include <string>
#include <vector>
#include <map>

class MaskEdPreviewWnd : public wxScrolledWindow
{
    std::vector<wxBitmap*>  m_bimgs;
    std::map<int, std::pair<std::string, bool> >  m_selected;
    wxBoxSizer             *m_boxsizer;
public:
    MaskEdPreviewWnd(wxWindow *parent, wxWindowID winid = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxScrolledWindowStyle | wxDOUBLE_BORDER,
                     const wxString& name = wxPanelNameStr);

    void init();
    void loadImages(const std::vector<std::string> &filesv);
    void loadImage(const std::string &filename);
    std::map<int, std::pair<std::string,bool> > getSelected() const;

    void OnSelection(wxCommandEvent &event);
};

#endif
