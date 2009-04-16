// -*- c-basic-offset: 4 -*-

/** @file PanoramaMemento.cpp
 *
 *  @brief implementation of PanoramaMemento Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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


#include <hugin_config.h>

#include "PanoramaOptions.h"

#include <hugin_utils/utils.h>
#include <hugin_math/hugin_math.h>
#include <panotools/PanoToolsInterface.h>


namespace HuginBase {

using namespace hugin_utils;
using namespace vigra;


const std::string & PanoramaOptions::getFormatName(FileFormat f)
{
    assert((int)f <= (int)FILEFORMAT_NULL);
    return fileformatNames[(int) f];
}

const std::string & PanoramaOptions::getOutputExtension() const
{
    assert((int)outputFormat < (int)FILEFORMAT_NULL);
    return fileformatExt[(int) outputFormat];
}

PanoramaOptions::FileFormat PanoramaOptions::getFormatFromName(const std::string & name)
{
    int max = (int) FILEFORMAT_NULL;
    int i;
    for (i=0; i<max; i++) {
        if (name == fileformatNames[i]) {
            break;
        }
    }
    if (i+1 == max) {
        DEBUG_ERROR("could not parse format " << name );
        return TIFF_m;
    }
    return (FileFormat) i;
}


void PanoramaOptions::printScriptLine(std::ostream & o, bool forPTOptimizer) const
{
    o << "p f" << m_projectionFormat << " w" << getWidth()<< " h" << getHeight()
            << " v" << getHFOV() << " ";

    if (! forPTOptimizer) {
        switch (colorCorrection) {
        case NONE:
            break;
        case BRIGHTNESS_COLOR:
            o << " k" << colorReferenceImage;
            break;
        case BRIGHTNESS:
            o << " b" << colorReferenceImage;
            break;
        case COLOR:
            o << " d" << colorReferenceImage;
            break;
        }

        // the new exposure options
        o << " E" << outputExposureValue;
        o << " R" << outputMode;
        if (outputPixelType.size() > 0) {
            o << " T" << outputPixelType;
        }
        if (m_projectionParams.size() > 0) {
            o << " P\"";
            for (int i=0; i < (int) m_projectionParams.size(); i++) {
                o << m_projectionParams[i];
                if (i+1 < (int)m_projectionParams.size())
                    o << " ";
            }
            o << "\"";
        }
        if (m_roi != vigra::Rect2D(m_size)) {
            o << " S" << m_roi.left() << "," << m_roi.right() << "," << m_roi.top() << "," << m_roi.bottom();
        }
    }

    o << " n\"" << getFormatName(outputFormat);
    if ( outputFormat == JPEG ) {
        o << " q" << quality;
    } else if ( outputFormat == TIFF ||
                outputFormat == TIFF_m ||
                outputFormat == TIFF_mask ||
                outputFormat == TIFF_multilayer ||
                outputFormat == TIFF_multilayer_mask)
    {
        o << " c:" << tiffCompression;
        if (tiff_saveROI) {
            o << " r:CROP";
        }
    }
    o << "\"";
    o << std::endl;

    // misc options
    o << "m g" << gamma << " i" << interpolator;
    switch (remapAcceleration) {
    case NO_SPEEDUP:
        break;
    case MAX_SPEEDUP:
        o << " f0";
        break;
    case MEDIUM_SPEEDUP:
        o << " f1";
    }
    o << " m" << huberSigma;

    // options for photometric estimation.
    o << " p" << photometricHuberSigma;
    if (photometricSymmetricError) 
        o << " s1";

    o << std::endl;
}

void PanoramaOptions::setProjection(ProjectionFormat f)
{
#ifdef HasPANO13
    if ((int) f >= panoProjectionFormatCount()) {
        // reset to equirect if this projection is not known
        f = EQUIRECTANGULAR;
    }
#endif
    // only try to keep the FOV if calculations are implemented for
    // both projections
    if (fovCalcSupported(m_projectionFormat) && fovCalcSupported(f)) 
    {
        // keep old fov
        // correct image width and height
        PanoramaOptions copy(*this);
        copy.m_projectionFormat = f;
        double hfov = std::min(getHFOV(), copy.getMaxHFOV());
        double vfov = std::min(getVFOV(), copy.getMaxVFOV());
        setHFOV(hfov, false);
        setVFOV(vfov);
#ifdef HasPANO13
        panoProjectionFeaturesQuery(f, &m_projFeatures);
#endif
        m_projectionFormat = f;
#ifdef HasPANO13
        m_projectionParams.resize(m_projFeatures.numberOfParameters);
        if (m_projFeatures.numberOfParameters) {
            if (f == ALBERS_EQUAL_AREA_CONIC) {
                m_projectionParams[0] = 0;
                m_projectionParams[1] = 60;
            };
			if (f == BIPLANE)
			{
				m_projectionParams[0] = 45;
			};
			if (f == TRIPLANE)
			{
				m_projectionParams[0] = 60;
			};
        }
#endif
        setHFOV(hfov, false);
        setVFOV(vfov);
    } else {
        m_projectionFormat = f;
#ifdef HasPANO13
        panoProjectionFeaturesQuery(f, &m_projFeatures);
        m_projectionParams.resize(m_projFeatures.numberOfParameters);
        if (m_projFeatures.numberOfParameters) {
            if (f == ALBERS_EQUAL_AREA_CONIC) {
                m_projectionParams[0] = 0;
                m_projectionParams[1] = 60;
            };
			if (f == BIPLANE)
			{
				m_projectionParams[0] = 45;
			};
			if (f == TRIPLANE)
			{
				m_projectionParams[0] = 60;
			};
        }
#endif
        double hfov = std::min(getHFOV(), getMaxHFOV());
        setHFOV(hfov, false);
    }
}


void PanoramaOptions::setProjectionParameters(const std::vector<double> & params)
{
#ifdef HasPANO13
    assert(m_projFeatures.numberOfParameters == (int) params.size());
    // check if the parameters are good.
    if (m_projFeatures.numberOfParameters == (int) params.size()) {
        m_projectionParams = params;
        // enforce limits.
        for (size_t i=0; i < params.size(); i++) {
            if (m_projectionParams[i] > m_projFeatures.parm[i].maxValue) {
                m_projectionParams[i] = m_projFeatures.parm[i].maxValue;
            }
            if (m_projectionParams[i] < m_projFeatures.parm[i].minValue) {
                m_projectionParams[i] = m_projFeatures.parm[i].minValue;
            }
        }
    }
#endif
}

bool PanoramaOptions::fovCalcSupported(ProjectionFormat f) const
{
    /*
#ifdef HasPANO13
     pano_projection_features pfeat;
     if (panoProjectionFeaturesQuery((int) m_projectionFormat, &pfeat)) {
         return pfeat.maxVFOV <=180;
     } else {
         return false;
     }
#else
     */
    return ( f == RECTILINEAR
             || f == CYLINDRICAL
             || f == EQUIRECTANGULAR
             || f == MERCATOR
             || f == SINUSOIDAL 
             || f == MILLER_CYLINDRICAL
             || f == PANINI
             || f == ARCHITECTURAL
             || f == EQUI_PANINI
			 || f == BIPLANE
			 || f == TRIPLANE);
    //#endif
}


