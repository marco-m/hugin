// -*- c-basic-offset: 4 -*-
/** @file MaskEditingWnd.h
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

#ifndef MASKEDITINGWND_H
#define MASKEDITINGWND_H

#include <wx/wx.h>

class MaskEditingWnd : public wxScrolledWindow
{
    wxBitmap *m_bimg;
public:
    MaskEditingWnd(wxWindow *parent,
                     wxWindowID winid = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxScrolledWindowStyle | wxDOUBLE_BORDER,
                     const wxString& name = wxPanelNameStr);
    ~MaskEditingWnd();
    void LoadImage(const wxString &filename);
    void OnPaint(wxPaintEvent &event);
    void OnScrollEvt(wxScrollEvent &event);
    void Draw();

    DECLARE_EVENT_TABLE()
};

#endif
