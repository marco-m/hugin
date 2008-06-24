// -*- c-basic-offset: 4 -*-
/** @file lazysnapping.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id: lazysnapping.cpp 3138 2008-06-21 03:20:03Z fmannan $
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
#include "LazySnapping.h"

using namespace std;
using HuginBase::ImageCache;

LazySnapping::LazySnapping() : m_mask(0)
{
    init();
}
LazySnapping::LazySnapping(const string &filename) : m_mask(0)
{
    init();
    setImage(filename);
}
LazySnapping::~LazySnapping() {}

void LazySnapping::init()
{
    m_name = "LazySnapping";
}

void LazySnapping::reset()
{}

void LazySnapping::markPixels(vector<PixelCoord> coords, Label label)
{}
void LazySnapping::setRegion(vector<PixelCoord> coords, Label label)
{}
void LazySnapping::setImage(unsigned char* data, int row, int col, int depth)
{}
void LazySnapping::setImage(const std::string &filename)
{
    init();
    m_filename = filename;
    if(m_mask)
        delete m_mask;
    ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
    HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
    m_mask = new wxBitmap(img->width(), img->height(), 1);
}
wxMask* LazySnapping::getMask() const
{
    return new wxMask(*m_mask, *wxWHITE);
}

wxBitmap* LazySnapping::getMaskBitmap() const
{
    return m_mask;
}


