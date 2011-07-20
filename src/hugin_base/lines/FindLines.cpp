// -*- c-basic-offset: 4 -*-
/**  @file FindLines.cpp
 *
 *  @brief functions for finding lines
 *
 */

/***************************************************************************
 *   Copyright (C) 2009 by Tim Nugent                                      *
 *   timnugent@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "vigra/edgedetection.hxx"
#include "FindLines.h"
#include "FindN8Lines.h"
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <nona/RemappedPanoImage.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include "algorithms/basic/CalculateCPStatistics.h"

using namespace vigra;
using namespace std;

namespace HuginLines
{
double resize_image(UInt8RGBImage& in, UInt8RGBImage& out, int resize_dimension)
{
    // Re-size to max dimension
    double sizefactor=1.0;
    if (in.width() > resize_dimension || in.height() > resize_dimension)
    {
        int nw;
        int nh;
        if (in.width() >= in.height())
        {
            sizefactor = (double)resize_dimension/in.width();
            // calculate new image size
            nw = resize_dimension;
            nh = static_cast<int>(0.5 + (sizefactor*in.height()));
        }
        else
        {
            sizefactor = (double)resize_dimension/in.height();
            // calculate new image size
            nw = static_cast<int>(0.5 + (sizefactor*in.width()));
            nh = resize_dimension;
        }

        // create an image of appropriate size
        out.resize(nw, nh);
        // resize the image, using a bi-cubic spline algorithm
        resizeImageNoInterpolation(srcImageRange(in),destImageRange(out));
    }
    else
    {
        out.resize(in.size());
        copyImage(srcImageRange(in),destImage(out));
    };
    return 1.0/sizefactor;
}

vigra::BImage* detectEdges(UInt8RGBImage input,double scale,double threshold,unsigned int resize_dimension, double& size_factor)
{
    // Resize image
    UInt8RGBImage scaled;
    size_factor=resize_image(input, scaled, resize_dimension);
    input.resize(0,0);

    // Convert to greyscale
    BImage grey(scaled.width(), scaled.height());
    copyImage(srcImageRange(scaled, RGBToGrayAccessor<RGBValue<UInt16> >()), destImage(grey));

    // Run Canny edge detector
    BImage* image=new BImage(grey.width(), grey.height(), 255);
    cannyEdgeImage(srcImageRange(grey), destImage(*image), scale, threshold, 0);
    return image;
};

double calculate_focal_length_pixels(double focal_length,double cropFactor,double width, double height)
{
    double pixels_per_mm = 0;
    if (cropFactor > 1)
    {
        pixels_per_mm= (cropFactor/24.0)* ((width>height)?height:width);
    }
    else
    {
        pixels_per_mm= (24.0/cropFactor)* ((width>height)?height:width);
    }
    return focal_length*pixels_per_mm;
}


Lines findLines(vigra::BImage& edge, double length_threshold, double focal_length,double crop_factor)
{
    unsigned int longest_dimension=(edge.width() > edge.height()) ? edge.width() : edge.height();
    double min_line_length_squared=(length_threshold*longest_dimension)*(length_threshold*longest_dimension);

    int lmin = int(sqrt(min_line_length_squared));
    double flpix=calculate_focal_length_pixels(focal_length,crop_factor,edge.width(),edge.height());

    BImage lineImage = edgeMap2linePts(edge);
    Lines lines;
    int nlines = linePts2lineList( lineImage, lmin, flpix, lines );

    return lines;
};

void ScaleLines(Lines& lines,const double scale)
{
    for(unsigned int i=0; i<lines.size(); i++)
    {
        for(unsigned int j=0; j<lines[i].line.size(); j++)
        {
            lines[i].line[j]*=scale;
        };
    };
};

HuginBase::CPVector GetControlPoints(const SingleLine line,const unsigned int imgNr, const unsigned int lineNr,const unsigned int numberOfCtrlPoints)
{
    HuginBase::CPVector cpv;
    double interval = (line.line.size()-1)/(1.0*numberOfCtrlPoints);
    for(unsigned int k = 0; k < numberOfCtrlPoints; k++)
    {
        int start = (int)(k * interval);
        int stop =  (int)((k+1) * interval);
        HuginBase::ControlPoint cp(imgNr,line.line[start].x, line.line[start].y,
                                   imgNr,line.line[stop].x, line.line[stop].y,lineNr);
        cpv.push_back(cp);
    };
    return cpv;
};

#define MAX_RESIZE_DIM 1600

struct VerticalLine
{
    vigra::Point2D start;
    vigra::Point2D end;
};

typedef std::vector<VerticalLine> VerticalLineVector;

VerticalLineVector FilterLines(Lines lines,double roll)
{
    VerticalLineVector vertLines;
    if(lines.size()>0)
    {
        for(Lines::const_iterator it=lines.begin(); it!=lines.end(); it++)
        {
            if((*it).status==valid_line)
            {
                VerticalLine vl;
                vl.start=(*it).line[0];
                vl.end=(*it).line[(*it).line.size()-1];
                vigra::Diff2D diff=vl.end-vl.start;
                if(diff.magnitude()>20)
                {
                    if(abs((diff.x*cos(DEG_TO_RAD(roll))+diff.y*sin(DEG_TO_RAD(roll)))/diff.magnitude())<0.1)
                    {
                        vertLines.push_back(vl);
                    };
                };
            };
        };
    };
    return vertLines;
};

HuginBase::CPVector GetVerticalLines(const HuginBase::Panorama& pano,const unsigned int imgNr,vigra::UInt8RGBImage& image)
{
    HuginBase::CPVector verticalLines;
    const HuginBase::SrcPanoImage& srcImage=pano.getImage(imgNr);
    bool needsRemap=srcImage.getProjection()!=HuginBase::SrcPanoImage::RECTILINEAR;
    double roll=(needsRemap?0:srcImage.getRoll());
    double size_factor=1.0;
    HuginBase::SrcPanoImage remappedImage;
    HuginBase::PanoramaOptions opts;
    vigra::BImage* edge;
    if(!needsRemap)
    {
        //rectilinear image can be used as is
        //detect edges
        edge=detectEdges(image,2,4,MAX_RESIZE_DIM,size_factor);
    }
    else
    {
        //remap all other image to cylindrical
        //create temporary SrcPanoImage, set appropriate image variables
        remappedImage=pano.getSrcImage(imgNr);
        remappedImage.setYaw(0);
        remappedImage.setPitch(0);
        remappedImage.setX(0);
        remappedImage.setY(0);
        remappedImage.setZ(0);
        remappedImage.setExposureValue(0);
        remappedImage.setEMoRParams(std::vector<float>(5, 0.0));
        remappedImage.deleteAllMasks();
        remappedImage.setActive(true);
        //create PanoramaOptions for remapping of image
        opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
        opts.setWidth(MAX_RESIZE_DIM);
        opts.outputExposureValue=0;
        //calculate output canvas size
        HuginBase::Panorama tempPano;
        tempPano.addImage(remappedImage);
        tempPano.setOptions(opts);

        HuginBase::CalculateFitPanorama fitPano(tempPano);
        fitPano.run();
        opts.setHFOV(fitPano.getResultHorizontalFOV());
        opts.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        tempPano.setOptions(opts);

        //finally remap image
        HuginBase::Nona::RemappedPanoImage<vigra::UInt8RGBImage,vigra::BImage>* remapped=new HuginBase::Nona::RemappedPanoImage<vigra::UInt8RGBImage,vigra::BImage>;
        AppBase::MultiProgressDisplay* progress=new AppBase::DummyMultiProgressDisplay();
        remapped->setPanoImage(remappedImage,opts,opts.getROI());
        remapped->remapImage(vigra::srcImageRange(image),vigra_ext::INTERP_CUBIC,*progress);
        vigra::UInt8RGBImage remappedBitmap=remapped->m_image;
        edge=detectEdges(remappedBitmap,2,4,std::max(remappedBitmap.width(),remappedBitmap.height())+10,size_factor);
        delete remapped;
        delete progress;
    };
    //detect lines
    //we need the focal length
    double focalLength=srcImage.getExifFocalLength();
    if(focalLength==0)
    {
        focalLength=HuginBase::SrcPanoImage::calcFocalLength(
            srcImage.getProjection(),srcImage.getHFOV(),srcImage.getExifCropFactor(),srcImage.getSize());
    };
    Lines foundLines=findLines(*edge,0.05,focalLength,srcImage.getExifCropFactor());
    //filter results
    VerticalLineVector filteredLines=FilterLines(foundLines,roll);
    //create control points
    if(filteredLines.size()>0)
    {
        //we need to transform the coordinates to image coordinates because the detection
        //worked on smaller images or in remapped image
        HuginBase::PTools::Transform transform;
        if(needsRemap)
        {
            transform.createTransform(remappedImage,opts);
        };
        for(size_t i=0; i<filteredLines.size(); i++)
        {
            HuginBase::ControlPoint cp;
            cp.image1Nr=imgNr;
            cp.image2Nr=imgNr;
            cp.mode=HuginBase::ControlPoint::X;
            if(!needsRemap)
            {
                cp.x1=filteredLines[i].start.x*size_factor;
                cp.y1=filteredLines[i].start.y*size_factor;
                cp.x2=filteredLines[i].end.x*size_factor;
                cp.y2=filteredLines[i].end.y*size_factor;
            }
            else
            {
                double xout;
                double yout;
                if(!transform.transformImgCoord(xout,yout,filteredLines[i].start.x,filteredLines[i].start.y))
                {
                    continue;
                };
                cp.x1=xout;
                cp.y1=yout;
                if(!transform.transformImgCoord(xout,yout,filteredLines[i].end.x,filteredLines[i].end.y))
                {
                    continue;
                };
                cp.x2=xout;
                cp.y2=yout;
            };
            verticalLines.push_back(cp);
        };
        //now a final check of the found vertical lines
        //we optimize the pano with a single image and disregard vertical lines with bigger errors
        if(verticalLines.size()>0)
        {
            HuginBase::Panorama tempPano;
            HuginBase::SrcPanoImage tempImage=pano.getSrcImage(imgNr);
            tempImage.setYaw(0);
            tempImage.setX(0);
            tempImage.setY(0);
            tempImage.setZ(0);
            tempPano.addImage(tempImage);
            for(size_t i=0; i<verticalLines.size(); i++)
            {
                HuginBase::ControlPoint newCP=verticalLines[i];
                newCP.image1Nr=0;
                newCP.image2Nr=0;
                tempPano.addCtrlPoint(newCP);
            };
            HuginBase::PanoramaOptions opt2;
            opt2.setProjection(HuginBase::PanoramaOptions::CYLINDRICAL);
            tempPano.setOptions(opt2);
            HuginBase::OptimizeVector optVec;
            std::set<std::string> imgopt;
            imgopt.insert("p");
            imgopt.insert("r");
            optVec.push_back(imgopt);
            tempPano.setOptimizeVector(optVec);
            HuginBase::PTools::optimize(tempPano);
            //calculate statistic and determine limit
            double min,max,mean,var;
            HuginBase::CalculateCPStatisticsError::calcCtrlPntsErrorStats(tempPano,min,max,mean,var);
            double limit=mean+sqrt(var);
            HuginBase::CPVector cps=tempPano.getCtrlPoints();
            for(int i=cps.size()-1; i>=0; i--)
            {
                if(cps[i].error>limit)
                {
                    verticalLines.erase(verticalLines.begin()+i);
                };
            };
        };
    };
    return verticalLines;
};

}; //namespace