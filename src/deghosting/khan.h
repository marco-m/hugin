
/**
 * Header file for Khan's deghosting algorithm
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
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KHAN_H_
#define KHAN_H_

#include "deghosting.h"
// for AlgTinyVector, NormalizeFunctor and LogarithmFunctor
#include "support.h"
#include "algtinyvector.h"
#include "imageloop.h"

// needed for RGB2Lab
#include <vigra/imageinfo.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/colorconversions.hxx>

#include <vigra/rgbvalue.hxx>

// needed for Kh()
#define PI 3.14159265358979323846

#if defined WIN32
    #define snprintf _snprintf
#endif

#ifdef DEGHOSTING_CACHE_IMAGES
    #include <cstring>
    #include <vigra/cachedfileimage.hxx>
#endif

using namespace vigra;

namespace deghosting
{
    template <class PixelType>
    class ImageTypes {
        public:
            #ifdef DEGHOSTING_CACHE_IMAGES
                typedef CachedFileImage<float> ProcessImageType;
            #else
                typedef BasicImage<float> ProcessImageType;
            #endif
    };

    template <class PixelType>
    class ImageTypes<RGBValue<PixelType> > {
        public:
            #ifdef DEGHOSTING_CACHE_IMAGES
                typedef CachedFileImage<AlgTinyVector<float, 3> > ProcessImageType;
            #else
                typedef BasicImage<AlgTinyVector<float, 3> > ProcessImageType;
            #endif
    };

    template <class PixelType>
    class Khan : public Deghosting, private ImageTypes<PixelType>
    {
        public:
            Khan(std::vector<std::string>& inputFiles, const uint16_t flags, const uint16_t debugFlags, int iterations, float sigma, int verbosity);
            Khan(std::vector<ImageImportInfo>& inputFiles, const uint16_t flags, const uint16_t debugFlags, int iterations, float sigma, int verbosity);
            std::vector<FImagePtr> createWeightMasks();
            ~Khan() {}
        protected:
            // input image (eg. FRGBImage etc)
            typedef BasicImage<PixelType> ImageType;
            // ProcessImageType is used for storing intermediate images
            // for processing (ie input images after all necessary transformations done)
            // they are defined as floats to speed up the multiplication with weight which is float
            typedef typename ImageTypes<PixelType>::ProcessImageType ProcessImageType;
            typedef typename ProcessImageType::traverser ProcessImageTraverser;
            typedef typename ProcessImageType::PixelType ProcessImagePixelType;
            typedef typename NumericTraits<ProcessImagePixelType>::ValueType ProcessImageValueType;
            typedef typename NumericTraits<ProcessImageValueType>::RealPromote ProcessImageValueTypeRealPromote;
            // inteligent pointer to process image
            typedef boost::shared_ptr<ProcessImageType> ProcessImageTypePtr;
            // used to determine whether input images are scalar
            typedef typename NumericTraits<PixelType>::isScalar srcIsScalar;
            
            // Kh() things
            // sigma in gaussian density function
            ProcessImageValueType sigma;
            
            // other necessary stuff
            std::vector<ProcessImageTypePtr> processImages;
            std::vector<FImagePtr> weights;
            
            /** Set sigma.
             * Sets sigma for Gaussian weighting function.
             */
            void setSigma(float sigma);
            
            /** transform image using EMoR response
             * @param inputFile filename of image to be transformed
             * @param *pInputImg FRGBImage to be transformed
             */
            //void linearizeRGB(std::string, FRGBImage* pInputImg);
            
            /** Convert image for internal use.
             * if input image is RGB then convert it to L*a*b
             * if input image is grayscale then only copy image
             */
            void convertImage(ImageType * in, ProcessImageTypePtr & out, VigraFalseType);
            void convertImage(ImageType * in, ProcessImageTypePtr & out, VigraTrueType);
            
            /** Import RGB image.
             */
            void importRGBImage(ImageImportInfo & info, ImageType * img, VigraFalseType);
            void importRGBImage(ImageImportInfo & info, ImageType * img, VigraTrueType);
            
            /** Function for input image preprocessing.
             * This function loads image, linearize it using EMoR (FIXME) and
             * transforms it using logarithm or gamma if input images are HDR.
             */
            void preprocessImage(unsigned int i, FImagePtr &weight, ProcessImageTypePtr &output);
    };
    
    template <class PixelType>
    Khan<PixelType>::Khan(std::vector<std::string>& newInputFiles, const uint16_t newFlags, const uint16_t newDebugFlags,
                int newIterations, float newSigma, int newVerbosity) {
        try {
            Deghosting::loadImages(newInputFiles);
            Deghosting::setFlags(newFlags);
            Deghosting::setDebugFlags(newDebugFlags);
            Deghosting::setIterationNum(newIterations);
            Deghosting::setVerbosity(newVerbosity);
            
            // I don't know why, but sigma for HDR input have to approximately 10 times smaller
            // FIXME: Maybe it would be better to use different sigma for different images in case both HDR and LDR are mixed
            const char * fileType= ImageImportInfo(newInputFiles[0].c_str()).getFileType();
            if ( (!strcmp(fileType,"TIFF") && strcmp(fileType,"UINT8")) || !strcmp(fileType,"EXR") || !strcmp(fileType,"FLOAT")) {
                setSigma(newSigma/10);
            } else {
                setSigma(newSigma);
            }
            
            for (unsigned int i=0; i<5; i++)
                Deghosting::response.push_back(0);
        } catch (...) {
            throw;
        }
    }
    
    template <class PixelType>
    Khan<PixelType>::Khan(std::vector<ImageImportInfo>& newInputFiles, const uint16_t newFlags, const uint16_t newDebugFlags,
                int newIterations, float newSigma, int newVerbosity) {
        try {
            Deghosting::loadImages(newInputFiles);
            Deghosting::setFlags(newFlags);
            Deghosting::setDebugFlags(newDebugFlags);
            Deghosting::setIterationNum(newIterations);
            Deghosting::setVerbosity(newVerbosity);
            
            // I don't know why, but sigma for HDR input have to approximately 10 times smaller
            // FIXME: Maybe it would be better to use different sigma for different images in case both HDR and LDR are mixed
            const char * fileType= newInputFiles[0].getFileType();
            if ( (!strcmp(fileType,"TIFF") && strcmp(fileType,"UINT8")) || !strcmp(fileType,"EXR") || !strcmp(fileType,"FLOAT")) {
                setSigma(newSigma/10);
            } else {
                setSigma(newSigma);
            }
            
            for (unsigned int i=0; i<5; i++)
                Deghosting::response.push_back(0);
        } catch (...) {
            throw;
        }
    }
    
    template <class PixelType>
    void Khan<PixelType>::setSigma(float newSigma) {
        sigma = newSigma;
    }
    
    /*void Khan::linearizeRGB(std::string inputFile,FRGBImage *pInputImg) {
        HuginBase::SrcPanoImage panoImg(inputFile);
        panoImg.setResponseType(HuginBase::SrcPanoImage::RESPONSE_EMOR);
        panoImg.setEMoRParams(response);
        // response transform functor
        HuginBase::Photometric::InvResponseTransform<RGBValue<float>,
                                                     RGBValue<float> > invResponse(panoImg);
        invResponse.enforceMonotonicity();

        // iterator to the upper left corner
        FRGBImage::traverser imgIterSourceY = pInputImg->upperLeft();
        // iterator to he lower right corner
        FRGBImage::traverser imgIterEnd = pInputImg->lowerRight();
        // iterator to the output
        FRGBImage::traverser imgIterOut = pInputImg->upperLeft();
        // loop through the image
        for (int y=1; imgIterSourceY.y != imgIterEnd.y; ++imgIterSourceY.y, ++imgIterOut.y, ++y) {
            // iterator to the input
            FRGBImage::traverser sx = imgIterSourceY;
            // iterator to the output
            FRGBImage::traverser dx = imgIterOut;
            for (int x=1; sx.x != imgIterEnd.x; ++sx.x, ++dx.x, ++x) {
                // transform image using response
                *dx = vigra_ext::zeroNegative(invResponse(*sx, hugin_utils::FDiff2D(x, y)));
            }
        }
    }*/
    
    // RGB
    template <class PixelType>
    void Khan<PixelType>::convertImage(ImageType * in, ProcessImageTypePtr & out, VigraFalseType) {
        RGB2LabFunctor<ProcessImageValueType> RGB2Lab;
        transformImage(srcImageRange(*in), destImage(*out), RGB2Lab);
    }
    
    // grayscale
    template <class PixelType>
    void Khan<PixelType>::convertImage(ImageType* in, ProcessImageTypePtr& out, VigraTrueType) {
        copyImage(srcImageRange(*in), destImage(*out));
    }
    
    // load image and convert it to grayscale
    template <class PixelType>
    void Khan<PixelType>::importRGBImage(ImageImportInfo & info, ImageType * img, VigraTrueType) {
        // NOTE: I guess this is not optimal, but it works
        RGBToGrayAccessor<FRGBImage::PixelType> color2gray;
        FRGBImage tmpImg(info.size());
        if (info.numBands() == 4) {
            BImage imgAlpha(info.size());
            importImageAlpha(info, destImage(tmpImg), destImage(imgAlpha));
        } else {
            importImage(info, destImage(tmpImg));
        }
        transformImage(srcImageRange(tmpImg, color2gray), destImage(*img), log(Arg1()+Param(1.0f)));
    }
    
    // only load image
    template <class PixelType>
    void Khan<PixelType>::importRGBImage(ImageImportInfo & info, ImageType * img, VigraFalseType) {
        if (info.numBands() == 4) {
            BImage imgAlpha(info.size());
            importImageAlpha(info, destImage(*img), destImage(imgAlpha));
        } else {
            importImage(info, destImage(*img));
        }
    }
    
    template <class PixelType>
    void Khan<PixelType>::preprocessImage(unsigned int i, FImagePtr &weight, ProcessImageTypePtr &output) {
        ImageImportInfo imgInfo(inputFiles[i]);
        ImageType * pInputImg =  new ImageType(imgInfo.size());
        weight = FImagePtr(new FImage(imgInfo.size()));
        output = ProcessImageTypePtr(new ProcessImageType(imgInfo.size()));
        
        // import image. srcIsScalar() determines whether we want to use grayscale images for computations
        // NOTE: Maybe alpha can be of some use but I don't know about any now
        if (imgInfo.isColor()) {
            importRGBImage(imgInfo, pInputImg, srcIsScalar());
        } else {
            importImage(imgInfo, destImage(*pInputImg));
        }
        
        // linearize RGB and convert it to L*a*b image
        //linearizeRGB(inputFiles[i], pInputImg);
        
        // take logarithm or gamma correction if the input images are HDR
        // I'm not sure if it's the right way how to
        // find out if they are HDR
        const char * fileType= imgInfo.getFileType();
        if ( (!strcmp(fileType,"TIFF") && strcmp(fileType,"UINT8")) || !strcmp(fileType,"EXR") || !strcmp(fileType,"FLOAT")) {
            // use gamma 2.2
            if (flags & ADV_GAMMA) {
                // GammaFunctor is only in vigra 1.6 GRRR
                // I have to use BrightnessContrastFunctor
                // TODO: change to the GammaFunctor in the future
                vigra::FindMinMax<float> minmax;
                vigra::inspectImage(srcImageRange(*pInputImg), minmax);
                transformImage(srcImageRange(*pInputImg),destImage(*pInputImg),BrightnessContrastFunctor<PixelType>(0.45,1.0,minmax.min, minmax.max));
            } else {
                // take logarithm
                transformImage(srcImageRange(*pInputImg),destImage(*pInputImg),LogarithmFunctor<PixelType>(1.0));
            }
        }
        
        // generate initial weights
        transformImage(srcImageRange(*pInputImg),destImage(*weight),HatFunctor<PixelType>());
        
        convertImage(pInputImg, output, srcIsScalar());
        
        delete pInputImg;
        pInputImg = 0;
    }
    
    template <class PixelType>
    std::vector<FImagePtr> Khan<PixelType>::createWeightMasks() {
        for (unsigned int i = 0; i < inputFiles.size(); i++) {
            FImagePtr weight;
            ProcessImageTypePtr processImage;
            preprocessImage(i, weight, processImage);
            processImages.push_back(processImage);
            weights.push_back(weight);
            
            // save init weights
            if (debugFlags & SAVE_INITWEIGHTS) {
                char tmpfn[100];
                snprintf(tmpfn, 99, "init_weights_%d.tiff", i);
                ImageExportInfo exWeights(tmpfn);
                exportImage(srcImageRange(*weight), exWeights.setPixelType("UINT8"));
            }
        }
        
        imageLoop(this, weights, processImages, GaussianDensityFunctor<ProcessImageValueType, ProcessImagePixelType>(sigma));
        
        return weights;
    }
}

#endif /* KHAN_H_ */
