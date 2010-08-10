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

#include <panodata/PanoramaData.h>
#include <hugin_utils/utils.h>

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

/// Automates an very often occuring sequence
#define  newVarDef(var, name, ...) \
mf::Variable* var = mgr.own(new mf::Variable(name, __VA_ARGS__)); \
var->getDef().add();

namespace HuginBase
{
using namespace makefile;
namespace mf = makefile;

/// constants
static const string hdrgrayRemappedExt = "_gray.pgm";
static const string hdrRemappedExt = ".exr";
const string ldrRemappedExt(".tif");

std::vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs);
std::vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs);

bool PanoramaMakefilelibExport::create()
{
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
	mgr.own_add(new Comment("Force using cmd.exe");
	mf::Variable* winshell = mgr_own(new mf::Variable("SHELL", getenv("ComSpec")));
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
	newVarDef(vrm, "RM", "del");
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
    mf::Variable* vnonaldr = NULL;
    mf::Variable* vnonaopts = NULL;
	if(opts.remapper == PanoramaOptions::NONA)
	{
		string val;
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
	mf::Variable* venblendopts = NULL;
	mf::Variable* venblendldrcomp = NULL;
	mf::Variable* venblendhdrcomp = NULL;

	if(opts.blendMode == PanoramaOptions::ENBLEND_BLEND)
	{
		{
			valuestream.str(opts.enblendOptions);
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
			string val;
			if (opts.outputImageTypeHDR == "tif" && opts.outputImageTypeHDRCompression.size() != 0) {
				val += "--compression " + opts.outputImageTypeHDRCompression;
			}
			venblendhdrcomp = mgr.own(new mf::Variable("ENBLEND_HDR_COMP", val, Makefile::NONE));
			venblendhdrcomp->getDef().add();
		}
	}

	mf::Variable* vptblenderopts = NULL;
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
	mf::Variable* vsmartblendopts = NULL;
	if(opts.blendMode == PanoramaOptions::SMARTBLEND_BLEND)
	{
		vsmartblendopts = mgr.own(new mf::Variable(
			"SMARTBLEND_OPTS",
			opts.getHFOV() == 360.0 ? " -w" : ""));
		vsmartblendopts->getDef().add();
		// TODO: build smartblend command line from given images. (requires additional program)
	}

	//----------
	mf::Variable* vhdrmergeopts = NULL;
	if(opts.hdrMergeMode == PanoramaOptions::HDRMERGE_AVERAGE)
	{
		vhdrmergeopts = mgr.own(new mf::Variable(
				"HDRMERGE_OPTS", opts.hdrmergeOptions, Makefile::NONE));
		vhdrmergeopts->getDef().add();
	}

	//----------
	newVarDef(venfuseopts,
			"ENFUSE_OPTS",
			opts.getHFOV() == 360.0 ? " -w" : "", Makefile::NONE);

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

	for (UIntSet::iterator it = images.begin(); it != images.end(); it++)
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
	createstacks(stacks, "HDR_STACK", "_stack_hdr_", "_hdr_", hdrRemappedExt,
			hdr_stacks, hdr_stacks_shell, hdr_stacks_input, hdr_stacks_input_shell);

	std::vector<mf::Variable*> ldrexp_stacks, ldrexp_stacks_shell, ldrexp_stacks_input, ldrexp_stacks_input_shell;
	mgr.own_add(new Comment("number of image sets with similar exposure"));
	createexposure(getExposureLayers(pano, images), "LDR_EXPOSURE_LAYER", "_exposure_", "_exposure_layers_", ldrRemappedExt,
				ldrexp_stacks, ldrexp_stacks_shell, ldrexp_stacks_input, ldrexp_stacks_input_shell);

	std::vector<mf::Variable*> ldr_stacks, ldr_stacks_shell, ldr_stacks_input, ldr_stacks_input_shell;
	mgr.own_add(new Comment("stacked ldr images"));
	createstacks(stacks, "LDR_STACK", "_stack_ldr_", "_exposure_layers_", ldrRemappedExt,
			ldr_stacks, ldr_stacks_shell, ldr_stacks_input, ldr_stacks_input_shell);



	Makefile::getSingleton().writeMakefile(makefile);
	Makefile::getSingleton().clean();
	return true;
}


void PanoramaMakefilelibExport::createstacks(const std::vector<UIntSet> stackdata,
		const std::string stkname,
		const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
		std::vector<mf::Variable*>& stacks,
		std::vector<mf::Variable*>& stacks_shell,
		std::vector<mf::Variable*>& stacks_input,
		std::vector<makefile::Variable*>& stacks_input_shell)
{
	std::ostringstream stknrs;
	for (unsigned i=0; i < stackdata.size(); i++)
	{
		stknrs << i << " ";
		std::ostringstream filename, stackname;
		filename << outputPrefix << filenamecenter << std::setfill('0') << std::setw(4) << i << filenameext;
		stackname << stkname << "_" << i;

		std::vector<std::string> inputs;
		for (UIntSet::iterator it = stackdata[i].begin(); it != stackdata[i].end(); it++)
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
	std::vector<mf::Variable*>::iterator it, it2;
	it = stacks.begin();
	it2 = stacks_shell.begin();
	for(; it != stacks.end() && it2 != stacks_shell.end(); it++, it2++)
	{
		stackrefs += (*it)->getRef().toString() + " ";
		stackrefsshell += (*it2)->getRef().toString() + " ";
	}
	newVarDef(vstacks, stkname + "S", stackrefs, Makefile::NONE);
	newVarDef(vstacksshell, stkname + "S_SHELL", stackrefsshell, Makefile::NONE);

}
void PanoramaMakefilelibExport::createexposure(const std::vector<UIntSet> stackdata,
		const std::string stkname,
		const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
		std::vector<mf::Variable*>& stacks,
		std::vector<mf::Variable*>& stacks_shell,
		std::vector<mf::Variable*>& stacks_input,
		std::vector<makefile::Variable*>& stacks_input_shell)
{
	std::vector<std::string> allimgs;
	std::ostringstream stknrs;
	for (unsigned i=0; i < stackdata.size(); i++)
	{
		stknrs << i << " ";
		std::ostringstream filename, stackname;
		filename << outputPrefix << filenamecenter << std::setfill('0') << std::setw(4) << i << filenameext;
		stackname << stkname << "_" << i;

		std::vector<std::string> inputs, inputspt;
		double exposure = 0;
		for (UIntSet::iterator it = stackdata[i].begin(); it != stackdata[i].end(); it++)
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
		stacks_input.push_back(v);
		v->getDef().add();

		v = mgr.own(new mf::Variable(stackname.str() + "_INPUT_PTMENDER_SHELL", inputspt.begin(), inputspt.end(), Makefile::SHELL, "\\\n"));
		stacks_input_shell.push_back(v);
		v->getDef().add();

		v = mgr.own(new mf::Variable(stackname.str() + "_EXPOSURE", exposure / stackdata[i].size()));
		v->getDef().add();
	}
	newVarDef(vhdrstacksnr, stkname + "S_NUMBERS", stknrs.str(), Makefile::NONE);

	std::string stackrefs;
	std::string stackrefsshell;
	std::vector<mf::Variable*>::iterator it, it2;
	it = stacks.begin();
	it2 = stacks_shell.begin();
	for(; it != stacks.end() && it2 != stacks_shell.end(); it++, it2++)
	{
		stackrefs += (*it)->getRef().toString() + " ";
		stackrefsshell += (*it2)->getRef().toString() + " ";
	}
	newVarDef(vstacks, stkname + "S", stackrefs, Makefile::NONE);
	newVarDef(vstacksshell, stkname + "S_SHELL", stackrefsshell, Makefile::NONE);
	newVarDef(vstackrem, stkname + "S_REMAPPED", allimgs.begin(), allimgs.end(), Makefile::MAKE, "\\\n");
	newVarDef(vstackremshell, stkname + "S_REMAPPED_SHELL", allimgs.begin(), allimgs.end(), Makefile::SHELL, "\\\n");
}
}
