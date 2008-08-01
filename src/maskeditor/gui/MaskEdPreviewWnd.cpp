// -*- c-basic-offset: 4 -*-
/** @file MaskEdPreviewWnd.cpp
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

#include "huginapp/ImageCache.h"
#include "MaskEdClientWnd.h"
#include "MaskEdPreviewWnd.h"
#include "MaskEdThumbnailPreviewCtrl.h"
#include <wx/filename.h>
using HuginBase::ImageCache;
using namespace std;

#define ID_THNAIL_SELECT (wxID_HIGHEST + 1)

MaskEdPreviewWnd::MaskEdPreviewWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name) 
{
    m_boxsizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_boxsizer);
    m_boxsizer->SetSizeHints(this);
    SetScrollbars(20, 20, 50, 50);
}

void MaskEdPreviewWnd::init()
{
    m_bimgs.clear();
    m_selected.clear();
    m_boxsizer->Clear(true);
}

void MaskEdPreviewWnd::loadImage(const vector<string> &filesv)
{
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
    {
        loadImage(*it);
    }
}

void MaskEdPreviewWnd::loadImage(const string &filename)
{
    if(filename != "")
    {
        ImageCache::EntryPtr e = ImageCache::getInstance().getSmallImage(filename);
        HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
        if (img) {
            int index = m_bimgs.size();
            m_selected[index] = make_pair(filename, true);
            m_bimgs.push_back(new wxBitmap( wxImage(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true)));
            wxString fullpath(filename.c_str(), wxConvUTF8);
            wxString name;
            wxFileName::SplitPath(fullpath, NULL, &name, NULL);
            m_boxsizer->Add(new MaskEdThumbnailPreviewCtrl(this, ID_THNAIL_SELECT + index, name,
                fullpath,
                wxDefaultPosition, wxDefaultSize, wxCHK_2STATE,
                wxDefaultValidator, fullpath),
                1, wxALIGN_LEFT | wxALL, 5);
            ((MaskEdThumbnailPreviewCtrl*)this->FindWindowById(ID_THNAIL_SELECT + index, this))->SetValue(true);
            this->Connect(ID_THNAIL_SELECT + index, wxEVT_COMMAND_CHECKBOX_CLICKED,
                wxCommandEventHandler(MaskEdPreviewWnd::OnSelection));
            this->Refresh();
        } 
    }
}

map<int,pair<string,bool> > MaskEdPreviewWnd::getSelected() const 
{
    return m_selected;
}

void MaskEdPreviewWnd::OnSelection(wxCommandEvent &event)
{
    int index = event.GetId() - ID_THNAIL_SELECT;
    (m_selected[index]).second = event.IsChecked();
    ((MaskEdClientWnd*)m_parent)->getMaskEdEditWnd()->setSelectedImage(index, m_selected[index].second);
}