void PanoramaOptions::setWidth(unsigned int w, bool keepView)
{
    if (m_projectionFormat == EQUIRECTANGULAR || m_projectionFormat == SINUSOIDAL) {
        if (w%2 == 1) {
            w = w+1;
        }
    }
    bool nocrop =  (m_roi == vigra::Rect2D(m_size));
    double scale = w / (double) m_size.x;
    m_size.x = w;
    if (nocrop) {
        m_roi = vigra::Rect2D(m_size);
    } else {
        // for now, do a simple proportional scaling
        m_roi.setUpperLeft(vigra::Point2D(roundi(scale*m_roi.left()), m_roi.top()));
        m_roi.setLowerRight(vigra::Point2D(roundi(scale*m_roi.right()), m_roi.bottom()));
        // ensure ROI is inside the panorama
        m_roi &= vigra::Rect2D(m_size);
    }

    if (keepView) {
        m_size.y = hugin_utils::roundi(m_size.y*scale);
        if (nocrop) {
            m_roi = vigra::Rect2D(m_size);
        } else {
            m_roi.setUpperLeft(vigra::Point2D(m_roi.left(), roundi(scale*m_roi.top())));
            m_roi.setLowerRight(vigra::Point2D(m_roi.right(), roundi(scale*m_roi.bottom())));
            // ensure ROI is inside the panorama
            m_roi &= Rect2D(m_size);
        }
        if (fovCalcSupported(m_projectionFormat)) {
            if (getVFOV() > getMaxVFOV()) {
                setVFOV(getMaxVFOV());
            }
        }
    }

    DEBUG_DEBUG(" HFOV: " << m_hfov << " size: " << m_size << " roi: " << m_roi << "  => vfov: " << getVFOV());
}

