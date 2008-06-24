// -*- c-basic-offset: 4 -*-
/** @file MaskEdClientWnd.cpp
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
#include <algorithm>
#include <vector>
#include "huginapp/ImageCache.h"
#include "MaskEdClientWnd.h"
#include "../core/MaskMgr.h"
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace std;

string wxStringTostring(wxString str)
{
    return string(str.mb_str());
}
MaskEdClientWnd::MaskEdClientWnd(wxWindow *parent, wxWindowID id,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxSplitterWindow(parent, id, pos, size, style, name)
{
    m_MaskEdEditWnd = new MaskEdEditWnd(this);
    m_MaskEdPreviewWnd = new MaskEdPreviewWnd(this);
    
    SetMinimumPaneSize(20);
    m_MaskEdEditWnd->Show();
    m_MaskEdPreviewWnd->Show();
    SplitHorizontally(m_MaskEdEditWnd, m_MaskEdPreviewWnd);
}

MaskEdClientWnd::~MaskEdClientWnd()
{
    delete m_MaskEdEditWnd;
    delete m_MaskEdPreviewWnd;
}   
void MaskEdClientWnd::LoadProject(const wxString &filename)
{
    
    MaskMgr::getInstance()->loadMaskProject(string(filename.mb_str()));
    m_MaskEdEditWnd->loadImages(MaskMgr::getInstance()->getImages());
    m_MaskEdPreviewWnd->loadImages(MaskMgr::getInstance()->getImages());
}

void MaskEdClientWnd::loadImages(const wxArrayString &filesv)
{
    vector<string> strfilesv;
    transform(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(strfilesv), wxStringTostring);
    MaskMgr::getInstance()->loadImages(strfilesv);
    m_MaskEdEditWnd->loadImages(strfilesv);
    m_MaskEdPreviewWnd->loadImages(strfilesv);
}

void MaskEdClientWnd::saveMask(int index, const wxString &filename)
{
    wxBitmap *bmp = MaskMgr::getInstance()->getSegmentation(index)->getMaskBitmap();
    bmp->SaveFile(filename, wxBITMAP_TYPE_BMP);
}

wxString MaskEdClientWnd::GetFile() const 
{
    return wxT("");
}

void MaskEdClientWnd::zoom(float scale, wxRect region)
{
    m_MaskEdEditWnd->zoom(scale);
}

float MaskEdClientWnd::getZoomLevel() const 
{
    return m_MaskEdEditWnd->getZoomLevel();
}

void MaskEdClientWnd::SetEditMode(MaskEdEditMode_t edmode)
{
    m_MaskEdEditWnd->SetEditMode(edmode);
}
