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
#include "MaskEdPreviewWnd.h"

using HuginBase::ImageCache;
using namespace std;

MaskEdPreviewWnd::MaskEdPreviewWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name) {}

void MaskEdPreviewWnd::Init()
{
    m_bimgs.clear();
    m_selected.clear();
}
void MaskEdPreviewWnd::LoadImages(const vector<string> &filesv)
{
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
    {
        LoadImage(*it);
    }
}

void MaskEdPreviewWnd::LoadImage(const string &filename)
{
    if(filename != "")
    {
        ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
        HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
        if (img) {
            m_bimgs.push_back(new wxBitmap( wxImage(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true)));
        } 
    }
}

vector<bool> MaskEdPreviewWnd::getSelected() const 
{
    return m_selected;
}