void PanoramaOptions::setHeight(unsigned int h) 
{
    bool nocrop =  (m_roi == vigra::Rect2D(m_size));

    if (h == 0) {
        h = 1;
    }
    int dh = h - m_size.y;
    m_size.y = h;
    if (nocrop) {
        m_roi = vigra::Rect2D(m_size);
    } else {
        // move ROI
        m_roi.moveBy(0,dh/2);
        m_roi &= vigra::Rect2D(m_size);
    }

    DEBUG_DEBUG(" HFOV: " << m_hfov << " size: " << m_size << " roi:" << m_roi << "  => vfov: " << getVFOV());
}

void PanoramaOptions::setHFOV(double h, bool keepView)
{
    if (keepView && !fovCalcSupported(m_projectionFormat)) {
        DEBUG_NOTICE("Ignoring keepView");
        keepView = false;
    }

    if (h <= 0) {
        h = 1;
    }
    double vfov;
    if (keepView) {
        vfov = getVFOV();
    }
    m_hfov = std::min(h, getMaxHFOV());
    if (keepView) {
        setVFOV(std::min(vfov, getMaxVFOV()));
    }
}

void PanoramaOptions::setVFOV(double VFOV)
{
    VFOV = std::min(VFOV, getMaxVFOV());

    if (! fovCalcSupported(m_projectionFormat)) {
        return;
    }

    bool nocrop =  (m_roi == vigra::Rect2D(m_size));

    if (VFOV <= 0) {
        VFOV = 1;
    }
    // TODO: create transform from equirect to target projection and
    // set additional
    PTools::Transform transf;
    SrcPanoImage src;
    src.setProjection(SrcPanoImage::EQUIRECTANGULAR);
    src.setHFOV(360);
    src.setSize(vigra::Size2D(360,180));
    transf.createInvTransform(src, *this);

    FDiff2D pmiddle;

    if (VFOV>180 && getMaxVFOV() > 180) {
        // we have crossed the pole
        transf.transform(pmiddle, FDiff2D(180, 180-VFOV/2 - 0.01));
    } else {
        transf.transform(pmiddle, FDiff2D(0, VFOV/2));
    }
    // try to keep the same ROI
    vigra::Size2D oldSize = m_size;
    m_size.y = abs(hugin_utils::roundi(2*pmiddle.y));

    if (nocrop) {
        m_roi = vigra::Rect2D(m_size);
    } else {
        // adjust ROI to stay in previous position
        int dh = m_size.y - oldSize.y;
        m_roi.moveBy(0, dh/2);
        // ensure ROI is visible
        m_roi &= vigra::Rect2D(m_size);
    }

    DEBUG_DEBUG(" HFOV: " << m_hfov << " size: " << m_size << " roi: " << m_roi << "  => vfov: " << VFOV);

}

double PanoramaOptions::getVFOV() const
{
    // calcuale VFOV based on current panorama
    PTools::Transform transf;
    SrcPanoImage src;
    src.setProjection(SrcPanoImage::EQUIRECTANGULAR);
    src.setHFOV(360);
    src.setSize(vigra::Size2D(360,180));
    transf.createTransform(src, *this);

    FDiff2D pmiddle;
    FDiff2D pcorner;
    transf.transform(pmiddle, FDiff2D(0, m_size.y/2.0));
//    transf.transform(pcorner, FDiff2D(m_size.x/2.0, m_size.y/2.0));
    double VFOV;
    if (pmiddle.x > 90 ||pmiddle.y < -90) {
        // the pole has been crossed
        VFOV = 2*(180-pmiddle.y);
    } else {
        VFOV = 2*pmiddle.y;
    }
    //double VFOV = 2.0*std::max(pcorner.y, pmiddle.y);

    /*
    double VFOV;
    switch (m_projectionFormat) {
        case PanoramaOptions::RECTILINEAR:
            VFOV = 2.0 * atan( (double)m_height * tan(DEG_TO_RAD(m_hfov)/2.0) / m_width);
            VFOV = RAD_TO_DEG(VFOV);
            break;
        case PanoramaOptions::CYLINDRICAL:
        {
            // equations: w = f * v (f: focal length, in pixel)
            double f = m_width / DEG_TO_RAD(m_hfov);
            VFOV = 2*atan(m_height/(2.0*f));
            VFOV = RAD_TO_DEG(VFOV);
            break;
        }
        case PanoramaOptions::EQUIRECTANGULAR:
            // FIXME: This is wrong!
        case TRANSVERSE_MERCATOR:
        case MERCATOR:
            VFOV = m_hfov * m_height / m_width;
            break;
        case PanoramaOptions::FULL_FRAME_FISHEYE:
            VFOV = m_hfov * m_height / m_width;
            break;
    }
    */
    DEBUG_DEBUG(" HFOV: " << m_hfov << " size: " << m_size << " roi: " << m_roi << "  => vfov: " << VFOV);
    return VFOV;
}

