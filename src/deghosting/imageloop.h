
/**
 * Functions for looping over image pixels.
 * Copyright (C) 2010  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef IMAGE_LOOP
#define IMAGE_LOOP

#include "deghosting.h"
#include "multiresolution_scaling.h"

#include <vigra/resizeimage.hxx>

// number of pixels to look at in all directions
// ie. 1 for neighbourhood of size 3x3, 2 for 5x5 etc.
#define NEIGHB_DIST 1

using namespace vigra;
using std::vector;
using std::cout;
using std::endl;

namespace deghosting {
    /** Loop over all pixels neighborhood  in set of images applying given weighting functor.
     */
    template <class ProcessImageType, class Functor>
    void imageLoop(Deghosting *deghostingInfo, std::vector<FImagePtr> &weights, std::vector<boost::shared_ptr<ProcessImageType> > &processImages, Functor Kh)
    {
        typedef typename ProcessImageType::PixelType ProcessImagePixelType;
        typedef typename ProcessImageType::traverser ProcessImageTraverser;
        typedef boost::shared_ptr<ProcessImageType> ProcessImageTypePtr;
        typedef typename NumericTraits<ProcessImagePixelType>::ValueType ProcessImageValueType;
        typedef typename NumericTraits<ProcessImageValueType>::RealPromote ProcessImageValueTypeRealPromote;
        
        float maxWeight = 0;
        // image size
        const int origWidth = weights[0]->width();
        const int origHeight = weights[0]->height();
        // information from deghostingInfo
        const uint16_t flags = deghostingInfo->getFlags();
        const int iterations = deghostingInfo->getIterationNum();
        const int verbosity = deghostingInfo->getVerbosity();
        const int imageNum = processImages.size();
        
        // if we doing scaling, we have to backup L*a*b images of original size
        std::vector<ProcessImageTypePtr> backupLab;
        backupLab.reserve(imageNum);
        if (flags & ADV_MULTIRES) {
            for (unsigned int i = 0; i < processImages.size(); i++) {
                // backup original size L*a*b
                backupLab.push_back(processImages[i]);
            }
        }
        
        if (verbosity > 0)
            cout << "Running khan algorithm" << endl;
        // and we can run khan algorithm
        // khan iteration
        for (int it = 0; it < iterations; it++) {
            if (verbosity > 0)
                cout << "iteration " << it+1 << endl;
            // copy weights from previous iteration
            if (verbosity > 1)
                cout << "copying weights from previous iteration" << endl;
            
            std::vector<FImagePtr> prevWeights;
            prevWeights.reserve(imageNum);
            for (unsigned int i = 0; i < imageNum; i++) {
                // scale weights to the required size
                if (flags & ADV_MULTIRES) {
                    // it would be better to use resampleImage, but it seems to be present only in VIGRA 1.6
                    // so let's use some of the resizeImageINTERPOLATION() functions
                    
                    // scale
                    Size2D newScale;
                    if (multires_scale(origWidth, origHeight, iterations, it, newScale)) {
                        // destination images
                        FImagePtr resizedWeight(new FImage(newScale));
                        ProcessImageTypePtr resizedLab(new ProcessImageType(newScale));
                        
                        // No interpolation – only for testing
                        resizeImageNoInterpolation(srcImageRange(*weights[i]), destImageRange(*resizedWeight));
                        resizeImageNoInterpolation(srcImageRange(*backupLab[i]), destImageRange(*resizedLab));
                        
                        prevWeights[i] = resizedWeight;
                        processImages[i] = resizedLab;
                        weights[i] = FImagePtr(new FImage(*resizedWeight));
                    } else {
                        // don't scale at all
                        // just copy weights as if no scaling setting was applied
                        goto DONTSCALE;
                    }
                } else {
                    DONTSCALE:
                    FImagePtr tmp(new FImage(*weights[i]));
                    prevWeights[i] = tmp;
                }
            }
            
            // loop through all images
            for (unsigned int i = 0; i < imageNum; i++) {
                if (verbosity > 1)
                    cout << "processing image " << i+1 << endl;
                
                // vector storing pixel data
                ProcessImagePixelType X;
                // sums for eq. 6
                ProcessImageValueTypeRealPromote wpqssum = 0;
                ProcessImageValueTypeRealPromote wpqsKhsum = 0;
                // image size
                const int width = processImages[i]->width();
                const int height = processImages[i]->height();

                // iterator to the upper left corner
                ProcessImageTraverser sy = processImages[i]->upperLeft();
                // iterator to the lower right corner
                ProcessImageTraverser send = processImages[i]->lowerRight();
                // iterator to the weight image left corner
                FImage::traverser wy = weights[i]->upperLeft();
                // loop through the row
                for (int y=0; sy.y != send.y; ++sy.y, ++wy.y, ++y) {
                    // iterator to the source (L*a*b image)
                    ProcessImageTraverser sx = sy;
                    // iterator to the weight
                    FImage::traverser wx = wy;
                    // loop over the pixels
                    for (int x=0; sx.x != send.x; ++sx.x, ++wx.x, ++x) {
                        if (verbosity > 2)
                            cout << "processing pixel (" << x+1 << "," << y+1 << ")" << endl;
                        // set pixel vector
                        X = *sx;
                        
                        // loop through all layers
                        for (unsigned int j = 0; j < processImages.size(); j++) {
                            if (verbosity > 2)
                                cout << "processing layer " << j << endl;
                            
                            // iterator to the neighbour
                            ProcessImageTraverser neighby = processImages[j]->upperLeft();
                            // iterator to the weight
                            FImage::traverser weighty = prevWeights[j]->upperLeft();
                            // pixel offset
                            int ndy = -NEIGHB_DIST;
                            // move them to the upper bound
                            // find the best upper bound
                            if (y-NEIGHB_DIST < 0) {
                                ndy = -y;
                            }
                            else {
                                neighby.y += y-NEIGHB_DIST;
                                weighty.y += y-NEIGHB_DIST;
                            }
                            
                            // iterate through neighbourhoods y axis
                            int maxDisty = (height - y) > NEIGHB_DIST ? NEIGHB_DIST : (height - y-1);
                            for (; ndy <= maxDisty; ++neighby.y, ++weighty.y, ++ndy) {
                                ProcessImageTraverser neighbx = neighby;
                                FImage::traverser weightx = weighty;
                                // pixel offset
                                int ndx = -NEIGHB_DIST;
                                // move them to the upper bound
                                // find the best upper bound
                                if (x-NEIGHB_DIST < 0) {
                                    ndx = -x;
                                }
                                else {
                                    neighbx.x += x-NEIGHB_DIST;
                                    weightx.x += x-NEIGHB_DIST;
                                }
                                // iterate through neighbourhoods x axis
                                int maxDistx = (width - x) > NEIGHB_DIST ? NEIGHB_DIST : (width - x-1);
                                for (; ndx <= maxDistx; ++neighbx.x, ++weightx.x, ++ndx) {
                                    if (verbosity > 3)
                                        cout << "(" << ndx << "," << ndy << ")";
                                    // now we can construct pixel vector
                                    // should omit the middle pixel, ie use only neighbours
                                    if (ndx != 0 || ndy != 0) {
                                        wpqsKhsum += (*weightx * Kh(X-(*neighbx)));
                                        wpqssum += *weightx;
                                    }
                                    
                                    maxDistx = (width - x) > NEIGHB_DIST ? NEIGHB_DIST : (width - x-1);
                                }
                                if (verbosity > 3)
                                    cout << endl;
                                
                                maxDisty = (height - y) > NEIGHB_DIST ? NEIGHB_DIST : (height - y-1);
                            }
                        }
                        
                        if (verbosity > 2)
                            cout << "computing new weight" << endl;
                        if (flags & ADV_ONLYP)
                            *wx = (float) wpqsKhsum/wpqssum;
                        else
                            *wx *= (float) wpqsKhsum/wpqssum;
                        if (maxWeight < *wx)
                            maxWeight = *wx;
                        wpqsKhsum = wpqssum = 0;
                        
                    }
                }
            }
        }
        
        if (verbosity > 1)
                cout << "normalizing weights" << endl;
        double factor = 255.0f/maxWeight;
        for (unsigned int i=0; i<weights.size(); ++i) {
            transformImage(srcImageRange(*(weights[i])), destImage(*(weights[i])), NormalizeFunctor<float>(factor));
        }
    }
}

#endif
