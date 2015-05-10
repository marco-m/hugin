// -*- c-basic-offset: 4 -*-
/** @file LayerStacks.h
 *
 *  @brief declaration of functions to handle stacks and layers
 *
 */

 /*  This is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _BASICALGORITHMS_LAYERSTACKS_H
#define _BASICALGORITHMS_LAYERSTACKS_H

#include <panodata/PanoramaData.h>

namespace HuginBase
{
/** returns vector of set of output stacks */
IMPEX UIntSetVector getHDRStacks(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts);
/** returns vector of set of output exposure layers */
IMPEX UIntSetVector getExposureLayers(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts);
IMPEX UIntSetVector getExposureLayers(const PanoramaData & pano, UIntSet allImgs, const double maxEVDiff);
/** returns set of images which are visible in output ROI */
IMPEX UIntSet getImagesinROI(const PanoramaData& pano, const UIntSet activeImages);
/** returns set of images which are visible in given ROI */
IMPEX UIntSet getImagesinROI(const PanoramaData& pano, const UIntSet activeImages, const vigra::Rect2D panoROI);
}

#endif /* _BASICALGORITHMS_LAYERSTACKS_H */
