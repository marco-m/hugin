// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief implementation of HFOVDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoImage.h 1970 2007-04-18 22:26:56Z dangelo $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _Hgn1_PANOIMAGE_H
#define _Hgn1_PANOIMAGE_H

#include <panodata/PanoImage.h>
#include <panodata/SrcPanoImage.h>
#include <panodata/DestPanoImage.h>

namespace PT {

    using HuginBase::ImageOptions;
    using HuginBase::PanoImage;
    using HuginBase::SrcPanoImage;
    using HuginBase::DestPanoImage;
    
    inline bool initImageFromFile(SrcPanoImage & img, double & focalLength, double & cropFactor)
    {
        return HuginBase::SrcPanoImage::initImageFromFile(img, focalLength, cropFactor);
    }

    inline double calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize)
    {
        return HuginBase::SrcPanoImage::calcHFOV(proj,fl,crop,imageSize);
    }

    inline double calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize)
    {
        return HuginBase::SrcPanoImage::calcFocalLength(proj,hfov,crop,imageSize);
    }
    
    
} // namespace

#endif // PANOIMAGE_H