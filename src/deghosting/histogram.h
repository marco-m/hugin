
/**
 * Histogram implementation and various operations with histograms
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vigra/stdimage.hxx>
#include <vigra/copyimage.hxx>

using namespace vigra;

/**
 * Histogram with 256 levels.
 */
template <class IMAGETYPE>
class Histogram
{
    public:
        Histogram(IMAGETYPE &image);
        ~Histogram();
        
        void match(Histogram<IMAGETYPE> &reference);
        /**
         * Equalize image.
         */
        void equalize();
    private:
        // image storing RGB data as integers in [0,255]
        BImage byteImage;
        int histogram[256];
        int cumulativeHistogram[256];
        // variables for cumulative histogram
        long pixelCount;
        int maxIntensity;
        
        /**
         * Create instensity image from vector image (ie from RGB image)
         */
        void createIntensityImage(IMAGETYPE &srcImage, BImage &intensityImage, VigraFalseType);
        /**
         * Create instensity image from scalar image (ie from grayscale image)
         */
        void createIntensityImage(IMAGETYPE &srcImage, BImage &intensityImage, VigraTrueType);
};

template <class IMAGETYPE>
Histogram<IMAGETYPE>::Histogram(IMAGETYPE &srcImage) {
    pixelCount = 0;
    maxIntensity = 0;
    // initialize histogram to all zero
    for (int i = 0; i < 256; i++)
        histogram[i] = 0;
    
    typedef typename NumericTraits<PixelType>::isScalar srcIsScalar;
    createIntensityImage(srcImage, byteImage, srcIsScalar());
    
    // compute histogram
    BImage::const_traverser sy = byteImage.upperLeft();
    BImage::const_traverser send = byteImage.lowerRight();
    for(; sy.y != send.y; ++sy.y) {
        BImage::const_traverser sx = sy;
        for(; sx.x != send.x; ++sx.x) {
            histogram[*sx]++;
            pixelCount++;
            maxIntensity = (*sx > maxIntensity) ? *sx : maxIntensity;
        }
    }
}

template <class IMAGETYPE>
BImage Histogram<IMAGETYPE>::equalize() {
    // build LUT
    int LUT[256];
    int sum = 0;
    for (int i = 0; i <256; i++) {
        sum += histogram[i];
        LUT[i] = sum * maxIntensity/pixelCount;
    }
    
    IMAGETYPE equalizedImage(Size2D(byteImage.width(), byteImage.height()));
    
    // transform image
    BImage::const_traverser sy = byteImage.upperLeft();
    BImage::const_traverser send = byteImage.lowerRight();
    IMAGETYPE::traverser dy = equalizedImage.upperLeft();
    for(; sy.y != send.y; ++sy.y, ++dy.y) {
        BImage::const_traverser sx = sy;
        IMAGETYPE::traverser dx = dy;
        for(; sx.x != send.x; ++sx.x, ++dx.x) {
            // TODO color image needs to operate on Lab
            *dx = LUT[]
        }
    }
}


template <class IMAGETYPE>
void Histogram<IMAGETYPE>::createIntensityImage(IMAGETYPE &srcImage, BImage &intensityImage, VigraFalseType) {
    RGBToGrayAccessor<IMAGETYPE::PixelType> color2gray;
    copyImage(srcImageRange(srcImage, color2gray), destImage(intensityImage));
}

template <class IMAGETYPE>
void Histogram<IMAGETYPE>::createIntensityImage(IMAGETYPE &srcImage, BImage &intensityImage, VigraTrueType) {
    copyImage(srcImageRange(srcImage), destImage(intensityImage));
}
