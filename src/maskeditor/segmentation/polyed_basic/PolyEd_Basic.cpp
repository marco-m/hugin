// -*- c-basic-offset: 4 -*-
/** @file polyed_basic.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id: polyed_basic.cpp 3138 2008-06-21 03:20:03Z fmannan $
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
#include "PolyEd_Basic.h"
using namespace std;
using HuginBase::ImageCache;
PolyEd_Basic::PolyEd_Basic() : m_mask(0)
{
    init();
}
PolyEd_Basic::PolyEd_Basic(const string &filename) : m_mask(0)
{
    init();
    setImage(filename);
}
PolyEd_Basic::~PolyEd_Basic() {}

void PolyEd_Basic::init()
{
    m_name = "PolyEd_Basic";
}

void PolyEd_Basic::reset()
{}

void PolyEd_Basic::markPixels(vector<PixelCoord> coords, Label label)
{}

void PolyEd_Basic::setRegion(vector<PixelCoord> coords, Label label)
{
    wxMemoryDC dc(*m_mask);
    assert(dc.IsOk());
    
    if(label == ISegmentation::FGND)
        dc.SetBrush(*wxBLACK_BRUSH);
    else
        dc.SetBrush(*wxWHITE_BRUSH);
    wxPoint *coords_pt = new wxPoint[coords.size()];
    copy(coords.begin(), coords.end(), coords_pt);
    dc.DrawPolygon(coords.size(), coords_pt);
    delete []coords_pt;
}

void PolyEd_Basic::setImage(unsigned char* data, int row, int col, int depth)
{
    init();
    //...
}

void PolyEd_Basic::setImage(const std::string &filename)
{
    init();
    m_filename = filename;

    if(m_mask)
        delete m_mask;
    ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
    HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
    m_mask = new wxBitmap(img->width(), img->height(), 1);
    wxMemoryDC dc(*m_mask);
    assert(dc.IsOk());
    //dc.SetBrush(*wxWHITE_BRUSH);
    //dc.SetBrush(*wxBLACK_BRUSH);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
}

wxMask* PolyEd_Basic::getMask() const
{
    return new wxMask(*m_mask, *wxWHITE);
}

wxBitmap* PolyEd_Basic::getMaskBitmap() const
{
    return m_mask;
}

vector<wxPoint> PolyEd_Basic::getOutline() const
{
    vector<wxPoint> outline;
    return outline;
}


