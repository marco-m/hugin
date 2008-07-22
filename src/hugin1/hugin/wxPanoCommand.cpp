// -*- c-basic-offset: 4 -*-

/** @file PanorCommand.cpp
 *
 *  @brief implementation of some PanoCommands
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

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"
#include "common/wxPlatform.h"

#include <base_wx/ImageCache.h>
#include <base_wx/platform.h>
#include <hugin/wxPanoCommand.h>

#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>



using namespace std;
using namespace vigra;
using namespace utils;

namespace PT {

void wxAddCtrlPointGridCmd::execute()
{
    PanoCommand::execute();

    // TODO: rewrite, and use same function for modular image creation.
#if 1
    const PanoImage & i1 = pano.getImage(img1);
    const PanoImage & i2 = pano.getImage(img1);

    // run both images through the harris corner detector
    ImageCache::EntryPtr eptr = ImageCache::getInstance().getSmallImage(i1.getFilename());

    vigra::BImage leftImg(eptr->get8BitImage()->size());

    vigra::GreenAccessor<vigra::RGBValue<vigra::UInt8> > ga;
    vigra::copyImage(srcImageRange(*(eptr->get8BitImage()), ga ),
                     destImage(leftImg));

    double scale = i1.getWidth() / (double) leftImg.width();

    //const vigra::BImage & leftImg = ImageCache::getInstance().getPyramidImage(
    //    i1.getFilename(),1);

    BImage leftCorners(leftImg.size());
    FImage leftCornerResponse(leftImg.size());

    // empty corner image
    leftCorners.init(0);

    DEBUG_DEBUG("running corner detector threshold: " << cornerThreshold << "  scale: " << scale );

    // find corner response at scale scale
    vigra::cornerResponseFunction(srcImageRange(leftImg),
        destImage(leftCornerResponse),
        scale);

    //    saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");
    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(leftCornerResponse), destImage(leftCorners), 255);

//    exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("c:/corner_response_maxima.png"));

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(leftCornerResponse), destImage(leftCornerResponse),
        vigra::Threshold<double, double>(
        cornerThreshold, DBL_MAX, 0.0, 1.0));

    vigra::combineTwoImages(srcImageRange(leftCorners), srcImage(leftCornerResponse),
        destImage(leftCorners), std::multiplies<float>());

//    exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("c:/corner_response_threshold.png"));

    // create transform from img1 -> sphere
    PTools::Transform img1ToSphere;
    PTools::Transform sphereToImg2;

    PanoramaOptions opts;
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
    opts.setHFOV(360);
    opts.setWidth(360);
    opts.setVFOV(180);

    img1ToSphere.createInvTransform(pano, img1, opts);
    sphereToImg2.createTransform(pano, img2, opts);


    int border = 5;
    double sphx, sphy;
    double img2x, img2y;
    // need to scale the images.
    // sample grid on img1 and try to add ctrl points
    for (unsigned int x=0; x < (unsigned int)leftImg.width(); x++ ) {
        for (unsigned int y=0; y < (unsigned int)leftImg.height(); y++) {
            if (leftCorners(x,y) > 0) {
                img1ToSphere.transformImgCoord(sphx, sphy, scale*x, scale*y);
                sphereToImg2.transformImgCoord(img2x, img2y, sphx, sphy);
                // check if it is inside..
                if (   img2x > border && img2x < i2.getWidth() - border
                    && img2y > border && img2y < i2.getHeight() - border )
                {
                    // add control point
                    ControlPoint p(img1, scale*x, scale*y, img2, img2x, img2y);
                    pano.addCtrlPoint(p);
                }
            }
        }
    }
#endif
    pano.changeFinished();
}


void wxAddImagesCmd::execute()
{
    PanoCommand::execute();

    // check if the files should be sorted by date
    long sort = wxConfigBase::Get()->Read(wxT("General/SortNewImgOnAdd"), HUGIN_GUI_SORT_NEW_IMG_ON_ADD);

    switch (sort) {
        case 1:
                // sort by filename
            std::sort(files.begin(), files.end());
            break;
        case 2:
                // sort by date
            std::sort(files.begin(), files.end(), FileIsNewer());
            break;
        default:
                // no or unknown sort method
            break;
    }

    std::vector<std::string>::const_iterator it;


    double cropFactor = 0;
    double focalLength = 0;

    Lens lens;
    VariableMap vars;
    fillVariableMap(vars);
    ImageOptions imgopts;
    SrcPanoImage srcImg;

    // load additional images...
    for (it = files.begin(); it != files.end(); ++it) {
        bool added = false;
        const std::string &filename = *it;
        wxString fname(filename.c_str(), HUGIN_CONV_FILENAME);

        int assumeSimilar = wxConfigBase::Get()->Read(wxT("/LensDefaults/AssumeSimilar"), HUGIN_LENS_ASSUME_SIMILAR);
        if (!assumeSimilar) {
            focalLength = 0;
            cropFactor = 0;
        }

        // try to read settings automatically.
        srcImg.setFilename(filename);
        bool ok = srcImg.readEXIF(focalLength, cropFactor, true);
        if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
            wxMessageBox(wxString::Format(_("Could not decode image:\n%s\nAbort"), fname.c_str()), _("Unsupported image file format"));
            return;
        }
//#if 0
        if (! ok && assumeSimilar) {
                 // search for image with matching size and exif data
                 // and re-use it.
                for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
                    SrcPanoImage other = pano.getSrcImage(i);
                    double dummyfl=0;
		    double dummycrop = 0; 
                    other.readEXIF(dummyfl, dummycrop, false);
                    if ( other.getSize() == srcImg.getSize() 
                         &&
                         other.getExifModel() == srcImg.getExifModel() &&
                         other.getExifMake()  == srcImg.getExifMake() &&
                         other.getExifFocalLength() == srcImg.getExifFocalLength()
                       )
                    {
                        double ev = srcImg.getExposureValue();
                        srcImg = pano.getSrcImage(i);
                        srcImg.setFilename(filename);
                        srcImg.setExposureValue(ev);
                        // add image
                        PanoImage img(filename, srcImg.getSize().x, srcImg.getSize().y, other.getLensNr());
                        srcImg.setLensNr(other.getLensNr());
                        int imgNr = pano.addImage(img, vars);
                        pano.setSrcImage(imgNr, srcImg);
                        added=true;
                        break;
                    }
                }
		if (added) continue;
        }
//#endif
        int matchingLensNr=-1;
        // if no similar image found, ask user
        if (! ok) {
            if (!getLensDataFromUser(MainFrame::Get(), srcImg, focalLength, cropFactor)) {
                // assume a standart lens
                srcImg.setHFOV(50);
                srcImg.setExifCropFactor(1);
            }
        }

        // check the image hasn't disappeared on us since the HFOV dialog was
        // opened
        wxString fn(srcImg.getFilename().c_str(),HUGIN_CONV_FILENAME);
        if (!wxFileName::FileExists(fn)) {
            DEBUG_INFO("Image: " << fn.mb_str() << " has disappeared, skipping...");
            continue;
        }

        // FIXME: check if the exif information
        // indicates this image matches a already used lens
        for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
            SrcPanoImage other = pano.getSrcImage(i);
            // force reading of exif data, as it is currently not stored in the
            // Panorama data class
            if (other.readEXIF(focalLength, cropFactor, false)) {
                if (other.getSize() == srcImg.getSize()
                    && other.getExifModel() == srcImg.getExifModel()
                    && other.getExifMake()  == srcImg.getExifMake()
                    && other.getExifFocalLength() == srcImg.getExifFocalLength()
                   )
                {
                    matchingLensNr = pano.getImage(i).getLensNr();
                    // copy data from other image, just keep
                    // the file name and reload the exif data (for exposure)
                    double ev = srcImg.getExposureValue();
                    srcImg = pano.getSrcImage(i);
                    srcImg.setFilename(filename);
                    srcImg.setExposureValue(ev);
                    break;
                }
            } else if (assumeSimilar) {
                // no exiv information, just check image size.
                if (other.getSize() == srcImg.getSize() ) {
                    matchingLensNr = pano.getImage(i).getLensNr();
                    // copy data from other image, just keep
                    // the file name
                    srcImg = pano.getSrcImage(i);
                    srcImg.setFilename(filename);
                    break;
                }
            }
        }

        if (matchingLensNr == -1) {
            // create and add new lens
            Lens lens;
            lens.setImageSize(srcImg.getSize());
            matchingLensNr = pano.addLens(lens);
        }

        PanoImage img(filename, srcImg.getSize().x, srcImg.getSize().y, (unsigned int) matchingLensNr);
        srcImg.setLensNr(matchingLensNr);
        int imgNr = pano.addImage(img, vars);
        pano.setSrcImage(imgNr, srcImg);
        if (imgNr == 0) {
            // get initial value for output exposure
            PanoramaOptions opts = pano.getOptions();
            opts.outputExposureValue = srcImg.getExposureValue();
            pano.setOptions(opts);
        }
    }
    pano.changeFinished();
}


void wxLoadPTProjectCmd::execute()
{
    PanoCommand::execute();
#ifdef _Hgn1_PANOCOMMAND_H
    Panorama& pano = o_pano;
#endif

    PanoramaMemento newPano;
    int ptoVersion = 0;
    std::ifstream in(filename.c_str());
    if (newPano.loadPTScript(in, ptoVersion, prefix)) {
        pano.setMemento(newPano);
        PanoramaOptions opts = pano.getOptions();
        // always reset to TIFF_m ...
        opts.outputFormat = PanoramaOptions::TIFF_m;
        // get enblend and enfuse options from preferences
        if (ptoVersion < 2) {
            // no options stored in file, use default arguments in config file
            opts.enblendOptions = wxConfigBase::Get()->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
            opts.enfuseOptions = wxConfigBase::Get()->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);
        }
        pano.setOptions(opts);

        unsigned int nImg = pano.getNrOfImages();
        wxString basedir;
        double focalLength=0;
        double cropFactor=0;
        bool autopanoSiftFile=false;
        SrcPanoImage autopanoSiftRefImg;
        for (unsigned int i = 0; i < nImg; i++) {
            wxFileName fname(wxString (pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
            while (! fname.FileExists()){
                        // Is file in the new path
                if (basedir != wxT("")) {
                    DEBUG_DEBUG("Old filename: " << pano.getImage(i).getFilename());
                    std::string fn = utils::stripPath(pano.getImage(i).getFilename());
                    DEBUG_DEBUG("Old filename, without path): " << fn);
                    wxString newname(fn.c_str(), HUGIN_CONV_FILENAME);
                            // GetFullName does only work with local paths (not compatible across platforms)
//                            wxString newname = fname.GetFullName();
                    fname.AssignDir(basedir);
                    fname.SetFullName(newname);
                    DEBUG_TRACE("filename with new path: " << fname.GetFullPath().mb_str(wxConvLocal));
                    if (fname.FileExists()) {
                        pano.setImageFilename(i, (const char *)fname.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
                        DEBUG_TRACE("New filename set: " << fname.GetFullPath().mb_str(wxConvLocal));
                                // TODO - set pano dirty flag so that new paths are saved
                        continue;
                    }
                }

                wxMessageBox(wxString::Format(_("Image file not found:\n%s\nPlease select correct image"), fname.GetFullPath().c_str()), _("Image file not found"));

                if (basedir == wxT("")) {
                    basedir = fname.GetPath();
                }

                // open file dialog
                wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                                 basedir, fname.GetFullName(),
                                 HUGIN_WX_FILE_IMG_FILTER, wxOPEN, wxDefaultPosition);
                dlg.SetDirectory(basedir);
                if (dlg.ShowModal() == wxID_OK) {
                    pano.setImageFilename(i, (const char *)dlg.GetPath().mb_str(HUGIN_CONV_FILENAME));
                            // save used path
                    basedir = dlg.GetDirectory();
                    DEBUG_INFO("basedir is: " << basedir.mb_str(wxConvLocal));
                } else {
                    PanoramaMemento emptyPano;
                    pano.setMemento(emptyPano);
                            // set an empty panorama
                    pano.changeFinished();
                    return;
                }
                fname.Assign(dlg.GetPath());
            }
            // check if image size is correct
            SrcPanoImage srcImg = pano.getSrcImage(i);
            //
            vigra::ImageImportInfo imginfo(srcImg.getFilename().c_str());
            if (srcImg.getSize() != imginfo.size()) {
                // adjust size properly.
                srcImg.resize(imginfo.size());
            }
            // check if script contains invalid HFOV
            unsigned lNr = pano.getImage(i).getLensNr();
            Lens cLens = pano.getLens(lNr);
            double hfov = const_map_get(pano.getVariables()[i], "v").getValue();
            if (cLens.getProjection() == Lens::RECTILINEAR
                && hfov >= 180 && autopanoSiftFile == false)
            {
                autopanoSiftFile = true;
                // something is wrong here, try to read from exif data (all images)
                bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
                if (! ok) {
                    getLensDataFromUser(MainFrame::Get(), srcImg, focalLength, cropFactor);
                }
                autopanoSiftRefImg = srcImg;
            } else if (autopanoSiftFile) {
                // need to copy the lens parameters from the first lens.
                srcImg.setHFOV(autopanoSiftRefImg.getHFOV());
            } else {
                // load exif data, but do not apply it
                srcImg.readEXIF(focalLength, cropFactor);
            }
            pano.setSrcImage(i, srcImg);
        }
    } else {
        DEBUG_ERROR("could not load panotools script");
    }
    in.close();
    pano.changeFinished();
}


void wxApplyTemplateCmd::execute()
{
    PanoCommand::execute();
#ifdef _Hgn1_PANOCOMMAND_H
    Panorama& pano = o_pano;
#endif
    
    
    wxConfigBase* config = wxConfigBase::Get();

    if (pano.getNrOfImages() == 0) {
        // TODO: prompt for images!

        // add a dummy lens
        Lens dummyLens;
        pano.addLens(dummyLens);

        wxString path = config->Read(wxT("actualPath"), wxT(""));
        wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                path, wxT(""),
                HUGIN_WX_FILE_IMG_FILTER, wxOPEN|wxMULTIPLE , wxDefaultPosition);
        dlg.SetDirectory(path);

        // remember the image extension
        wxString img_ext;
        if (config->HasEntry(wxT("lastImageType"))){
            img_ext = config->Read(wxT("lastImageType")).c_str();
        }
        if (img_ext == wxT("all images"))
            dlg.SetFilterIndex(0);
        else if (img_ext == wxT("jpg"))
            dlg.SetFilterIndex(1);
        else if (img_ext == wxT("all files"))
            dlg.SetFilterIndex(2);
        DEBUG_INFO ( "Image extention: " << img_ext.mb_str(wxConvLocal) );

        // call the file dialog
        if (dlg.ShowModal() == wxID_OK) {
            // get the selections
            wxArrayString Pathnames;
            wxArrayString Filenames;
            dlg.GetPaths(Pathnames);
            dlg.GetFilenames(Filenames);

            // e safe the current path to config
            config->Write(wxT("actualPath"), dlg.GetDirectory());  // remember for later
            DEBUG_INFO ( wxString::Format(wxT("img_ext: %d"), dlg.GetFilterIndex()).mb_str(wxConvLocal) );
            // save the image extension
            switch ( dlg.GetFilterIndex() ) {
                case 0: config->Write(wxT("lastImageType"), wxT("all images")); break;
                case 1: config->Write(wxT("lastImageType"), wxT("jpg")); break;
                case 2: config->Write(wxT("lastImageType"), wxT("all files")); break;
            }

            // add images.
            for (unsigned int i=0; i< Pathnames.GetCount(); i++) {
                std::string filename = (const char *)Pathnames[i].mb_str(HUGIN_CONV_FILENAME);
                vigra::ImageImportInfo inf(filename.c_str());
                PanoImage img(filename, inf.width(), inf.height(), 0);
                VariableMap vars;
                fillVariableMap(vars);
                (void)pano.addImage(img, vars);
            }

        }
    }

    unsigned int nOldImg = pano.getNrOfImages();
    PanoramaMemento newPanoMem;

    int ptoVersion = 0;
    if (newPanoMem.loadPTScript(in, ptoVersion, "")) {
        Panorama newPano;
        newPano.setMemento(newPanoMem);

        unsigned int nNewImg = newPano.getNrOfImages();
        if (nOldImg != nNewImg) {
            wxString errMsg = wxString::Format(_("Error, template expects %d images,\ncurrent project contains %d images\n"), nNewImg, nOldImg);
            wxMessageBox(errMsg, _("Could not apply template"), wxICON_ERROR);
            pano.changeFinished();
            return;
        }

        // check image sizes, and correct parameters if required.
        for (unsigned int i = 0; i < nNewImg; i++) {

            // check if image size is correct
            SrcPanoImage oldSrcImg = pano.getSrcImage(i);
            SrcPanoImage newSrcImg = newPano.getSrcImage(i);

            // just keep the file name
            DEBUG_DEBUG("apply template fn:" <<  newSrcImg.getFilename() << " real fn: " << oldSrcImg.getFilename());
            newSrcImg.setFilename(oldSrcImg.getFilename());
            if (oldSrcImg.getSize() != newSrcImg.getSize()) {
                // adjust size properly.
                newSrcImg.resize(oldSrcImg.getSize());
            }
            newPano.setSrcImage(i, newSrcImg);
        }
        // keep old control points.
        newPano.setCtrlPoints(pano.getCtrlPoints());
        newPanoMem = newPano.getMemento();
        pano.setMemento(newPanoMem);
    } else {
        wxMessageBox(_("Error loading project file"), _("Could not apply template"), wxICON_ERROR);
    }
    pano.changeFinished();
}



} // namespace

