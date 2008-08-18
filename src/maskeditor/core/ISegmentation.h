// -*- c-basic-offset: 4 -*-
/** @file ISegmentation.h
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

#ifndef ISEGMENTATION_H
#define ISEGMENTATION_H
#include <vector>
#include "BrushStroke.h"
#include "MaskPoly.h"
#include "Pixel.h"
#include "IMaskEdMemento.h"

class wxMask;
class wxBitmap;
class wxPoint;

class ISegmentation
{
protected:
    std::string m_name;               //segmentation technique
    std::string m_filename;
    unsigned char* m_mask;
public:
    enum Label { NOLABEL, BKGND, FGND };
   
    ISegmentation() : m_mask(0) {}
    virtual ~ISegmentation() { delete m_mask; }

    virtual void init(vigra::BImage *mask = 0) = 0;
    virtual void reset() = 0;
    std::string name() { return m_name; }

    virtual void setMemento(IMaskEdMemento *memento) = 0;
    virtual IMaskEdMemento* createMemento() = 0;

    virtual void saveMaskMetaData(const std::string &filename) = 0;

    virtual void markPixels(std::vector<PixelCoord> coords, Label label) = 0;
    virtual void setRegion(std::vector<PixelCoord> coords, Label label) = 0;
    virtual void setImage(unsigned char* data, int row, int col, int depth) = 0;
    virtual void setImage(const std::string &filename) = 0;
    virtual void setImage(const std::string &imgId, const vigra::BRGBImage* img, vigra::BImage *mask = 0) = 0;
    virtual wxMask* getMask() const = 0;
    virtual wxBitmap* getMaskBitmap() const = 0;
    virtual std::vector<wxPoint> getOutline() const = 0;
};

#endif
