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
#include "PolyEdBasicMemento.h"
#include <wx/wx.h>

using namespace std;
using HuginBase::ImageCache;

struct PixelCoordToWxPoint
{
     wxPoint operator()(const PixelCoord &p)
     {
        return wxPoint(p.x, p.y);
     }
};

PolyEd_Basic::PolyEd_Basic() : m_mask(0), m_width(0), m_height(0)
{
    m_name = "PolyEd_Basic";
}
PolyEd_Basic::PolyEd_Basic(const string &filename) : m_mask(0), m_width(0), m_height(0)
{
    m_name = "PolyEd_Basic";
    setImage(filename);
}
PolyEd_Basic::PolyEd_Basic(const string &imgId, const vigra::BRGBImage *img, vigra::BImage *mask) : m_mask(0), m_width(0), m_height(0)
{
    m_name = "PolyEd_Basic";
    setImage(imgId, img, mask);
}
PolyEd_Basic::~PolyEd_Basic() {}

void PolyEd_Basic::setMemento(IMaskEdMemento *memento)
{
    if(m_mask)
        delete m_mask;
    m_mask = ((PolyEdBasicMemento*)memento)->m_mask;
    m_width = ((PolyEdBasicMemento*)memento)->m_width;
    m_height = ((PolyEdBasicMemento*)memento)->m_height;
}

IMaskEdMemento* PolyEd_Basic::createMemento()
{
    return new PolyEdBasicMemento(m_mask, m_width, m_height);
}

void PolyEd_Basic::init(vigra::BImage *mask)
{
    if(m_width > 0 && m_height > 0)
    {
        if(!mask) {
            m_mask = new wxBitmap(m_width, m_height, 1);
            wxMemoryDC dc(*m_mask);
            dc.SetBackground(*wxWHITE_BRUSH);
            dc.Clear();
        } else {
            assert(mask->width() == m_width && mask->height() == m_height);
            m_mask = new wxBitmap((const char*)mask->data(), mask->width(), mask->height());
        }
    }
}

void PolyEd_Basic::reset()
{
    m_width = 0;
    m_height = 0;
}

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
    //copy(coords.begin(), coords.end(), coords_pt);
    transform(coords.begin(), coords.end(), coords_pt, PixelCoordToWxPoint());
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
    reset();
    m_filename = filename;

    if(m_mask)
        delete m_mask;
    ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
    HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
    m_width = img->width();
    m_height = img->height();
    init();
    //m_mask = new wxBitmap(img->width(), img->height(), 1);
    //wxMemoryDC dc(*m_mask);
    //assert(dc.IsOk());
    ////dc.SetBrush(*wxWHITE_BRUSH);
    ////dc.SetBrush(*wxBLACK_BRUSH);
    //dc.SetBackground(*wxWHITE_BRUSH);
    //dc.Clear();
}

void PolyEd_Basic::setImage(const string &imgId, const vigra::BRGBImage* img, vigra::BImage *mask)
{
    reset();
    m_filename = imgId;
    assert(img);
    
    m_width = img->width();
    m_height = img->height();
    init(mask);    
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


