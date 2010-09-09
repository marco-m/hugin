/*
This file is part of hugin.

hugin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

hugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with hugin.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file PanoramaMakefilelibExport.cpp
 * @brief
 *  Created on: Aug 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "PanoramaMakefilelibExport.h"

#include <makefilelib/char_type.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <locale>
#include <makefilelib/Comment.h>
#include <makefilelib/Variable.h>
#include <makefilelib/VariableDef.h>
#include <makefilelib/VariableRef.h>
#include <makefilelib/MakefileItem.h>
#include <makefilelib/Makefile.h>
#include <makefilelib/AutoVariable.h>
#include <makefilelib/Newline.h>
#include <makefilelib/Rule.h>
#include <makefilelib/Conditional.h>
#include <makefilelib/Manager.h>
#include <makefilelib/Anything.h>

#include <panodata/PanoramaData.h>
#include <hugin_utils/utils.h>
#include <hugin_version.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <algorithms/nona/ComputeImageROI.h>
#ifdef _WINDOWS
#include "windows.h"
#else
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#else
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#endif
#endif
/// Automates an very often occuring sequence
#define  newVarDef(var, name, ...) \
mf::Variable* var = mgr.own(new mf::Variable(name, __VA_ARGS__)); \
var->getDef().add();

namespace HuginBase
{
using namespace makefile;
using namespace std;
using namespace vigra;
namespace mf = makefile;

/// constants
static const makefile::string hdrgrayRemappedExt("_gray.pgm");
static const makefile::string hdrRemappedExt(".exr");
static const makefile::string ldrRemappedExt(".tif");
static const makefile::string ldrRemappedMode("TIFF_m");
static const makefile::string hdrRemappedMode("EXR_m");

// should be moved somewhere else (will be after GSOC anyway)
vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    UIntSet stack;

    CalculateImageOverlap overlap(&pano);
    overlap.calculate(10);  // we are testing 10*10=100 points
    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            if(overlap.getOverlap(srcImg,srcImg2)>0.7)
            {
                stack.insert(srcImg2);
                allImgs.erase(srcImg2);
            }
        }
        result.push_back(stack);
        stack.clear();
    } while (allImgs.size() > 0);

    return result;
}

// should be moved somewhere else (will be after GSOC anyway)
vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    UIntSet stack;

    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        SrcPanoImage simg = pano.getSrcImage(srcImg);
        // FIXME this should be a user preference
        double maxEVDiff = 0.5;
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            SrcPanoImage simg2 = pano.getSrcImage(srcImg2);
            if ( fabs(simg.getExposureValue() - simg2.getExposureValue()) < maxEVDiff )
            {
                stack.insert(srcImg2);
                allImgs.erase(srcImg2);
            }
        }
        result.push_back(stack);
        stack.clear();
    } while (allImgs.size() > 0);

    return result;
}


// should be moved somewhere else (will be after GSOC anyway)
UIntSet getImagesinROI (const PanoramaData& pano, const UIntSet activeImages)
{
    UIntSet images;
    PanoramaOptions opts = pano.getOptions();
    for (UIntSet::const_iterator it = activeImages.begin(); it != activeImages.end(); ++it)
    {
        Rect2D roi = estimateOutputROI(pano, opts, *it);
        if (! (roi.isEmpty())) {
            images.insert(*it);
        }
    }
    return images;
}

bool PanoramaMakefilelibExport::createItems()
{
    // we use this Variable for initializing pointers that get an object only under certain conditions
    mf::Variable* nullvar = mgr.own(new mf::Variable("NOT_DEFINED", "This_variable_has_not_been_defined"));

    mgr.own_add((new Comment(
        "makefile for panorama stitching, created by hugin using the new makefilelib")));

    //----------
    // set temporary dir if defined
    if(!tmpDir.empty())
    {
#ifdef UNIX_LIKE
        mgr.own_add(new Comment("set temporary directory for UNIX_LIKE"));
        mf::Variable* vtmpdir = mgr.own(new mf::Variable("TMPDIR", tmpDir));
        vtmpdir->setExport(true); vtmpdir->getDef().add();
#else
        mgr.own_add(new Comment("set temporary directory for not UNIX_LIKE"));
        mf::Variable* vtmpdir = mgr.own(new mf::Variable("TEMP", tmpDir));
        vtmpdir->setExport(true); vtmpdir->getDef().add();
        mf::Variable* vtmpdir2 = mgr.own(new mf::Variable("TMP", tmpDir));
        vtmpdir2->setExport(true); vtmpdir2->getDef().add();
#endif
    }

#ifdef _WINDOWS
    mgr.own_add(new Comment("Force using cmd.exe"));
    mf::Variable* winshell = mgr.own(new mf::Variable("SHELL", getenv("ComSpec")));
    winshell->getDef().add();
#endif

    //----------
    // set the tool commands
    mgr.own_add(new Comment("Tool configuration"));
    newVarDef(vnona, "NONA", progs.nona);
    newVarDef(vPTStitcher, "PTSTITCHER", progs.PTStitcher);
    newVarDef(vPTmender, "PTMENDER", progs.PTmender);
    newVarDef(vPTblender, "PTBLENDER", progs.PTblender);
    newVarDef(vPTmasker, "PTMASKER", progs.PTmasker);
    newVarDef(vPTroller, "PTROLLER", progs.PTroller);
    newVarDef(venblend, "ENBLEND", progs.enblend);
    newVarDef(venfuse, "ENFUSE", progs.enfuse);
    newVarDef(vsmartblend, "SMARTBLEND", progs.smartblend);
    newVarDef(vhdrmerge, "HDRMERGE", progs.hdrmerge);
#ifdef _WINDOWS
    newVarDef(vrm, "RM", "del", Makefile::NONE);
#else
    newVarDef(vrm, "RM", "rm");
#endif

    //----------
    // if this is defined and we have .app in the exiftool command, we execute it with perl by prepending perl -w
    // to the command name.
#ifdef MAC_SELF_CONTAINED_BUNDLE
    mf::Variable* vexiftool = mgr.own(new mf::Variable("EXIFTOOL",
            progs.exiftool.find(".app") != std::string::npos ?
            "perl -w " + progs.exiftool :
            progs.exiftool));
#else
    mf::Variable* vexiftool = mgr.own(new mf::Variable("EXIFTOOL", progs.exiftool));
#endif
    vexiftool->getDef().add();

    //----------
    // Project parameters
    mgr.own_add(new Comment("Project parameters"));
    PanoramaOptions opts = pano.getOptions();
    mf::Variable* vhugin_projection = mgr.own(new mf::Variable("HUGIN_PROJECTION",
            opts.getProjection()));
    vhugin_projection->getDef().add();

    mf::Variable* vhugin_hfov = mgr.own(new mf::Variable("HUGIN_HFOV", opts.getHFOV()));
    vhugin_hfov->getDef().add();
    mf::Variable* vhugin_width = mgr.own(new mf::Variable("HUGIN_WIDTH", opts.getWidth()));
    vhugin_width->getDef().add();
    mf::Variable* vhugin_height = mgr.own(new mf::Variable("HUGIN_HEIGHT", opts.getHeight()));
    vhugin_height->getDef().add();

    //----------
    // options for the programs
    mgr.own_add(new Comment("options for the programs"));
    // set remapper specific settings
    mf::Variable* vnonaldr = nullvar;
    mf::Variable* vnonaopts = nullvar;
    if(opts.remapper == PanoramaOptions::NONA)
    {
        makefile::string val;
        if (opts.outputImageType == "tif" && opts.outputLayersCompression.size() != 0)
            val = "-z " + opts.outputLayersCompression;
        else if (opts.outputImageType == "jpg")
            val = "-z PACKBITS ";

        vnonaldr = mgr.own(new mf::Variable("NONA_LDR_REMAPPED_COMP", val, Makefile::NONE));
        vnonaldr->getDef().add();

        vnonaopts = mgr.own(new mf::Variable("NONA_OPTS",
                opts.remapUsingGPU ? "-g " : "", Makefile::NONE));
        vnonaopts->getDef().add();
    }

    // set blender specific settings
    mf::Variable* venblendopts = nullvar;
    mf::Variable* venblendldrcomp = nullvar;
    mf::Variable* venblendhdrcomp = nullvar;

    if(opts.blendMode == PanoramaOptions::ENBLEND_BLEND)
    {
        {
            valuestream.str("");    // clear the streams buffer
            valuestream << opts.enblendOptions;
            if (opts.getHFOV() == 360.0)
                // blend over the border
                valuestream << " -w";

            vigra::Rect2D roi = opts.getROI();
            if (roi.top() != 0 || roi.left() != 0 )
                valuestream << " -f" << roi.width() << "x" << roi.height() << "+" << roi.left() << "+" << roi.top();
            else
                valuestream << " -f" << roi.width() << "x" << roi.height();
            venblendopts = mgr.own(new mf::Variable("ENBLEND_OPTS", valuestream.str(), Makefile::NONE));
            venblendopts->getDef().add();
        }

        {
            valuestream.str("");
            if (opts.outputImageType == "tif" && opts.outputImageTypeCompression.size() != 0)
                valuestream << "--compression " << opts.outputImageTypeCompression;
            else if (opts.outputImageType == "jpg")
                valuestream << "--compression " << opts.quality;

            venblendldrcomp = mgr.own(new mf::Variable("ENBLEND_LDR_COMP", valuestream.str(), Makefile::NONE));
            venblendldrcomp->getDef().add();
        }

        {
            makefile::string val;
            if (opts.outputImageTypeHDR == "tif" && opts.outputImageTypeHDRCompression.size() != 0) {
                val += "--compression " + opts.outputImageTypeHDRCompression;
            }
            venblendhdrcomp = mgr.own(new mf::Variable("ENBLEND_HDR_COMP", val, Makefile::NONE));
            venblendhdrcomp->getDef().add();
        }
    }

    mf::Variable* vptblenderopts = nullvar;
    if(opts.blendMode == PanoramaOptions::PTBLENDER_BLEND)
    {
        valuestream.str("");
        switch (opts.colorCorrection)
        {
            case PanoramaOptions::NONE:
                break;
            case PanoramaOptions::BRIGHTNESS_COLOR:
                valuestream << " -k " << opts.colorReferenceImage;
                break;
            case PanoramaOptions::BRIGHTNESS:
                valuestream << " -k " << opts.colorReferenceImage;
                break;
            case PanoramaOptions::COLOR:
                valuestream << " -k " << opts.colorReferenceImage;
                break;
        }
        vptblenderopts = mgr.own(new mf::Variable("PTBLENDER_OPTS", valuestream.str(), Makefile::NONE));
        vptblenderopts->getDef().add();
    }

    //----------
    mf::Variable* vsmartblendopts = nullvar;
    if(opts.blendMode == PanoramaOptions::SMARTBLEND_BLEND)
    {
        vsmartblendopts = mgr.own(new mf::Variable(
            "SMARTBLEND_OPTS",
            opts.getHFOV() == 360.0 ? " -w" : ""));
        vsmartblendopts->getDef().add();
        // TODO: build smartblend command line from given images. (requires additional program)
    }

    //----------
    mf::Variable* vhdrmergeopts = nullvar;
    if(opts.hdrMergeMode == PanoramaOptions::HDRMERGE_AVERAGE)
    {
        vhdrmergeopts = mgr.own(new mf::Variable(
                "HDRMERGE_OPTS", opts.hdrmergeOptions, Makefile::NONE));
        vhdrmergeopts->getDef().add();
    }

    //----------
    newVarDef(venfuseopts,
            "ENFUSE_OPTS", opts.enfuseOptions +
            (opts.getHFOV() == 360.0 ? " -w" : ""), Makefile::NONE);

    //----------
    newVarDef(vexiftoolcopyargs,
            "EXIFTOOL_COPY_ARGS", progs.exiftool_opts, Makefile::NONE);


    //----------
    // Panorama output
    mgr.own_add(new Comment("the output panorama"));

    newVarDef(vldrremappedprefix,
        "LDR_REMAPPED_PREFIX", outputPrefix, Makefile::MAKE);
    newVarDef(vldrremappedprefixshell,
        "LDR_REMAPPED_PREFIX_SHELL", vldrremappedprefix->getValue(), Makefile::SHELL);

    newVarDef(vhdrstackremappedprefix,
        "HDR_STACK_REMAPPED_PREFIX", outputPrefix + "_hdr_", Makefile::MAKE);
    newVarDef(vhdrstackremappedprefixshell,
        "HDR_STACK_REMAPPED_PREFIX_SHELL", vhdrstackremappedprefix->getValue(), Makefile::SHELL);

    newVarDef(vldrexposureremappedprefix,
        "LDR_EXPOSURE_REMAPPED_PREFIX", outputPrefix + "_exposure_layers_", Makefile::MAKE);
    newVarDef(vldrexposureremappedprefixshell,
        "LDR_EXPOSURE_REMAPPED_PREFIX_SHELL", vldrexposureremappedprefix->getValue(), Makefile::SHELL);

    newVarDef(vprojectfile, "PROJECT_FILE", ptofile, Makefile::MAKE);
    newVarDef(vprojectfileshell, "PROJECT_FILE_SHELL", ptofile, Makefile::SHELL);

    newVarDef(vldrblended, "LDR_BLENDED", outputPrefix + "." + opts.outputImageType, Makefile::MAKE);
    newVarDef(vldrblendedshell, "LDR_BLENDED_SHELL", vldrblended->getValue(), Makefile::SHELL);

    newVarDef(vldrstackedblended, "LDR_STACKED_BLENDED", outputPrefix + "_fused." + opts.outputImageType, Makefile::MAKE);
    newVarDef(vldrstackedblendedshell, "LDR_STACKED_BLENDED_SHELL", vldrstackedblended->getValue(), Makefile::SHELL);

    newVarDef(vldrexposurelayersfused,
        "LDR_EXPOSURE_LAYERS_FUSED", outputPrefix + "_blended_fused." + opts.outputImageType, Makefile::MAKE);
    newVarDef(vldrexposurelayersfusedshell,
        "LDR_EXPOSURE_LAYERS_FUSED_SHELL", vldrexposurelayersfused->getValue(), Makefile::SHELL);

    newVarDef(vhdrblended, "HDR_BLENDED", outputPrefix + "_hdr." + opts.outputImageTypeHDR, Makefile::MAKE);
    newVarDef(vhdrblendedshell, "HDR_BLENDED_SHELL", vhdrblended->getValue(), Makefile::SHELL);

    //----------
    // Input Image filenames
    mgr.own_add(new Comment ("first input image"));
    newVarDef(vinimage1, "INPUT_IMAGE_1", pano.getImage(0).getFilename(), Makefile::MAKE);
    newVarDef(vinimage1shell, "INPUT_IMAGE_1_SHELL", pano.getImage(0).getFilename(), Makefile::SHELL);

    mgr.own_add(new Comment("all input images"));
    // Assemble them all into one string
    std::vector<std::string> inimages;

    for (unsigned int i=0; i < pano.getNrOfImages(); i++)
    {
        inimages.push_back(pano.getImage(i).getFilename());
    }
    newVarDef(vinimages, "INPUT_IMAGES", inimages.begin(), inimages.end(), Makefile::MAKE, "\\\n");
    newVarDef(vinimagesshell, "INPUT_IMAGES_SHELL", inimages.begin(), inimages.end(), Makefile::SHELL, "\\\n");

    //----------
    std::vector<std::string> remappedImages;
    std::vector<std::string> remappedHDRImages;
    std::vector<std::string> remappedHDRgrayImages;

    for (UIntSet::const_iterator it = images.begin(); it != images.end(); it++)
    {
        std::ostringstream fn1, fn2, fn3;
        fn1 << outputPrefix << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
        fn2 << outputPrefix << "_hdr_" << std::setfill('0') << std::setw(4) << *it << hdrRemappedExt;
        fn3 << outputPrefix << "_hdr_" << std::setfill('0') << std::setw(4) << *it << hdrgrayRemappedExt;
        remappedImages.push_back(fn1.str());
        remappedHDRImages.push_back(fn2.str());
        remappedHDRgrayImages.push_back(fn3.str());
    }
    mgr.own_add(new Comment("remapped images"));
    newVarDef(vldrlayers, "LDR_LAYERS", remappedImages.begin(), remappedImages.end(), Makefile::MAKE, "\\\n");
    newVarDef(vldrlayersshell, "LDR_LAYERS_SHELL", remappedImages.begin(), remappedImages.end(), Makefile::SHELL, "\\\n");

    mgr.own_add(new Comment("remapped images (hdr)"));
    newVarDef(vhdrlayers, "HDR_LAYERS", remappedHDRImages.begin(), remappedHDRImages.end(), Makefile::MAKE, "\\\n");
    newVarDef(vhdrlayersshell, "HDR_LAYERS_SHELL", remappedHDRImages.begin(), remappedHDRImages.end(), Makefile::SHELL, "\\\n");

    mgr.own_add(new Comment("remapped maxval images"));
    newVarDef(vhdrgraylayers, "HDR_LAYERS_WEIGHTS", remappedHDRgrayImages.begin(), remappedHDRgrayImages.end(), Makefile::MAKE, "\\\n");
    newVarDef(vhdrgraylayersshell, "HDR_LAYERS_WEIGHTS_SHELL", remappedHDRgrayImages.begin(), remappedHDRgrayImages.end(), Makefile::SHELL, "\\\n");

    //----------
    // hdr, exposure, ldr stacks
    std::vector<mf::Variable*> hdr_stacks, hdr_stacks_shell, hdr_stacks_input, hdr_stacks_input_shell;
    mgr.own_add(new Comment("stacked hdr images"));
    std::vector<UIntSet> stacks = getHDRStacks(pano, images);
    mf::Variable* vhdrstacks,* vhdrstacksshell;
    std::vector<std::string> stackedhdrImgs;
    createstacks(stacks, "HDR_STACK", "_stack_hdr_", "_hdr_", hdrRemappedExt,
            hdr_stacks, hdr_stacks_shell, hdr_stacks_input, hdr_stacks_input_shell, vhdrstacks, vhdrstacksshell, stackedhdrImgs);

    std::vector<mf::Variable*> ldrexp_stacks, ldrexp_stacks_shell, ldrexp_stacks_input, ldrexp_stacks_input_shell, ldrexp_stacks_pt_input, ldrexp_stacks_input_pt_shell;
    mgr.own_add(new Comment("number of image sets with similar exposure"));
    mf::Variable* vldrexposurelayers,* vldrexposurelayersshell,* vldrexposurelayersremapped,* vldrexposurelayersremappedshell;
    std::vector<UIntSet> exposures = getExposureLayers(pano, images);
    std::vector<std::string> exposureimgs;
    createexposure(exposures, "LDR_EXPOSURE_LAYER", "_exposure_", "_exposure_layers_", ldrRemappedExt,
                ldrexp_stacks, ldrexp_stacks_shell, ldrexp_stacks_input, ldrexp_stacks_input_shell, ldrexp_stacks_pt_input, ldrexp_stacks_input_pt_shell,
                vldrexposurelayers, vldrexposurelayersshell, vldrexposurelayersremapped, vldrexposurelayersremappedshell,
                exposureimgs);

    std::vector<mf::Variable*> ldr_stacks, ldr_stacks_shell, ldr_stacks_input, ldr_stacks_input_shell;
    mgr.own_add(new Comment("stacked ldr images"));
    mf::Variable* vldrstacks,* vldrstacksshell;
    std::vector<std::string> stackedldrimgs;
    createstacks(stacks, "LDR_STACK", "_stack_ldr_", "_exposure_layers_", ldrRemappedExt,
            ldr_stacks, ldr_stacks_shell, ldr_stacks_input, ldr_stacks_input_shell, vldrstacks, vldrstacksshell, stackedldrimgs);


    //----------
    if(!includePath.empty())
    {
        mgr.own_add(new Anything("include " + Makefile::quote(includePath, Makefile::MAKE)));
        return true;
    }

    //----------
    // Collect prerequisites

    std::vector<mf::Variable*> allprereqs, cleanprereqs;

    if(opts.outputLDRBlended)
    {
        allprereqs.push_back(vldrblended);
        append(outputFiles, vldrblended->getValues());
        newVarDef(vdoldrblend, "DO_LDR_BLENDED", 1);
        if(!opts.outputLDRLayers)
        {
            cleanprereqs.push_back(vldrlayersshell);
            append(outputFiles, vldrlayers->getValues());
        }
    }

    if(opts.outputLDRLayers)
    {
        allprereqs.push_back(vldrlayers);
        append(outputFiles, vldrlayers->getValues());
    }

    if(opts.outputLDRExposureRemapped)
    {
        allprereqs.push_back(vldrexposurelayersremapped);
        append(outputFiles, vldrexposurelayersremapped->getValues());
    }

    if(opts.outputLDRExposureLayers)
    {
        allprereqs.push_back(vldrexposurelayers);
        append(outputFiles, exposureimgs);
        if(!opts.outputLDRExposureRemapped)
        {
            cleanprereqs.push_back(vldrexposurelayersremappedshell);
            append(outputFiles, vldrexposurelayersremapped->getValues());
        }
    }

    if(opts.outputLDRExposureBlended)
    {
        allprereqs.push_back(vldrstackedblended);
        append(outputFiles, vldrstackedblended->getValues());
        newVarDef(vdoldrstackedblended, "DO_LDR_STACKED_BLENDED", 1);
        cleanprereqs.push_back(vldrstacksshell);
        append(outputFiles, stackedldrimgs);
        if(!opts.outputLDRExposureRemapped && !opts.outputLDRExposureLayers)
        {
            cleanprereqs.push_back(vldrexposurelayersremappedshell);
            append(outputFiles, vldrexposurelayersremapped->getValues());
        }
    }

    if(opts.outputLDRExposureLayersFused)
    {
        allprereqs.push_back(vldrexposurelayersfused);
        append(outputFiles, vldrexposurelayersfused->getValues());
        newVarDef(vdoldrexposurelayersfused, "DO_LDR_EXPOSURE_LAYERS_FUSED", 1);
        append(outputFiles, exposureimgs);
        if(!opts.outputLDRExposureRemapped && !opts.outputLDRExposureLayers)
        {
            cleanprereqs.push_back(vldrexposurelayersremappedshell);
            append(outputFiles, vldrexposurelayersremapped->getValues());
            cleanprereqs.push_back(vldrexposurelayersshell);
        }
    }

    if(opts.outputHDRLayers)
    {
        allprereqs.push_back(vhdrlayers);
        append(outputFiles, vhdrlayers->getValues());
        append(outputFiles, vhdrgraylayers->getValues());
    }

    if(opts.outputHDRStacks)
    {
        allprereqs.push_back(vhdrstacks);
        append(outputFiles, stackedhdrImgs);
        if(!opts.outputHDRLayers)
        {
            cleanprereqs.push_back(vhdrlayersshell);
            cleanprereqs.push_back(vhdrgraylayersshell);
            append(outputFiles, vhdrlayers->getValues());
            append(outputFiles, vhdrgraylayers->getValues());
        }
    }

    if(opts.outputHDRBlended)
    {
        allprereqs.push_back(vhdrblended);
        append(outputFiles, vhdrblended->getValues());
        newVarDef(vdohdrblended, "DO_HDR_BLENDED", 1);
        if(!opts.outputHDRStacks)
        {
            cleanprereqs.push_back(vhdrstacksshell);
            append(outputFiles, stackedhdrImgs);
            if(!opts.outputHDRLayers)
            {
                cleanprereqs.push_back(vhdrlayersshell);
                cleanprereqs.push_back(vhdrgraylayersshell);
                append(outputFiles, vhdrlayers->getValues());
                append(outputFiles, vhdrgraylayers->getValues());
            }
        }
    }

    //----------
    // Assemble all and clean rules

    Rule* all = mgr.own(new Rule());
    all->addTarget("all");
    for(std::vector<mf::Variable*>::const_iterator it = allprereqs.begin(); it != allprereqs.end(); it++)
    {
        all->addPrereq((*it));
    }
    all->add();

    Rule* clean = mgr.own(new Rule());
    clean->addTarget("clean");
    {
    std::string vdefs;
        for(std::vector<mf::Variable*>::const_iterator it = cleanprereqs.begin(); it != cleanprereqs.end(); it++)
        {
            vdefs += (*it)->getRef() + " ";
        }
    clean->addCommand("-" + vrm->getRef() + ' ' + vdefs);
    }
    clean->add();

    //----------
    // Test rules check if programs exist.
    Rule* test = mgr.own(new Rule());
    test->addTarget("test");
    // test remapper
    switch(opts.remapper) {
        case PanoramaOptions::NONA:
            createcheckProgCmd(*test,"nona","@-$(NONA) --help");
            break;
        case PanoramaOptions::PTMENDER:
            break;
    }
    // test blender
    switch(opts.blendMode) {
        case PanoramaOptions::ENBLEND_BLEND:
            createcheckProgCmd(*test,"enblend","@-$(ENBLEND) -h");
            break;
        case PanoramaOptions::PTBLENDER_BLEND:
            createcheckProgCmd(*test,"PTblender","@-$(PTBLENDER) -h");
            break;
        case PanoramaOptions::SMARTBLEND_BLEND:
            createcheckProgCmd(*test,"smartblend","@-$(SMARTBLEND)");
            break;
    }
    // test enfuse
    createcheckProgCmd(*test,"enfuse","@-$(ENFUSE) -h");
    // test hugin_hdrmerge
    createcheckProgCmd(*test,"hugin_hdrmerge","@-$(HDRMERGE) -h");
    // test exiftool
    createcheckProgCmd(*test,"exiftool","@-$(EXIFTOOL) -ver");
    test->add();

    //----------
    // Info rule to get some infomation about project
    Rule* info = mgr.own(new Rule());
    info->addTarget("info");
    echoInfo(*info,"===========================================================================");
    echoInfo(*info,"***************  Panorama makefile generated by Hugin       ***************");
    echoInfo(*info,"===========================================================================");
    echoInfo(*info,"System information");
    echoInfo(*info,"===========================================================================");
    std::ostringstream infostream;
    infostream.imbue(makefile::GetMakefileLocale());
    infostream << fixed;
    printSystemInfo(*info);
    infostream.str("");
    infostream << DISPLAY_VERSION;
    echoInfo(*info,"===========================================================================");
    echoInfo(*info,"Output options");
    echoInfo(*info,"===========================================================================");
    echoInfo(*info,"Hugin Version: "+infostream.str());
    echoInfo(*info,"Project file: "+ptofile);
    echoInfo(*info,"Output prefix: "+outputPrefix);
    pano_projection_features proj;
	if (panoProjectionFeaturesQuery(opts.getProjection(), &proj)) 
    {
        infostream.str("");
        infostream << proj.name << " (" << opts.getProjection() << ")";
        echoInfo(*info,"Projection: "+infostream.str());
    }
    std::vector<double> parameters=opts.getProjectionParameters();
    if(parameters.size()>0)
    {
        infostream.str("");
        unsigned int i=0;
        infostream << setprecision(2) << parameters[i];
        i++;
        while(i<parameters.size())
        {
            infostream << ", " << parameters[i];
            i++;
        };
        echoInfo(*info,"Projection parameters: "+infostream.str());
    };
    infostream.str("");
    infostream << setprecision(0) << opts.getHFOV() << " x " << setprecision(0) << opts.getVFOV();
    echoInfo(*info,"Field of view: "+infostream.str());
    infostream.str("");
    infostream << opts.getWidth() << " x " << opts.getHeight();
    echoInfo(*info,"Canvas dimensions: "+infostream.str());
    infostream.str("");
    infostream << "(" << opts.getROI().left() << "," << opts.getROI().top() << ") - (" << opts.getROI().right() << "," << opts.getROI().bottom() << ")";
    echoInfo(*info,"Crop area: "+infostream.str());
    infostream.str("");
    infostream << setprecision(2) << opts.outputExposureValue;
    echoInfo(*info,"Output exposure value: "+infostream.str());
    echoInfo(*info,"Selected outputs");
    if(opts.outputLDRBlended || opts.outputLDRLayers)
    {
        echoInfo(*info,"Normal panorama");
        if(opts.outputLDRBlended)
            echoInfo(*info,"* Blended panorama");
        if(opts.outputLDRLayers)
            echoInfo(*info,"* Remapped images");
    };
    if(opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused || opts.outputLDRExposureLayers || opts.outputLDRExposureRemapped)
    {
        echoInfo(*info,"Exposure fusion");
        if(opts.outputLDRExposureBlended)
            echoInfo(*info,"* Fused and blended panorama");
        if(opts.outputLDRExposureLayersFused)
            echoInfo(*info,"* Blended and fused panorama");
        if(opts.outputLDRExposureLayers)
            echoInfo(*info,"* Blended exposure layers");
        if(opts.outputLDRExposureRemapped)
            echoInfo(*info,"* Remapped images");
    };
    if(opts.outputHDRBlended || opts.outputHDRLayers || opts.outputHDRStacks)
    {
        echoInfo(*info,"HDR merging");
        if(opts.outputHDRBlended)
            echoInfo(*info,"* Merged and blended panorama");
        if(opts.outputHDRStacks)
            echoInfo(*info,"* Remapped merged stacks");
        if(opts.outputHDRLayers)
            echoInfo(*info,"* Remapped images");
    };
    if(opts.remapUsingGPU)
        echoInfo(*info,"Using GPU for remapping");

    echoInfo(*info,"===========================================================================");
    echoInfo(*info,"Input images");
    echoInfo(*info,"===========================================================================");
    infostream.str("");
    infostream << pano.getNrOfImages();
    echoInfo(*info,"Number of images in project file: "+infostream.str());
    infostream.str("");
    infostream << images.size();
    echoInfo(*info,"Number of active images: "+infostream.str());
    for(UIntSet::const_iterator it=images.begin();it!=images.end();it++)
    {
        infostream.str("");
        const HuginBase::SrcPanoImage &img=pano.getImage(*it);
        infostream << "Image " << *it << ": " << img.getFilename();
        echoInfo(*info,infostream.str());
        infostream.str("");
        infostream << "Image " << *it << ": Size " << img.getSize().width() << "x" << img.getSize().height() << ", Exposure: " << setprecision(2) << img.getExposureValue();
        echoInfo(*info,infostream.str());
    };
    info->add();

    //----------
    // Rules for every single file
    if(opts.remapper == PanoramaOptions::NONA)
    {
        mgr.own_add(new Comment("Rules for ordinary TIFF_m and hdr output"));
        UIntSet::const_iterator it = images.begin();
        size_t i=0;
        for(; it != images.end(); it++, i++)
        {
            std::string source = Makefile::quote(pano.getImage(*it).getFilename(), Makefile::MAKE);
            std::ostringstream imgnr;
            imgnr << *it;

            // ldr part
            Rule* ruleldr = mgr.own(new Rule()); ruleldr->add();
            ruleldr->addTarget(Makefile::quote(remappedImages[i], Makefile::MAKE));
            ruleldr->addPrereq(source);
            ruleldr->addPrereq(vprojectfile);
            ruleldr->addCommand(
                    vnona->getRef() +" "+ vnonaopts->getRef() +" "+ vnonaldr->getRef()
                    + " -r ldr -m " + ldrRemappedMode + " -o " + vldrremappedprefixshell->getRef() +
                    " -i " + imgnr.str() +" "+ vprojectfileshell->getRef());

            // hdr part
            Rule* rulehdr = mgr.own(new Rule()); rulehdr->add();
            rulehdr->addTarget(Makefile::quote(remappedHDRImages[i], Makefile::MAKE));
            rulehdr->addPrereq(source);
            rulehdr->addPrereq(vprojectfile);
            rulehdr->addCommand(
                    vnona->getRef() +" "+ vnonaopts->getRef() +" -r hdr -m " + hdrRemappedMode + " -o " +
                    vhdrstackremappedprefixshell->getRef() + " -i " + imgnr.str() +" "+ vprojectfileshell->getRef());
        }

        mgr.own_add(new Comment("Rules for exposure layer output"));

        size_t j=0;
        for(i=0; i < exposures.size(); i++)
        {
            for(UIntSet::const_iterator it = exposures[i].begin(); it != exposures[i].end(); it++, j++)
            {
                std::ostringstream expvalue, imgnr;
                imgnr << *it;
                expvalue.imbue(makefile::GetMakefileLocale());
                expvalue <<  pano.getSrcImage(*it).getExposureValue();
                Rule* rule = mgr.own(new Rule()); rule->add();
                rule->addTarget(Makefile::quote(vldrexposurelayersremapped->getValues()[j], Makefile::MAKE));
                rule->addPrereq(Makefile::quote(pano.getImage(*it).getFilename(), Makefile::MAKE));
                rule->addPrereq(vprojectfile);
                rule->addCommand(
                        vnona->getRef() +" "+ vnonaopts->getRef() +" "+ vnonaldr->getRef()
                        + " -r ldr -e " + expvalue.str() + " -m " + ldrRemappedMode + " -o " + vldrexposureremappedprefixshell->getRef() +
                        " -i " + imgnr.str() +" "+ vprojectfileshell->getRef());

            }
        }
    }

    if(opts.remapper == PanoramaOptions::PTMENDER)
    {
        Rule* rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrlayers);
        rule->addPrereq(vinimages);
        rule->addPrereq(vprojectfile);
        rule->addCommand(vPTmender->getRef() + " -o " + vldrremappedprefixshell->getRef() +" "+
                vprojectfileshell->getRef());
    }

    //----------
    // Rules for LDR and HDR stack merging, a rule for each stack
    mgr.own_add(new Comment("Rules for LDR and HDR stack merging, a rule for each stack"));
    for(size_t i=0; i< stacks.size(); i++)
    {
        std::ostringstream imgnr;
        imgnr << i;

        Rule* ruleldr = mgr.own(new Rule()); ruleldr->add();
        ruleldr->addTarget(ldr_stacks[i]);
        ruleldr->addPrereq(ldr_stacks_input[i]);
        ruleldr->addCommand(venfuse->getRef() +" "+ venfuseopts->getRef() + " -o " +
                ldr_stacks_shell[i]->getRef() +" "+ ldr_stacks_input_shell[i]->getRef());
        ruleldr->addCommand("-" + vexiftool->getRef() + " -overwrite_original_in_place -TagsFromFile " +
                vinimage1shell->getRef() +" "+ vexiftoolcopyargs->getRef() +" "+ ldr_stacks_shell[i]->getRef());

        Rule* rulehdr = mgr.own(new Rule()); rulehdr->add();
        rulehdr->addTarget(hdr_stacks[i]);
        rulehdr->addPrereq(hdr_stacks_input[i]);
        rulehdr->addCommand(vhdrmerge->getRef() +" "+ vhdrmergeopts->getRef() + " -o " +
                hdr_stacks_shell[i]->getRef() +" "+ hdr_stacks_input_shell[i]->getRef());
    }
    //----------
    // Blend modes
    // these commands are often occuring
    const std::string enblendcmd = venblend->getRef() +" "+ venblendldrcomp->getRef() +" "+
            venblendopts->getRef() + " -o ";
    const std::string exifcmd = "-" + vexiftool->getRef() + " -overwrite_original_in_place -TagsFromFile " +
            vinimage1shell->getRef() +" "+ vexiftoolcopyargs->getRef() +' ';
    const std::string ptrollercmd = vPTroller->getRef() + " -o ";

    if(opts.blendMode == PanoramaOptions::ENBLEND_BLEND)
    {
        Rule* rule = mgr.own(new Rule()); rule->add();

        // write rules for blending with enblend
        rule->addTarget(vldrblended);
        rule->addPrereq(vldrlayers);
        rule->addCommand(enblendcmd + vldrblendedshell->getRef() +" "+ vldrlayersshell->getRef());
        rule->addCommand(exifcmd + vldrblendedshell->getRef());

        // for LDR exposure blend planes
        for(unsigned i=0; i < exposures.size(); i++)
        {
            rule = mgr.own(new Rule()); rule->add();
            rule->addTarget(ldrexp_stacks[i]);
            rule->addPrereq(ldrexp_stacks_input[i]);
            rule->addCommand(enblendcmd + ldrexp_stacks_shell[i]->getRef() +" "+ ldrexp_stacks_input_shell[i]->getRef());
            rule->addCommand(exifcmd + ldrexp_stacks_shell[i]->getRef());
        }

        // rules for enfuse blending
        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrstackedblended);
        rule->addPrereq(vldrstacks);
        rule->addCommand(enblendcmd + vldrstackedblendedshell->getRef() +" "+ vldrstacksshell->getRef());
        rule->addCommand(exifcmd + vldrstackedblendedshell->getRef());

        // rules for fusing blended layers
        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrexposurelayersfused);
        rule->addPrereq(vldrexposurelayers);
        rule->addCommand(venfuse->getRef() +" "+ venblendldrcomp->getRef() +" "+ venfuseopts->getRef() + " -o " +
                vldrexposurelayersfusedshell->getRef() +" "+ vldrexposurelayersshell->getRef());
        rule->addCommand(exifcmd + vldrexposurelayersfusedshell->getRef());

        // rules for hdr blending
        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vhdrblended);
        rule->addPrereq(vhdrstacks);
        rule->addCommand(venblend->getRef() +" "+ venblendhdrcomp->getRef() +" "+ venblendopts->getRef() + " -o " +
                vhdrblendedshell->getRef() +" "+ vhdrstacksshell->getRef());

        // rules for multilayer output
        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrremappedprefix->getRef() + "_multilayer.tif");
        rule->addPrereq(vldrlayers);
        rule->addCommand("tiffcp " + vldrlayersshell->getRef() +" "+ vldrremappedprefixshell->getRef() + "_multilayer.tif");

        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrremappedprefix->getRef() + "_fused_multilayer.tif");
        rule->addPrereq(vldrstacks);
        rule->addPrereq(vldrexposurelayers);
        rule->addCommand("tiffcp " + vldrstacksshell->getRef() +" "+ vldrexposurelayersshell->getRef() +" "+ vldrremappedprefixshell->getRef() + "_fused_multilayer.tif");

        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrremappedprefix->getRef() + "_multilayer.psd");
        rule->addPrereq(vldrlayers);
        rule->addCommand("PTtiff2psd -o " + vldrremappedprefixshell->getRef() + "_multilayer.psd " + vldrlayersshell->getRef());

        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrremappedprefix->getRef() + "_fused_multilayer.psd");
        rule->addPrereq(vldrstacks);
        rule->addPrereq(vldrexposurelayers);
        rule->addCommand("PTtiff2psd -o " + vldrremappedprefixshell->getRef() + "_fused_multilayer.psd " + vldrstacksshell->getRef() +
                vldrexposurelayersshell->getRef());
    }


    if(opts.blendMode == PanoramaOptions::NO_BLEND)
    {
        Rule* rule = mgr.own(new Rule()); rule->add();

        // write rules for blending with enblend
        rule->addTarget(vldrblended);
        rule->addPrereq(vldrlayers);
        rule->addCommand("-" + vrm->getRef() +" "+ vldrblendedshell->getRef());
        rule->addCommand(ptrollercmd + vldrblendedshell->getRef() +" "+ vldrlayersshell->getRef());
        rule->addCommand(exifcmd + vldrblendedshell->getRef());

        // for LDR exposure blend planes
        for(unsigned i=0; i < exposures.size(); i++)
        {
            rule = mgr.own(new Rule()); rule->add();
            rule->addTarget(ldrexp_stacks[i]);
            rule->addPrereq(ldrexp_stacks_input[i]);
            rule->addCommand("-" + vrm->getRef() +" "+ ldrexp_stacks_shell[i]->getRef());
            rule->addCommand(ptrollercmd + ldrexp_stacks_shell[i]->getRef());
            rule->addCommand(exifcmd + ldrexp_stacks_shell[i]->getRef());
        }

        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vldrstackedblended);
        rule->addPrereq(vldrstacks);
        rule->addCommand("-" + vrm->getRef() +" "+ vldrstackedblendedshell->getRef());
        rule->addCommand(ptrollercmd + vldrstackedblendedshell->getRef() +" "+ vldrstacksshell->getRef());
        rule->addCommand(exifcmd + vldrstackedblendedshell->getRef());

        // rules for non-blended HDR panoramas
        rule = mgr.own(new Rule()); rule->add();
        rule->addTarget(vhdrblended);
        rule->addPrereq(vhdrlayers);
        rule->addCommand(vhdrmerge->getRef() +" "+ vhdrmergeopts->getRef() + " -o " +
                vhdrblendedshell->getRef() +" "+ vhdrlayersshell->getRef());
    }
    return true;
}


void PanoramaMakefilelibExport::createstacks(const std::vector<UIntSet> stackdata,
        const std::string stkname,
        const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
        std::vector<mf::Variable*>& stacks,
        std::vector<mf::Variable*>& stacks_shell,
        std::vector<mf::Variable*>& stacks_input,
        std::vector<makefile::Variable*>& stacks_input_shell,
        mf::Variable*& vstacks,
        mf::Variable*& vstacksshell,
        std::vector<std::string>& allfiles)
{
    std::ostringstream stknrs;
    for (unsigned i=0; i < stackdata.size(); i++)
    {
        stknrs << i << " ";
        std::ostringstream filename, stackname;
        filename << outputPrefix << filenamecenter << std::setfill('0') << std::setw(4) << i << filenameext;
        stackname << stkname << "_" << i;
        allfiles.push_back(filename.str());

        std::vector<std::string> inputs;
        for (UIntSet::const_iterator it = stackdata[i].begin(); it != stackdata[i].end(); it++)
        {
            std::ostringstream fns;
            fns << outputPrefix << inputfilenamecenter << std::setfill('0') << std::setw(4) << *it << filenameext;
            inputs.push_back(fns.str());
        }
        mf::Variable* v;
        v = mgr.own(new mf::Variable(stackname.str(), filename.str(), Makefile::MAKE));
        stacks.push_back(v);
        v->getDef().add();

        v = mgr.own(new mf::Variable(stackname.str() + "_SHELL", filename.str(), Makefile::SHELL));
        stacks_shell.push_back(v);
        v->getDef().add();

        v= mgr.own(new mf::Variable(stackname.str() + "_INPUT", inputs.begin(), inputs.end(), Makefile::MAKE, "\\\n"));
        stacks_input.push_back(v);
        v->getDef().add();

        v = mgr.own(new mf::Variable(stackname.str() + "_INPUT_SHELL", inputs.begin(), inputs.end(), Makefile::SHELL, "\\\n"));
        stacks_input_shell.push_back(v);
        v->getDef().add();
    }
    newVarDef(vhdrstacksnr, stkname + "S_NUMBERS", stknrs.str(), Makefile::NONE);

    std::string stackrefs;
    std::string stackrefsshell;
    std::vector<mf::Variable*>::const_iterator it, it2;
    it = stacks.begin();
    it2 = stacks_shell.begin();
    for(; it != stacks.end() && it2 != stacks_shell.end(); it++, it2++)
    {
        stackrefs += (*it)->getRef() + " ";
        stackrefsshell += (*it2)->getRef() + " ";
    }
    vstacks = mgr.own(new mf::Variable(stkname + "S", stackrefs, Makefile::NONE));
    vstacks->getDef().add();
    vstacksshell = mgr.own(new mf::Variable(stkname + "S_SHELL", stackrefsshell, Makefile::NONE));
    vstacksshell->getDef().add();

}
void PanoramaMakefilelibExport::createexposure(const std::vector<UIntSet> stackdata,
        const std::string stkname,
        const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
        std::vector<mf::Variable*>& stacks,
        std::vector<mf::Variable*>& stacks_shell,
        std::vector<mf::Variable*>& stacks_input,
        std::vector<mf::Variable*>& stacks_input_shell,
        std::vector<mf::Variable*>& stacks_input_pt,
        std::vector<mf::Variable*>& stacks_input_pt_shell,
        mf::Variable*& vstacks,
        mf::Variable*& vstacksshell,
        mf::Variable*& vstacksrem,
        mf::Variable*& vstacksremshell,
        std::vector<std::string>& alllayers)
{
    std::vector<std::string> allimgs;
    std::ostringstream stknrs;
    for (unsigned i=0; i < stackdata.size(); i++)
    {
        stknrs << i << " ";
        std::ostringstream filename, stackname;
        filename << outputPrefix << filenamecenter << std::setfill('0') << std::setw(4) << i << filenameext;
        stackname << stkname << "_" << i;
        alllayers.push_back(filename.str());

        std::vector<std::string> inputs, inputspt;
        double exposure = 0;
        for (UIntSet::const_iterator it = stackdata[i].begin(); it != stackdata[i].end(); it++)
        {
            std::ostringstream fns, fnpt;
            fns << outputPrefix << inputfilenamecenter << std::setfill('0') << std::setw(4) << *it << filenameext;
            fnpt << outputPrefix << std::setfill('0') << std::setw(4) << *it << filenameext;
            inputs.push_back(fns.str());
            inputspt.push_back(fnpt.str());

            exposure += pano.getSrcImage(*it).getExposureValue();
            allimgs.push_back(fns.str());
        }
        mf::Variable* v;
        v = mgr.own(new mf::Variable(stackname.str(), filename.str(), Makefile::MAKE));
        stacks.push_back(v);
        v->getDef().add();

        v = mgr.own(new mf::Variable(stackname.str() + "_SHELL", filename.str(), Makefile::SHELL));
        stacks_shell.push_back(v);
        v->getDef().add();

        v= mgr.own(new mf::Variable(stackname.str() + "_INPUT", inputs.begin(), inputs.end(), Makefile::MAKE, "\\\n"));
        stacks_input.push_back(v);
        v->getDef().add();

        v = mgr.own(new mf::Variable(stackname.str() + "_INPUT_SHELL", inputs.begin(), inputs.end(), Makefile::SHELL, "\\\n"));
        stacks_input_shell.push_back(v);
        v->getDef().add();

        v= mgr.own(new mf::Variable(stackname.str() + "_INPUT_PTMENDER", inputspt.begin(), inputspt.end(), Makefile::MAKE, "\\\n"));
        stacks_input_pt.push_back(v);
        v->getDef().add();

        v = mgr.own(new mf::Variable(stackname.str() + "_INPUT_PTMENDER_SHELL", inputspt.begin(), inputspt.end(), Makefile::SHELL, "\\\n"));
        stacks_input_pt_shell.push_back(v);
        v->getDef().add();

        v = mgr.own(new mf::Variable(stackname.str() + "_EXPOSURE", exposure / stackdata[i].size()));
        v->getDef().add();
    }
    newVarDef(vhdrstacksnr, stkname + "S_NUMBERS", stknrs.str(), Makefile::NONE);

    std::string stackrefs;
    std::string stackrefsshell;
    std::vector<mf::Variable*>::const_iterator it, it2;
    it = stacks.begin();
    it2 = stacks_shell.begin();
    for(; it != stacks.end() && it2 != stacks_shell.end(); it++, it2++)
    {
        stackrefs += (*it)->getRef() + " ";
        stackrefsshell += (*it2)->getRef() + " ";
    }
    vstacks = mgr.own(new mf::Variable(stkname + "S", stackrefs, Makefile::NONE));
    vstacks->getDef().add();
    vstacksshell = mgr.own(new mf::Variable(stkname + "S_SHELL", stackrefsshell, Makefile::NONE));
    vstacksshell->getDef().add();
    vstacksrem = mgr.own( new mf::Variable(stkname + "S_REMAPPED", allimgs.begin(), allimgs.end(), Makefile::MAKE, "\\\n"));
    vstacksrem->getDef().add();
    vstacksremshell = mgr.own( new mf::Variable(stkname + "S_REMAPPED_SHELL", allimgs.begin(), allimgs.end(), Makefile::SHELL, "\\\n"));
    vstacksremshell->getDef().add();
}

void PanoramaMakefilelibExport::createcheckProgCmd(Rule& testrule, const std::string& progName, const std::string& progCommand)
{
    std::string command;
#ifdef _WINDOWS
    testrule.addCommand("@echo Checking " + progName + "...");
    testrule.addCommand(progCommand + " > NUL 2>&1 && echo " + progName + " is ok || echo " +
            progName + " failed");
#else
    testrule.addCommand("@echo -n 'Checking " + progName + "...'");
    testrule.addCommand(progCommand + " > /dev/null 2>&1 && echo '[OK]' || echo '[FAILED]'");
#endif
}

void PanoramaMakefilelibExport::echoInfo(Rule& inforule, const std::string& info)
{
#ifdef _WINDOWS
    inforule.addCommand("@echo " + info);
#else
    inforule.addCommand("@echo '" + info + "'");
#endif
}

void PanoramaMakefilelibExport::printSystemInfo(Rule& inforule)
{
    std::ostringstream infostream("");
    infostream.imbue(makefile::GetMakefileLocale());
#ifdef _WINDOWS
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if(!GetVersionEx((OSVERSIONINFO *) &osvi))
        return;
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    infostream << "Windows ";

    if(osvi.dwPlatformId==VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion > 4)
    {
        if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
            infostream << "2000 ";
        if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==1)
            infostream << "XP ";
        if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2 && osvi.wProductType==VER_NT_WORKSTATION && siSysInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
            infostream << "XP Professional x64 Edition ";
        if(osvi.dwMajorVersion==6)
        {
            if(osvi.dwMinorVersion==0)
            {
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    infostream << "Vista ";
                else
                    infostream << "Server 2008 ";
            }
            if(osvi.dwMinorVersion==1)
            {
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    infostream << "7 ";
                else
                    infostream << "Server 2008 R2 ";
            }
        };
    }

    infostream << "(" << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << " " << osvi.szCSDVersion << ")";
    echoInfo(inforule,"Operating System: "+infostream.str());
    switch(siSysInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            infostream.str("x86");
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            infostream.str("AMD64");
            break;
        default:
            infostream.str("unknown");
            break;
    };
    echoInfo(inforule,"Architecture: "+infostream.str());
    infostream.str("");
    infostream << siSysInfo.dwNumberOfProcessors;
    echoInfo(inforule,"Number of logical processors: "+infostream.str());
    infostream.str("");
    infostream << statex.ullTotalPhys/1024 << " kiB (" << setprecision(2) << statex.dwMemoryLoad << "%% occupied)";
    echoInfo(inforule,"Physical memory: "+infostream.str()); 
    unsigned __int64 freeBytes;
    if(GetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)&freeBytes,NULL,NULL))
    {
        infostream.str("");
        infostream << freeBytes/(1024*1024) << " MiB";
        echoInfo(inforule,"Free space on disc: " + infostream.str());
    };
#else
#ifdef __APPLE__
    infostream.str("");
    SInt32 majorVersion,minorVersion;
    Gestalt(gestaltSystemVersionMajor, &majorVersion);
    Gestalt(gestaltSystemVersionMinor, &minorVersion);
    infostream << majorVersion << "." << minorVersion;
    echoInfo(inforule,"Operating System: MacOS "+infostream.str());
#else
    inforule.addCommand("@echo -n 'Operating system: '");
    inforule.addCommand("@-uname -o");
    inforule.addCommand("@echo -n 'Release: '");
    inforule.addCommand("@-uname -r");
    inforule.addCommand("@echo -n 'Kernel version: '");
    inforule.addCommand("@-uname -v");
    inforule.addCommand("@echo -n 'Machine: '");
    inforule.addCommand("@-uname -m");
    echoInfo(inforule,"Disc usage");
    inforule.addCommand("@-df -h");
    echoInfo(inforule,"Memory usage");
    inforule.addCommand("@-free -m");
#endif
#endif
};

void printstacks(const std::vector<UIntSet>& stackdata)
{
    std::cout << "printstacks: \n";
    for(std::vector<UIntSet>::const_iterator itv = stackdata.begin(); itv != stackdata.end(); itv++)
    {
        for(UIntSet::const_iterator itu = itv->begin(); itu != itv->end(); itu++)
        {
            std::cout << *itu << " ";
        }
        std::cout << "\n";
    }
}

template<typename T>
void append(std::vector<T>& dst, const std::vector<T>& src)
{
    dst.insert(dst.end(), src.begin(), src.end());
}
template<typename T>
void append(std::vector<T>& vec, const T& element)
{
    vec.push_back(element);
}
}
