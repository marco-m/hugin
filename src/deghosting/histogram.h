
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

#include <iostream>

#include <vigra/stdimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/colorconversions.hxx>
#include <vigra/transformimage.hxx>

#define HISTOGRAM_HISTOGRAM_SIZE 256

using namespace vigra;

// IMPORTANT NOTE: ValueHistogram may be used

template <class SrcIterator, class SrcAccessor>
TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> computeHistogram(SrcIterator sul, SrcIterator slr, SrcAccessor as)
{
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> histogram(0);
    
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
        }
    }
    
    return histogram;
}

/**
 * Luminance channel is used for computing histogram from color image.
 */
template <class SrcIterator, class SrcRGBValue>
TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> computeHistogram(SrcIterator sul, SrcIterator slr, RGBAccessor<SrcRGBValue> as)
{
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> histogram(0);
    
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
        }
    }
    
    return histogram;
}

template <class SrcIterator, class SrcAccessor>
TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> computeHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src)
{
    return computeHistogram(src.first, src.second, src.third);
}

template <class SrcIterator, class SrcAccessor>
TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> computeRGBHistogram(SrcIterator sul, SrcIterator slr, SrcAccessor as)
{
    TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> histogram;
    
    // this may be superfluous
    BRGBImage byteImage(slr.x-sul.x, slr.y - sul.y);
    copyImage(sul, slr, as, byteImage.upperLeft(), byteImage.accessor());
    
    // compute histogram
    BRGBImage::const_traverser by = byteImage.upperLeft();
    BRGBImage::const_traverser bend = byteImage.lowerRight();
    for(; by.y != bend.y; ++by.y) {
        BRGBImage::const_traverser bx = by;
        for(; bx.x != bend.x; ++bx.x) {
            histogram[0][(*bx)[0]]++;
            histogram[1][(*bx)[1]]++;
            histogram[2][(*bx)[2]]++;
        }
    }
    
    return histogram;
}

template <class SrcIterator, class SrcAccessor>
TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> computeRGBHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src)
{
    return computeRGBHistogram(src.first, src.second, src.third);
}

template <class SrcIterator, class SrcAccessor>
TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> computeCumulativeHistogram(SrcIterator sul, SrcIterator slr, SrcAccessor as)
{
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> histogram = computeHistogram(sul, slr, as);
    
    long sum = 0;
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++) {
        sum += histogram[i];
        histogram[i] = sum;
    }
    
    return histogram;
}

template <class SrcIterator, class SrcAccessor>
TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> computeCumulativeHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src)
{
    return computeCumulativeHistogram(src.first, src.second, src.third);
}

template <class SrcIterator, class SrcAccessor>
TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> computeCumulativeRGBHistogram(SrcIterator sul, SrcIterator slr, SrcAccessor as)
{
    TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> histogram = computeRGBHistogram(sul, slr, as);
    
    // compute cumulative histogram
    long sumR = 0;
    long sumG = 0;
    long sumB = 0;
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++) {
        sumR += histogram[0][i];
        histogram[0][i] = sumR;
        sumG += histogram[1][i];
        histogram[1][i] = sumG;
        sumB += histogram[2][i];
        histogram[2][i] = sumB;
    }
    
    return histogram;
}

template <class SrcIterator, class SrcAccessor>
TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> computeCumulativeRGBHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src)
{
    return computeCumulativeRGBHistogram(src.first, src.second, src.third);
}

template <class ValueType, int Size>
class matchingFunctionFunctor
{
    public:
        matchingFunctionFunctor(TinyVector<ValueType, Size> function) : mf(function)
        {}
        
        ValueType operator()(ValueType const& v) const
        {
            return mf[v];
        }
        
    protected:
        TinyVector<ValueType, Size> mf;
};

// functor used for Lab images
template <class PixelType, class ValueType, int Size>
class matchingFunctionFunctorLab
{
    public:
        matchingFunctionFunctorLab(TinyVector<ValueType, Size> function) : mf(function)
        {}
        
        TinyVector<PixelType, 3> operator()(TinyVector<PixelType, 3> const& v) const
        {
            TinyVector<PixelType, 3> retVal = v;
            retVal[0] = mf[v[0]];
            return retVal;
        }
        
    protected:
        TinyVector<ValueType, Size> mf;
};

// functor used for RGB images
template <class PixelType, class ValueType, int Size>
class matchingFunctionFunctorRGB
{
    public:
        matchingFunctionFunctorRGB(TinyVector<TinyVector<ValueType, Size>, 3> function) : mf(function)
        {}
        
        RGBValue<PixelType> operator()(RGBValue<PixelType> const& v) const
        {
            RGBValue<PixelType> retVal = v;
            retVal[0] = mf[0][v[0]];
            retVal[1] = mf[1][v[1]];
            retVal[2] = mf[2][v[2]];
            return retVal;
        }
        
    protected:
        TinyVector<TinyVector<ValueType, Size>, 3> mf;
};

