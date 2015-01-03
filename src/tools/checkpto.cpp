// -*- c-basic-offset: 4 -*-

/** @file checkpto.cpp
 *
 *  @brief helper program for assistant makefile
 *
 *
 *  @author Thomas Modes
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

#include <hugin_config.h>

#include <fstream>
#include <sstream>
#include <getopt.h>

#include <panodata/Panorama.h>
#include "algorithms/optimizer/ImageGraph.h"
#include "hugin_base/panotools/PanoToolsUtils.h"
#include "hugin_base/panodata/StandardImageVariableGroups.h"
#include "algorithms/basic/CalculateCPStatistics.h"
#include "algorithms/basic/LayerStacks.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char* name)
{
    cout << name << ": report the number of image groups in a project" << endl
         << name << " version " << hugin_utils::GetHuginVersion() << endl
         << endl
         << "Usage:  " << name << " input.pto" << endl
         << endl
         << name << " examines the connections between images in a project and" << endl
         << "reports back the number of parts or image groups in that project" << endl
         << endl
         << "Further switches:" << endl
         << "  --print-output-info     Print more information about the output" << endl
         << "  --print-lens-info       Print more information about lenses" << endl
         << "  --print-stack-info      Print more information about assigned stacks" << endl
         << "  --generate-argfile=file Generate Exiftool argfile" << endl
#ifdef EXIFTOOL_GPANO_SUPPORT
         << "  --no-gpano              Don't include GPano tags in argfile" << endl
#endif
         << "  --append-argfile=file   Append the argfile to the generated argfile" << endl
         << "                          spaceholders will be replaced with real values" << endl
         << endl
         << name << " is used by the assistant and by the stitching makefiles" << endl
         << endl;
}

void printImageGroup(const std::vector<HuginBase::UIntSet>& imageGroup, const std::string& prefix=std::string())
{
    for (size_t i=0; i < imageGroup.size(); i++)
    {
        if (!prefix.empty())
        {
            std::cout << prefix << " " << i << ": ";
        };
        std::cout << "[";
        size_t c=0;
        for (HuginBase::UIntSet::const_iterator it = imageGroup[i].begin(); it != imageGroup[i].end(); ++it)
        {
            std::cout << (*it);
            if (c+1 != imageGroup[i].size())
            {
                std::cout << ", ";
            }
            ++c;
        }
        std::cout << "]";
        if (prefix.empty() && (i+1 != imageGroup.size()))
        {
            std::cout << ", ";
        }
        std::cout << std::endl;
    }
};

std::string GetStringFromValue(const double value, const int precision)
{
    std::stringstream valueStream;
    valueStream.imbue(std::locale("C"));
    valueStream << fixed;
    valueStream << setprecision(precision) << value;
    return valueStream.str();
};

void GenerateArgfile(const std::string& filename, const Panorama& pano, bool noGPano, const std::string& appendedArgfile)
{
    pano_projection_features proj;
    const PanoramaOptions &opts=pano.getOptions();
    bool readProjectionName=panoProjectionFeaturesQuery(opts.getProjection(), &proj)!=0;

    std::ofstream infostream(filename.c_str(), std::ofstream::out);
    infostream.imbue(std::locale("C"));
    infostream << fixed;
    std::map<std::string, std::string> placeholders;
#ifdef _WIN32
    std::string linebreak("&\\#xd;&\\#xa;");
#else
    std::string linebreak("&\\#xa;");
#endif
    infostream << "-Software=Hugin " << hugin_utils::GetHuginVersion() << std::endl;
    infostream << "-E" << std::endl;
    infostream << "-UserComment<${UserComment}" << linebreak;
    if(readProjectionName)
    {
        placeholders.insert(std::make_pair("%projection", std::string(proj.name)));
        placeholders.insert(std::make_pair("%projectionNumber", GetStringFromValue(opts.getProjection(), 0)));
        infostream << "Projection: " << proj.name << " (" << opts.getProjection() << ")" << linebreak;
    };
    // fill in some placeholders
    placeholders.insert(std::make_pair("%hfov", GetStringFromValue(opts.getHFOV(), 0)));
    placeholders.insert(std::make_pair("%vfov", GetStringFromValue(opts.getVFOV(), 0)));
    placeholders.insert(std::make_pair("%ev", GetStringFromValue(opts.outputExposureValue, 2)));
    placeholders.insert(std::make_pair("%nrImages", GetStringFromValue(pano.getActiveImages().size(), 0)));
    placeholders.insert(std::make_pair("%nrAllImages", GetStringFromValue(pano.getNrOfImages(), 0)));
    placeholders.insert(std::make_pair("%fullwidth", GetStringFromValue(opts.getWidth(), 0)));
    placeholders.insert(std::make_pair("%fullheight", GetStringFromValue(opts.getHeight(), 0)));
    placeholders.insert(std::make_pair("%width", GetStringFromValue(opts.getROI().width(), 0)));
    placeholders.insert(std::make_pair("%height", GetStringFromValue(opts.getROI().height(), 0)));
    infostream << "FOV: " << placeholders["%hfov"] << " x " << placeholders["%vfov"] << linebreak;
    infostream << "Ev: " << placeholders["%ev"] << std::endl;
    infostream << "-f" << std::endl;
    if(!noGPano)
    {
        //GPano tags are only indented for equirectangular images
        if(opts.getProjection()==PanoramaOptions::EQUIRECTANGULAR)
        {
            vigra::Rect2D roi=opts.getROI();
            int left=roi.left();
            int top=roi.top();
            int width=roi.width();
            int height=roi.height();

            int fullWidth=opts.getWidth();
            if(opts.getHFOV()<360)
            {
                fullWidth=static_cast<int>(opts.getWidth() * 360.0 / opts.getHFOV());
                left += (fullWidth - opts.getWidth())/2;
            };
            int fullHeight=opts.getHeight();
            if(opts.getVFOV()<180)
            {
                fullHeight=static_cast<int>(opts.getHeight() * 180.0 / opts.getVFOV());
                top += (fullHeight - opts.getHeight())/2;
            };
            infostream << "-UsePanoramaViewer=True" << std::endl;
            infostream << "-StitchingSoftware=Hugin" << std::endl;
            infostream << "-ProjectionType=equirectangular"  << std::endl;
            infostream << "-CroppedAreaLeftPixels=" << left << std::endl;
            infostream << "-CroppedAreaTopPixels=" << top  << std::endl;
            infostream << "-CroppedAreaImageWidthPixels="  << width << std::endl;
            infostream << "-CroppedAreaImageHeightPixels=" << height << std::endl;
            infostream << "-FullPanoWidthPixels=" << fullWidth << std::endl;
            infostream << "-FullPanoHeightPixels=" << fullHeight << std::endl;;
            infostream << "-SourcePhotosCount=" << pano.getNrOfImages()  << std::endl;
        };
    };
    if (!appendedArgfile.empty())
    {
        std::ifstream inputArgfile(appendedArgfile.c_str());
        if (inputArgfile.is_open())
        {
            std::ostringstream contents;
            contents << inputArgfile.rdbuf();
            inputArgfile.close();
            std::string s(contents.str());
            // replace all placeholders
            for (std::map<std::string, std::string>::const_iterator it = placeholders.begin(); it != placeholders.end(); ++it)
            {
                boost::algorithm::replace_all(s, it->first, it->second);
            }
            // now append to existing argfile
            infostream << s << std::endl;
        }
        else
        {
            cerr << "ERROR: Could not open file \"" << appendedArgfile << "\"." << endl
                << "       Ignoring content of file." << endl;
        };
    };
    infostream.close();
};

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "h";
    enum
    {
        PRINT_OUTPUT_INFO=1000,
        GENERATE_ARGFILE=1001,
#if EXIFTOOL_GPANO_SUPPORT
        GENERATE_ARGFILE_WITHOUT_GPANO=1002,
#endif
        APPEND_ARGFILE=1003,
        PRINT_LENS_INFO=1004,
        PRINT_STACK_INFO=1005,
    };
    static struct option longOptions[] =
    {
        {"print-output-info", no_argument, NULL, PRINT_OUTPUT_INFO },
        { "print-lens-info", no_argument, NULL, PRINT_LENS_INFO },
        { "print-stack-info", no_argument, NULL, PRINT_STACK_INFO },
        { "generate-argfile", required_argument, NULL, GENERATE_ARGFILE },
#ifdef EXIFTOOL_GPANO_SUPPORT
        {"no-gpano", no_argument, NULL, GENERATE_ARGFILE_WITHOUT_GPANO},
#endif
        {"append-argfile", required_argument, NULL, APPEND_ARGFILE},
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    bool printOutputInfo=false;
    bool printLensInfo = false;
    bool printStackInfo = false;
#ifdef EXIFTOOL_GPANO_SUPPORT
    bool withoutGPano=false;
#else
    bool withoutGPano=true;
#endif
    std::string argfile;
    std::string appendedArgfile;
    int optionIndex = 0;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case PRINT_OUTPUT_INFO:
                printOutputInfo=true;
                break;
            case PRINT_LENS_INFO:
                printLensInfo = true;
                break;
            case PRINT_STACK_INFO:
                printStackInfo = true;
                break;
            case GENERATE_ARGFILE:
                argfile=optarg;
                break;
#ifdef EXIFTOOL_GPANO_SUPPORT
            case GENERATE_ARGFILE_WITHOUT_GPANO:
                withoutGPano=true;
                break;
#endif
            case APPEND_ARGFILE:
                appendedArgfile = optarg;
                if (!hugin_utils::FileExists(appendedArgfile))
                {
                    std::cerr << "WARNING: File \"" << appendedArgfile << "\" not found." << endl
                        << "         Ignoring parameter." << endl;
                    appendedArgfile = "";
                };
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind != 1)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return -1;
    };

    string input=argv[optind];

    Panorama pano;
    ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        cerr << "could not open script : " << input << endl;
        return -1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL)
    {
        cerr << "error while parsing panos tool script: " << input << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return -1;
    }

    if(!argfile.empty())
    {
        GenerateArgfile(argfile, pano, withoutGPano, appendedArgfile);
        return 0;
    };

    HuginBase::ConstStandardImageVariableGroups variable_groups(pano);
    std::cout << endl
              << "Opened project " << input << endl << endl
              << "Project contains" << endl
              << pano.getNrOfImages() << " images" << endl
              << variable_groups.getLenses().getNumberOfParts() << " lenses" << endl
              << variable_groups.getStacks().getNumberOfParts() << " stacks" << endl
              << pano.getNrOfCtrlPoints() << " control points" << endl << endl;
    //cp statistics
    if(pano.getNrOfCtrlPoints()>0)
    {
        double min;
        double max;
        double mean;
        double var;
        HuginBase::PTools::calcCtrlPointErrors(pano);
        CalculateCPStatisticsError::calcCtrlPntsErrorStats(pano, min, max, mean, var);
        if(max>0)
        {
            std::cout << "Control points statistics" << std::endl
                      << fixed << std::setprecision(2)
                      << "\tMean error        : " << mean << std::endl
                      << "\tStandard deviation: " << sqrt(var) << std::endl
                      << "\tMinimum           : " << min << std::endl
                      << "\tMaximum           : " << max << std::endl;
        };
    };
    CPGraph graph;
    createCPGraph(pano, graph);
    CPComponents comps;
    int n = findCPComponents(graph, comps);
    int returnValue=0;
    if(n==1)
    {
        std::cout << "All images are connected." << endl;
        // return value must be 0, otherwise the assistant does not continue
        returnValue=0;
    }
    else
    {
        std::cout << "Found unconnected images!" << endl
                  << "There are " << n << " image groups." << endl;

        std::cout << "Image groups: " << endl;
        for (unsigned i=0; i < comps.size(); i++)
        {
            std::cout << "[";
            CPComponents::value_type::const_iterator it;
            size_t c=0;
            for (it = comps[i].begin(); it != comps[i].end(); ++it)
            {
                std::cout << (*it);
                if (c+1 != comps[i].size())
                {
                    std::cout << ", ";
                }
                c++;
            }
            std::cout << "]";
            if (i+1 != comps.size())
            {
                std::cout << ", " << endl;
            }
        }
        returnValue=n;
    };
    std::cout << std::endl;
    if (printLensInfo)
    {
        std::cout << std::endl << "Lenses:" << std::endl;
        printImageGroup(variable_groups.getLenses().getPartsSet(), "Lens");
    };
    if (printStackInfo)
    {
        std::cout << std::endl << "Stacks:" << std::endl;
        printImageGroup(variable_groups.getStacks().getPartsSet(), "Stack");
    };
    if (printOutputInfo)
    {
        HuginBase::UIntSet outputImages=HuginBase::getImagesinROI(pano, pano.getActiveImages());
        std::vector<HuginBase::UIntSet> stacks=HuginBase::getHDRStacks(pano, outputImages, pano.getOptions());
        cout << endl << "Output contains" << endl
             << stacks.size() << " images stacks:" << endl;
        printImageGroup(stacks);
        std::vector<HuginBase::UIntSet> layers=HuginBase::getExposureLayers(pano, outputImages, pano.getOptions());
        cout << endl << endl << "and " << layers.size() << " exposure layers:" << std::endl;
        printImageGroup(layers);
    };
    return returnValue;
}
