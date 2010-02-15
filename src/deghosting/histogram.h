
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
#include <vigra/colorconversions.hxx>
#include <vigra/transformimage.hxx>

using namespace vigra;

/**
 * Equalize image histogram. Uses 256 levels. Grayscale version.
 */
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void equalizeHistogram(SrcIterator sul, SrcIterator slr, SrcAccessor as,
                        DestIterator dul, DestAccessor ad)
{
    long pixelCount = 0;
    UInt8 maxIntensity = 0;
    int histogram[256];
    // initialize histogram to all zero
    for (int i = 0; i < 256; i++)
        histogram[i] = 0;
    
    // this may be superfluous
    BImage byteImage(slr.x-sul.x, slr.y - sul.y);
    copyImage(sul, slr, as, byteImage.upperLeft(), byteImage.accessor());
    
    // compute histogram
    BImage::const_traverser by = byteImage.upperLeft();
    BImage::const_traverser bend = byteImage.lowerRight();
    for(; by.y != bend.y; ++by.y) {
        BImage::const_traverser bx = by;
        for(; bx.x != bend.x; ++bx.x) {
            histogram[*bx]++;
            pixelCount++;
            maxIntensity = (*bx > maxIntensity) ? *bx : maxIntensity;
        }
    }
    
    // build LUT
    int LUT[256];
    int sum = 0;
    for (int i = 0; i <256; i++) {
        sum += histogram[i];
        LUT[i] = sum * maxIntensity/pixelCount;
    }
    
    // transform (equalize) image using LUT
    SrcIterator sy = sul;
    DestIterator dy = dul;
    for(; sy.y != slr.y; ++sy.y, ++dy.y) {
        BImage::const_traverser sx = sy;
        DestIterator dx = dy;
        for(; sx.x != slr.x; ++sx.x, ++dx.x) {
            *dx = LUT[*sx];
        }
    }
}

/**
 * Equalize image histogram. Uses 256 levels. Color version.
 */
template <class SrcIterator, class SrcRGBValue,
          class DestIterator, class DestRGBValue>
void equalizeHistogram(SrcIterator sul, SrcIterator slr, RGBAccessor<SrcRGBValue> as,
                        DestIterator dul, RGBAccessor<DestRGBValue> ad)
{
    long pixelCount = 0;
    UInt8 maxIntensity = 0;
    int histogram[256];
    // initialize histogram to all zero
    for (int i = 0; i < 256; i++)
        histogram[i] = 0;
    
    // Convert to Lab
    typedef BasicImage<TinyVector<UInt8, 3> > Uint8LabImage;
    Uint8LabImage LabImage(slr.x-sul.x, slr.y - sul.y);
    RGB2LabFunctor<UInt8> RGB2Lab;
    transformImage(sul, slr, as, LabImage.upperLeft(), LabImage.accessor(), RGB2Lab);
    
    // compute histogram
    Uint8LabImage::const_traverser by = LabImage.upperLeft();
    Uint8LabImage::const_traverser bend = LabImage.lowerRight();
    for(; by.y != bend.y; ++by.y) {
        Uint8LabImage::const_traverser bx = by;
        for(; bx.x != bend.x; ++bx.x) {
            histogram[(*bx)[0]]++;
            pixelCount++;
            maxIntensity = ((*bx)[0] > maxIntensity) ? (*bx)[0] : maxIntensity;
        }
    }
    
    // build LUT
    int LUT[256];
    int sum = 0;
    for (int i = 0; i <256; i++) {
        sum += histogram[i];
        LUT[i] = sum * maxIntensity/pixelCount;
    }
    
    // transform (equalize) Lab image using LUT
    Uint8LabImage::traverser sy = LabImage.upperLeft();
    Uint8LabImage::const_traverser send = LabImage.lowerRight();
    for(; sy.y != send.y; ++sy.y) {
        Uint8LabImage::traverser sx = sy;
        for(; sx.x != send.x; ++sx.x) {
            (*sx)[0] = LUT[(*sx)[0]];
        }
    }
    
    // convert Lab image back to RGB
    Lab2RGBFunctor<UInt8> Lab2RGB;
    transformImage(LabImage.upperLeft(), LabImage.lowerRight(), LabImage.accessor(), dul, ad, Lab2RGB);
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void equalizeHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                        pair<DestIterator, DestAccessor> dest)
{
    equalizeHistogram(src.first, src.second, src.third,
                      dest.first, dest.second);
}
