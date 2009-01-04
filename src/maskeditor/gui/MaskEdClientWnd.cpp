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
    m_MaskEdEditWnd = new MaskEdEditWnd(this, wxID_ANY, wxDefaultPosition, wxSize(size.GetWidth(), size.GetHeight()*0.75));
    m_MaskEdPreviewWnd = new MaskEdPreviewWnd(this, wxID_ANY, wxDefaultPosition, wxSize(size.GetWidth(), size.GetHeight()*0.25));
    
    SetMinimumPaneSize(size.GetHeight() * 0.25);
    //SetSashPosition(size.GetHeight() * 0.75);
    m_MaskEdEditWnd->Show();
    m_MaskEdPreviewWnd->Show();
    SplitHorizontally(m_MaskEdEditWnd, m_MaskEdPreviewWnd);
}

MaskEdClientWnd::~MaskEdClientWnd()
{
    delete m_MaskEdEditWnd;
    delete m_MaskEdPreviewWnd;
}   

MaskEdEditWnd* MaskEdClientWnd::getMaskEdEditWnd()
{
    return m_MaskEdEditWnd;
}

MaskEdPreviewWnd* MaskEdClientWnd::getMaskEdPreviewWnd()
{
    return m_MaskEdPreviewWnd;
}

void MaskEdClientWnd::newProject(const wxString &filename)
{
    MaskMgr::getInstance()->init();
    m_MaskEdEditWnd->init();
    m_MaskEdPreviewWnd->init();
    m_MaskEdEditWnd->Refresh();
    m_MaskEdPreviewWnd->Refresh();
}

void MaskEdClientWnd::loadProject(const wxString &filename)
{
    MaskMgr::getInstance()->loadMaskProject(string(filename.mb_str()));
    m_MaskEdEditWnd->loadImage(MaskMgr::getInstance()->getImages());
    m_MaskEdPreviewWnd->loadImage(MaskMgr::getInstance()->getImages());
}

void MaskEdClientWnd::loadImage(const wxArrayString &filesv)
{
    newProject();
    vector<string> strfilesv;
    transform(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(strfilesv), wxStringTostring);
    MaskMgr::getInstance()->loadImage(strfilesv);
    m_MaskEdEditWnd->loadImage(strfilesv);
    m_MaskEdPreviewWnd->loadImage(strfilesv);
}

void MaskEdClientWnd::saveMask(int index, const wxString &filename)
{
    wxBitmap *bmp = MaskMgr::getInstance()->getSegmentation(index)->getMaskBitmap();
    bmp->SaveFile(filename, wxBITMAP_TYPE_BMP);
}

wxString MaskEdClientWnd::getFile() const 
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

void MaskEdClientWnd::setEditMode(MaskEdEditMode_t edmode)
{
    m_MaskEdEditWnd->setEditMode(edmode);
}

void MaskEdClientWnd::toggleShowOverlappedRect()
{
    m_MaskEdEditWnd->toggleShowOverlappedRect();
}

void MaskEdClientWnd::undo()
{
    m_MaskEdEditWnd->undo();
}

void MaskEdClientWnd::redo()
{
    m_MaskEdEditWnd->redo();
}