/**
 * Grayscale.
 */
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor, class Histogram>
void matchHistogram(SrcIterator sul, SrcIterator slr, SrcAccessor as,
                        DestIterator dul, DestAccessor ad, Histogram refHistogram)
{
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> histogram(0);
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> matchingFunction(0);
    
    histogram = computeCumulativeHistogram(sul, slr, as);
    
    // for each gray level G2 from reference histogram
    // find G1 from current picture histogram to create matching function
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++) {
        // find position in reference histogram
        for (int j = 0; j < HISTOGRAM_HISTOGRAM_SIZE; j++) {
            if (histogram[i] == refHistogram[j]) {
                matchingFunction[i] = j;
                break;
            }
        }
    }
    
    // match histogram using functor
    matchingFunctionFunctor<int, HISTOGRAM_HISTOGRAM_SIZE> mf(matchingFunction);
    transformImage(sul, slr, as, dul, ad, mf);
}

/**
 * RGB Color.
 */
template <class SrcIterator, class SrcRGBValue,
          class DestIterator, class DestRGBValue, class Histogram>
void matchRGBHistogram(SrcIterator sul, SrcIterator slr, RGBAccessor<SrcRGBValue> as,
                        DestIterator dul, RGBAccessor<DestRGBValue> ad, Histogram refHistogram)
{
    TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> histogram;
    TinyVector<TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE>, 3> matchingFunction;
    
    histogram = computeCumulativeRGBHistogram(sul, slr, as);
    
    // for each level G2 from reference histogram
    // find G1 from current picture histogram to create matching function
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++) {
        // find position in reference histogram
        for (int j = 0; j < HISTOGRAM_HISTOGRAM_SIZE; j++) {
            for (int k = 0; k < 3; k++) {
                if (histogram[k][i] == refHistogram[k][j]) {
                    matchingFunction[k][i] = j;
                }
            }
        }
    }
    
    // match histogram using functor
    matchingFunctionFunctorRGB<UInt8, int, HISTOGRAM_HISTOGRAM_SIZE> mf(matchingFunction);
    transformImage(sul, slr, as, dul, ad, mf);
}

template <class SrcIterator, class SrcAccessor, class DestIterator,
            class DestAccessor, class Histogram>
void matchRGBHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                    pair<DestIterator, DestAccessor> dest, Histogram refHistogram)
{
    matchRGBHistogram(src.first, src.second, src.third,
                      dest.first, dest.second, refHistogram);
}

/**
 * Color.
 */
template <class SrcIterator, class SrcRGBValue,
          class DestIterator, class DestRGBValue, class Histogram>
void matchHistogram(SrcIterator sul, SrcIterator slr, RGBAccessor<SrcRGBValue> as,
                        DestIterator dul, RGBAccessor<DestRGBValue> ad, Histogram refHistogram)
{
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> histogram(0);
    TinyVector<int, HISTOGRAM_HISTOGRAM_SIZE> matchingFunction(0);
    
    histogram = computeCumulativeHistogram(sul, slr, as);
    
    // for each gray level G2 from reference histogram
    // find G1 from current picture histogram to create matching function
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++) {
        // find position in reference histogram
        for (int j = 0; j < HISTOGRAM_HISTOGRAM_SIZE; j++) {
            if (histogram[i] == refHistogram[j]) {
                matchingFunction[i] = j;
                break;
            }
        }
    }
    
    // Convert to Lab
    typedef BasicImage<TinyVector<UInt8, 3> > Uint8LabImage;
    Uint8LabImage LabImage(slr.x-sul.x, slr.y - sul.y);
    RGB2LabFunctor<UInt8> RGB2Lab;
    transformImage(sul, slr, as, LabImage.upperLeft(), LabImage.accessor(), RGB2Lab);
    
    // match histogram of Lab image using functor
    matchingFunctionFunctorLab<UInt8, int, HISTOGRAM_HISTOGRAM_SIZE> mf(matchingFunction);
    transformImage(srcImageRange(LabImage), destImage(LabImage), mf);
    
    // convert back to RGB
    Lab2RGBFunctor<UInt8> Lab2RGB;
    transformImage(LabImage.upperLeft(), LabImage.lowerRight(), LabImage.accessor(), dul, ad, Lab2RGB);
}

template <class SrcIterator, class SrcAccessor, class DestIterator,
            class DestAccessor, class Histogram>
void matchHistogram(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                    pair<DestIterator, DestAccessor> dest, Histogram refHistogram)
{
    matchHistogram(src.first, src.second, src.third,
                      dest.first, dest.second, refHistogram);
}

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
    int histogram[HISTOGRAM_HISTOGRAM_SIZE];
    // initialize histogram to all zero
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++)
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
    int LUT[HISTOGRAM_HISTOGRAM_SIZE];
    long sum = 0;
    for (int i = 0; i <HISTOGRAM_HISTOGRAM_SIZE; i++) {
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
    int histogram[HISTOGRAM_HISTOGRAM_SIZE];
    // initialize histogram to all zero
    for (int i = 0; i < HISTOGRAM_HISTOGRAM_SIZE; i++)
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
    int LUT[HISTOGRAM_HISTOGRAM_SIZE];
    long sum = 0;
    for (int i = 0; i <HISTOGRAM_HISTOGRAM_SIZE; i++) {
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
