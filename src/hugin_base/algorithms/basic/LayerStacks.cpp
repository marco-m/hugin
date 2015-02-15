// -*- c-basic-offset: 4 -*-
/** @file LayerStacks.cpp
 *
 *  @brief implementation of functions to handle stacks and layers
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "LayerStacks.h"

#include <panodata/PanoramaData.h>
#include <panodata/StandardImageVariableGroups.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <algorithms/nona/ComputeImageROI.h>

namespace HuginBase
{
using namespace std;

vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    // special case: for a negtive overlap use the assigned stacks and skip
    // overlap calculation
    if (opts.outputStacksMinOverlap < 0)
    {
        HuginBase::ConstStandardImageVariableGroups variable_groups(pano);
        return variable_groups.getStacks().getPartsSet();
    };

    UIntSet stack;

    CalculateImageOverlap overlap(&pano);
    overlap.calculate(10);  // we are testing 10*10=100 points
    do
    {
        const unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        for (UIntSet::const_iterator it = allImgs.begin(); it != allImgs.end(); ++it)
        {
            const unsigned srcImg2 = *it;
            if (overlap.getOverlap(srcImg, srcImg2) > opts.outputStacksMinOverlap)
            {
                stack.insert(srcImg2);
            };
        };
        for (UIntSet::const_iterator it = stack.begin(); it != stack.end(); ++it)
        {
            allImgs.erase(*it);
        };
        result.push_back(stack);
        stack.clear();
    } while (!allImgs.empty());

    return result;
}

vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts)
{
    return getExposureLayers(pano, allImgs, opts.outputLayersExposureDiff);
};

std::vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs, const double maxEVDiff)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    UIntSet layer;

    do
    {
        const unsigned srcImg = *(allImgs.begin());
        layer.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a similar exposure values.
        const double firstExposureValue = pano.getImage(srcImg).getExposureValue();
        for (UIntSet::const_iterator it = allImgs.begin(); it !=  allImgs.end(); ++it)
        {
            const unsigned srcImg2 = *it;
            if ( fabs(firstExposureValue - pano.getImage(srcImg2).getExposureValue()) < maxEVDiff )
            {
                layer.insert(srcImg2);
            }
        }
        for (UIntSet::const_iterator it = layer.begin(); it != layer.end(); ++it)
        {
            allImgs.erase(*it);
        };
        result.push_back(layer);
        layer.clear();
    } while (!allImgs.empty());

    return result;
}

UIntSet getImagesinROI (const PanoramaData& pano, const UIntSet activeImages)
{
    return getImagesinROI(pano, activeImages, pano.getOptions().getROI());
}

UIntSet getImagesinROI(const PanoramaData& pano, const UIntSet activeImages, vigra::Rect2D panoROI)
{
    UIntSet images;
    PanoramaOptions opts = pano.getOptions();
    opts.setROI(panoROI);
    for (UIntSet::const_iterator it = activeImages.begin(); it != activeImages.end(); ++it)
    {
        vigra::Rect2D roi = estimateOutputROI(pano, opts, *it);
        if (!(roi.isEmpty()))
        {
            images.insert(*it);
        }
    }
    return images;
}

}
