// -*- c-basic-offset: 4 -*-
/** @file polyed_basic.h
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id: polyed_basic.h 3138 2008-06-21 03:20:03Z fmannan $
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
#ifndef POLYED_BASIC_H
#define POLYED_BASIC_H

#include <vigra/stdimage.hxx>

#include <vector>
#include <string>
#include "../../core/ISegmentation.h"

class PolyEd_Basic : public ISegmentation
{
    wxBitmap *m_mask;
    int       m_width;
    int       m_height;
public:
    PolyEd_Basic();
    PolyEd_Basic(const std::string &filename);
    PolyEd_Basic(const std::string &imgId, const vigra::BRGBImage *img, vigra::BImage *mask);
    ~PolyEd_Basic();

    void init(vigra::BImage *mask = 0);
    void reset();
    std::string name();

    void setMemento(IMaskEdMemento *memento);
    IMaskEdMemento* createMemento();

    void markPixels(std::vector<PixelCoord> coords, Label label);
    void setRegion(std::vector<PixelCoord> coords, Label label);
    void setImage(unsigned char* data, int row, int col, int depth);
    void setImage(const std::string &filename);
    void setImage(const std::string &imgId, const vigra::BRGBImage* img, vigra::BImage *mask = 0);
    wxMask* getMask() const;
    wxBitmap* getMaskBitmap() const;
    std::vector<wxPoint> getOutline() const;
};

#endif
