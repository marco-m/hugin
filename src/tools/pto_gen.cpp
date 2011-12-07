// -*- c-basic-offset: 4 -*-

/** @file pto_gen.cpp
 *
 *  @brief program to generate a pto file from given image files
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include <hugin_version.h>

#include <fstream>
#include <getopt.h>
#ifdef _WINDOWS
#include <io.h>
#endif
#include <panodata/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include "hugin_utils/alphanum.h"
#include <vigra/impex.hxx>

using namespace std;
using namespace HuginBase;

static void usage(const char* name)
{
    cout << name << ": generate project file from images" << endl
         << name << " version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] image1 [...]" << endl
         << endl
         << "  Options:" << endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << endl
         << "     -p, --projection=INT   Projection type (default: 0)" << endl
         << "     -f, --fov=FLOAT        Horizontal field of view of images (default: 50)" << endl
         << "     -s, --stacklength=INT  Number of images in stack" << endl
         << "                            (default: 1, no stacks)" << endl
         << "     -l, --linkstacks       Link image positions in stacks" << endl
         << "     -h, --help             Shows this help" << endl
         << endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:p:f:s:lh";

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"projection", required_argument, NULL, 'p' },
        {"fov", required_argument, NULL, 'f' },
        {"stacklength", required_argument, NULL, 's' },
        {"linkstacks", no_argument, NULL, 'l' },
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    int optionIndex = 0;
    string output;
    int projection=-1;
    float fov=-1;
    int stackLength=1;
    bool linkStacks=false;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case 'p':
                {
                    projection=atoi(optarg);
                    if((projection==0) && (strcmp(optarg,"0")!=0))
                    {
                        cerr << "Could not parse image number.";
                        return 1;
                    };
                    if(projection<0)
                    {
                        cerr << "Invalid projection number." << endl;
                        return 1;
                    };
                };
                break;
            case 'f':
                fov=atof(optarg);
                if(fov<1 || fov>360)
                {
                    cerr << "Invalid field of view";
                    return 1;
                };
                break;
            case 's':
                stackLength=atoi(optarg);
                if(stackLength<1)
                {
                    cerr << "Could not parse stack length." << endl;
                    return 1;
                };
                break;
            case 'l':
                linkStacks=true;
                break;
            case ':':
                cerr <<"Option " << longOptions[optionIndex].name << " requires a number" << endl;
                return 1;
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind < 1)
    {
        usage(argv[0]);
        return 1;
    };

    cout << "Generating pto file..." << endl;
    cout.flush();

    std::vector<string> filelist;
    while(optind<argc)
    {
        string input;
#ifdef _WINDOWS
        //do globbing
        input=GetAbsoluteFilename(argv[optind]);
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        char newFile[_MAX_PATH];

        _splitpath(input.c_str(), drive, dir, NULL, NULL);

        struct _finddata_t finddata;
        intptr_t findhandle = _findfirst(input.c_str(), &finddata);
        if (findhandle != -1)
        {
            do
            {
                _splitpath(finddata.name, NULL, NULL, fname, ext);
                _makepath(newFile, drive, dir, fname, ext);
                //check if valid image file
                if(vigra::isImage(newFile))
                {
                    filelist.push_back(std::string(newFile));
                };
            }
            while (_findnext(findhandle, &finddata) == 0);
            _findclose(findhandle);
        }
#else
        input=argv[optind];
        if(hugin_utils::FileExists(input))
        {
            if(vigra::isImage(input.c_str()))
            {
                filelist.push_back(GetAbsoluteFilename(input));
            };
        };
#endif
        optind++;
    };

    if(filelist.size()==0)
    {
        cerr << "No valid image files given." << endl;
        return 1;
    };

    //sort filenames
    sort(filelist.begin(),filelist.end(),doj::alphanum_less());

    Panorama pano;
    for(size_t i=0; i<filelist.size();i++)
    {
        SrcPanoImage srcImage(filelist[i]);
        cout << "Reading " << filelist[i] << "..." << endl;
        if(projection>=0)
        {
            srcImage.setProjection((HuginBase::BaseSrcPanoImage::Projection)projection);
        };
        if(fov>0)
        {
            srcImage.setHFOV(fov);
            if(!srcImage.hasEXIFread())
            {
                srcImage.setExifCropFactor(1.0);
            };
        }
        else
        {
            //set plausible default value if they could not read from exif
            if(!srcImage.hasEXIFread())
            {
                srcImage.setHFOV(50);
                srcImage.setExifCropFactor(1.0);
            };
        };
        try
        {
            vigra::ImageImportInfo info(filelist[i].c_str());
            std::string pixelType=info.getPixelType();
            if((pixelType=="UINT8") || (pixelType=="UINT16") || (pixelType=="INT16"))
            {
                srcImage.setResponseType(HuginBase::SrcPanoImage::RESPONSE_EMOR);
            }
            else
            {
                srcImage.setResponseType(HuginBase::SrcPanoImage::RESPONSE_LINEAR);
            };
        }
        catch(std::exception & e)
        {
            cerr << "ERROR: caught exception: " << e.what() << std::endl;
            cerr << "Could not get pixel type for file " << filelist[i] << std::endl;
        };

        pano.addImage(srcImage);
    };

    if(pano.getNrOfImages()==0)
    {
        cerr << "Adding images to project files failed." << endl;
        return 1;
    };

    //link lenses
    if(pano.getNrOfImages()>1)
    {
        StandardImageVariableGroups variable_groups(pano);
        ImageVariableGroup& lenses = variable_groups.getLenses();

        for(size_t i=1;i<pano.getNrOfImages();i++)
        {
            int image=-1;
            const SrcPanoImage & srcImg=pano.getImage(i);
            for(size_t j=0;j<i;j++)
            {
                const SrcPanoImage & compareImg=pano.getImage(j);
                if(srcImg.getHFOV()==compareImg.getHFOV() &&
                    srcImg.getProjection()==compareImg.getProjection() &&
                    srcImg.getExifModel()==compareImg.getExifModel() &&
                    srcImg.getExifMake()==compareImg.getExifMake() &&
                    srcImg.getSize()==compareImg.getSize())
                {
                    image=j;
                    break;
                };
            };
            if(image!=-1)
            {
                SrcPanoImage img=pano.getSrcImage(i);
                double ev=img.getExposureValue();
                lenses.switchParts(i,lenses.getPartNumber(image));
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, i);
                img.setExposureValue(ev);
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, i);
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, i);
                img.setWhiteBalanceRed(1);
                img.setWhiteBalanceBlue(1);
                pano.setSrcImage(i, img);
            };
        };
        cout << endl << "Assigned " << lenses.getNumberOfParts() << " lenses." << endl;
    };

    //link stacks
    if(stackLength>1)
    {
        stackLength=std::min<int>(stackLength,pano.getNrOfImages());
        int stackCount=pano.getNrOfImages() / stackLength;
        if(pano.getNrOfImages() % stackLength > 0)
        {
            stackCount++;
        };
        if(stackCount<pano.getNrOfImages())
        {
            for(size_t stackNr=0;stackNr<stackCount;stackNr++)
            {
                size_t firstImgStack=stackNr*stackLength;
                for(size_t i=0;i<stackLength;i++)
                {
                    if(firstImgStack+i<pano.getNrOfImages())
                    {
                        pano.linkImageVariableStack(firstImgStack,firstImgStack+i);
                        if(linkStacks)
                        {
                            pano.linkImageVariableYaw(firstImgStack,firstImgStack+i);
                            pano.linkImageVariablePitch(firstImgStack,firstImgStack+i);
                            pano.linkImageVariableRoll(firstImgStack,firstImgStack+i);
                        };
                    };
                };
            };
            cout << "Assigned " << stackCount << " stacks." << endl;
        };
    };

    //set output exposure value
    PanoramaOptions opt = pano.getOptions();
    opt.outputExposureValue = CalculateMeanExposure::calcMeanExposure(pano);
    pano.setOptions(opt);

    //output
    if(output=="")
    {
        output=hugin_utils::stripExtension(hugin_utils::stripPath(pano.getImage(0).getFilename()));
        if(pano.getNrOfImages()>1)
        {
            output.append("-");
            output.append(hugin_utils::stripExtension(hugin_utils::stripPath(pano.getImage(pano.getNrOfImages()-1).getFilename())));
        };
        output=output.append(".pto");
    };
    output=GetAbsoluteFilename(output);
    //write output
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(output));

    cout << endl << "Written output to " << output << endl;
    return 0;
}
