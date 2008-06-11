// -*- c-basic-offset: 4 -*-
/** @file MaskEdClientWnd.cpp
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
#include "huginapp/ImageCache.h"
#include "MaskEdClientWnd.h"
#include <wx/xrc/xmlres.h>

MaskEdClientWnd::MaskEdClientWnd(wxWindow *parent, wxWindowID id,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxSplitterWindow(parent, id, pos, size, style, name)
{
    m_MaskEdEditWnd = new MaskEdEditWnd(this, wxID_ANY, wxPoint(0,0), wxSize(size.GetWidth(), size.GetHeight()/2)); //XRCCTRL(*this, "m_MaskEdEditWnd", MaskEdEditWnd);
    m_MaskEdPreviewWnd = new MaskEdPreviewWnd(this, wxID_ANY, wxPoint(0, size.GetHeight()/2), wxSize(size.GetWidth(), size.GetHeight()/2)); //XRCCTRL(*this, "m_MaskEdPreviewWnd", MaskEdPreviewWnd);
}

MaskEdClientWnd::~MaskEdClientWnd()
{
    delete m_MaskEdEditWnd;
    delete m_MaskEdPreviewWnd;
}   
void MaskEdClientWnd::LoadFile(const wxString &filename)
{
    m_Filename = filename;
    m_MaskEdEditWnd->LoadImage(filename);
}

wxString MaskEdClientWnd::GetFile() const 
{
    return m_Filename;
}