double PanoramaOptions::getMaxHFOV() const
{
#ifdef HasPANO13
	double angle;
    pano_projection_features pfeat;
    if (panoProjectionFeaturesQuery((int)m_projectionFormat, &pfeat)) {
		switch(m_projectionFormat)
		{
			case BIPLANE:
				if (m_projectionParams.size()>0)
					angle = m_projectionParams[0];
				else
					angle = 0;
				return angle + 179;
			case TRIPLANE:
				if (m_projectionParams.size()>0)
					angle = m_projectionParams[0];
				else
					angle = 0;
				return 2 * angle + 179;
			default:
		return pfeat.maxHFOV;
		}
    } else {
        return 360;
    }
#else
    switch (m_projectionFormat)
    {
        case RECTILINEAR:
        case TRANSVERSE_MERCATOR:
            return 175;
//        case PANINI:
//        case EQUI_PANINI:
//			return 220;
        case STEREOGRAPHIC:
            return 355;
        case CYLINDRICAL:
        case EQUIRECTANGULAR:
        case FULL_FRAME_FISHEYE:
        case MERCATOR:
        case SINUSOIDAL:
        case LAMBERT:
        case LAMBERT_AZIMUTHAL:
        case ARCHITECTURAL:
//        case ORTHOGRAPHIC:
//        case EQUISOLID:
        default:
            return 360;
    }
#endif
}

double PanoramaOptions::getMaxVFOV() const
{
#ifdef HasPANO13
    pano_projection_features pfeat;
    if (panoProjectionFeaturesQuery((int) m_projectionFormat, &pfeat)) {
        return pfeat.maxVFOV;
    } else {
        return 180;
    }
#else
    switch (m_projectionFormat)
    {
        case RECTILINEAR:
        case CYLINDRICAL:
        case MERCATOR:
            return 175;
        default:
        case EQUIRECTANGULAR:
        case LAMBERT:
        case SINUSOIDAL:
            return 180;
        case STEREOGRAPHIC:
            return 355;
        case FULL_FRAME_FISHEYE:
        case TRANSVERSE_MERCATOR:
        case LAMBERT_AZIMUTHAL:
		case ARCHITECTURAL:
    }
#endif
}

DestPanoImage PanoramaOptions::getDestImage() const
{
    vigra::Size2D size(getWidth(), getHeight());
    return DestPanoImage((DestPanoImage::Projection) getProjection(),
                          getHFOV(), size );
}


const std::string PanoramaOptions::fileformatNames[] =
{
    "JPEG",
    "PNG",
    "TIFF",
    "TIFF_m",
    "TIFF_mask",
    "TIFF_multilayer",
    "TIFF_multilayer_mask",
    "PICT",
    "PSD",
    "PSD_m",
    "PSD_mask",
    "PAN",
    "IVR",
    "IVR_java",
    "VRML",
    "QTVR",
    "HDR",
    "HDR_m",
    "EXR",
    "EXR_m"
};


const std::string PanoramaOptions::fileformatExt[] =
{
    "jpg",
    "png",
    "tif",
    "tif",
    "tif",
    "tif",
    "tif",
    "pict",
    "psd",
    "psd",
    "psd",
    "pan",
    "ivr",
    "IVR_java",
    "wrl",
    "mov",
    "hdr",
    "hdr",
    "exr",
    "exr"
};





} //namespace
