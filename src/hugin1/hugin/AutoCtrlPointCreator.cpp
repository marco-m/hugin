// -*- c-basic-offset: 4 -*-

/** @file AutoCtrlPointCreator.cpp
 *
 *  @brief implementation of AutoCtrlPointCreator Class
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

#include <fstream>
#ifdef __GNUC__
#include <ext/stdio_filebuf.h>
#endif

#include "PT/Panorama.h"

#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/CommandHistory.h"

// Onur
#include "panodata/PanoramaData.h"
#include "algorithms/control_points/FeatureMatchingLinearSearch.h" // algorithms/control_points/
#include "algorithms/control_points/FeatureMatchingSingleKdTree.h" // algorithms/control_points/
#include "algorithms/control_points/FeatureMatchingMultiKdTree.h" // algorithms/control_points/

#include "time.h"
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif

#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "base_wx/huginConfig.h"
#include "common/wxPlatform.h"
#include <wx/utils.h>

using namespace std;
using namespace PT;
using namespace utils;

CPVector AutoCtrlPointCreator::readUpdatedControlPoints(const std::string & file,
                                                    PT::Panorama & pano)
{
    ifstream stream(file.c_str());
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open autopano output: " << file);
        return CPVector();
    }

    Panorama tmpp;
    PanoramaMemento newPano;
    int ptoVersion = 0;
    newPano.loadPTScript(stream, ptoVersion, "");
    tmpp.setMemento(newPano);

    // create mapping between the panorama images.
    map<unsigned int, unsigned int> imgMapping;
    for (unsigned int ni = 0; ni < tmpp.getNrOfImages(); ni++) {
        std::string nname = stripPath(tmpp.getImage(ni).getFilename());
        for (unsigned int oi=0; oi < pano.getNrOfImages(); oi++) {
            std::string oname = stripPath(pano.getImage(oi).getFilename());
            if (nname == oname) {
                // insert image
                imgMapping[ni] = oi;
                break;
            }
        }
        if (! set_contains(imgMapping, ni)) {
            DEBUG_ERROR("Could not find image " << ni << ", name: " << tmpp.getImage(ni).getFilename() << " in autopano output");
            return CPVector();
        }
    }


    // get control points
    CPVector ctrlPoints = tmpp.getCtrlPoints();
    // make sure they are in correct order
    for (CPVector::iterator it= ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        (*it).image1Nr = imgMapping[(*it).image1Nr];
        (*it).image2Nr = imgMapping[(*it).image2Nr];
    }

    return ctrlPoints;
}


CPVector AutoCtrlPointCreator::automatch(Panorama & pano,
                                         const UIntSet & imgs,
                                         int nFeatures)
{
    CPVector cps;
    int t = wxConfigBase::Get()->Read(wxT("/AutoPano/Type"),HUGIN_AP_TYPE);
    //if (t < 0) { // Onur

        wxString tmp[5];
    	tmp[0] = _("Autopano (version 1.03 or greater), from http://autopano.kolor.com");
    	tmp[1] = _("Autopano-Sift, from http://user.cs.tu-berlin.de/~nowozin/autopano-sift/");
    	tmp[2] = _("Feature Matching with Linear Search"); // Onur
    	tmp[3] = _("Feature Matching with Single KDTree"); // Onur
    	tmp[4] = _("Feature Matching with Multiple KDTree"); // Onur
    	// determine autopano type
    	wxSingleChoiceDialog d(NULL,  _("Choose which autopano program should be used\n"), _("Select autopano type"),
	   	 	       5, tmp, NULL);
        
        if (d.ShowModal() == wxID_OK) {
            t = d.GetSelection();
        } else {
            return cps;
        }
    //}
    
#ifdef __WXMAC__
    if(t==0)
    {
        if(wxMessageBox(_("Autopano from http://autopano.kolor.com is not available for OSX"), 
                        _("Would you like to use Autopano-Sift instead?"),
                        wxOK|wxCANCEL|wxICON_EXCLAMATION)
           == wxOK) t=1;
        else return cps;
    }
#endif
    switch (t) {
	case 0:
	{
	    // autopano@kolor
	    AutoPanoKolor matcher;
	    cps = matcher.automatch(pano, imgs, nFeatures);
	    break;
	}
	case 1:
	{
	    // autopano-sift
	    AutoPanoSift matcher;
	    cps = matcher.automatch(pano, imgs, nFeatures);
	    break;
	}
	case 2: // Onur
	{
	    // feature matching with linear search
		int clk = clock();
		HuginBase::Panorama pano2 = pano;
		HuginBase::PanoramaData* panodata = pano2.getNewCopy();
		HuginBase::FeatureMatchingLinearSearch matcher(*panodata);
		cout << "Linear Search (init): " << (clock()-clk)/CLK_TCK << " seconds" << endl;
		clk = clock();
		matcher.runAlgorithm();
		cout << "Linear Search (run): " << (clock()-clk)/CLK_TCK << " seconds" << endl;
	    cps = matcher.getControlPoints();
		printf("%d matches found\n", cps.size());
	    break;
	}
	case 3: // Onur
	{
	    // feature matching with single kd-tree
		int clk = clock();
		HuginBase::Panorama pano2 = pano;
		HuginBase::PanoramaData* panodata = pano2.getNewCopy();
		HuginBase::FeatureMatchingSingleKdTree matcher(*panodata);
		cout << "Single kd-tree (init): " << (clock()-clk)/CLK_TCK << " seconds" << endl;
		clk = clock();
		matcher.runAlgorithm();
		cout << "Single kd-tree (run): " << (clock()-clk)/CLK_TCK << " seconds" << endl;
	    cps = matcher.getControlPoints();
		printf("%d matches found\n", cps.size());
	    break;
	}
	case 4: // Onur
	{
	    // feature matching with multiple kd-trees
		int clk = clock();
		HuginBase::Panorama pano2 = pano;
		HuginBase::PanoramaData* panodata = pano2.getNewCopy();
		HuginBase::FeatureMatchingMultiKdTree matcher(*panodata);
		cout << "Multiple kd-tree (init): " << (clock()-clk)/CLK_TCK << " seconds" << endl;
		clk = clock();
		matcher.runAlgorithm();
		cout << "Multiple kd-tree (run): " << (clock()-clk)/CLK_TCK << " seconds" << endl;
	    cps = matcher.getControlPoints();
		printf("%d matches found\n", cps.size());
	    break;
	}
	default:
	    DEBUG_ERROR("Invalid autopano type");
    }
    wxConfigBase::Get()->Write(wxT("/AutoPano/Type"),t);
    return cps;
}

CPVector AutoPanoSift::automatch(Panorama & pano, const UIntSet & imgs,
                                     int nFeatures)
{
    CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    }
    // create suitable command line..

    bool customAutopanoExe = HUGIN_APSIFT_EXE_CUSTOM;
    wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExeCustom"), &customAutopanoExe);

#if (defined __WXMAC__) && defined MAC_SELF_CONTAINED_BUNDLE
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"), wxT(HUGIN_APSIFT_EXE));
    
    /*if (customAutopanoExe)*/
	if (autopanoExe == wxT(HUGIN_APSIFT_EXE)) 
    {
        // Check first for autopano-sift-c
        autopanoExe = MacGetPathToBundledResourceFile(CFSTR("autopano-sift-c"));

        if(autopanoExe == wxT(""))
        {
		  wxMessageBox(wxT(""), _("Specified Autopano-SIFT not installed in bundle."));
                return cps;
        }
    } else if (autopanoExe == wxT("panomatic")) {
		// Check for panomatic
		autopanoExe = MacGetPathToBundledResourceFile(CFSTR("panomatic"));

        if(autopanoExe == wxT(""))
        {
		  wxMessageBox(wxT(""), _("Specified panomatic not installed in bundle."));
                return cps;
        }
    } else if (autopanoExe == wxT("matchpoint-complete-mac.sh")) {
		// Check for matchpoint/generatekeys shell script
		autopanoExe = MacGetPathToBundledResourceFile(CFSTR("matchpoint-complete-mac.sh"));

        if(autopanoExe == wxT(""))
        {
		  wxMessageBox(wxT(""), _("Specified matchpoint-complete-mac.sh not installed in bundle."));
                return cps;
        }
	} else if(!wxFileExists(autopanoExe)) {
        /*wxLogError(_("Autopano-SIFT not found. Please specify a valid path in the preferences"));
        return cps; */
		wxFileDialog dlg(0,_("Select autopano frontend (script)"),
		wxT(""), wxT(""),
		_("Exe or Script (*.*)|*.*"),
		wxOPEN, wxDefaultPosition);
		if (dlg.ShowModal() == wxID_OK) {
		   autopanoExe = dlg.GetPath();
		wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExe"), autopanoExe);
    } else {
             wxLogError(_("No autopano selected"));
             return cps;
      }
    }
#elif defined __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"),wxT(HUGIN_APSIFT_EXE));
    if (! customAutopanoExe) {
        autopanoExe = wxT(HUGIN_APSIFT_EXE);
    } else {
        if(!wxFileExists(autopanoExe)) {
            wxLogError(_("Autopano-SIFT not found. Please specify a valid path in the preferences"));
            return cps;
        }
    }
#else
    // autopano should be in the path on linux
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"),wxT(HUGIN_APSIFT_EXE));
#endif

    wxString autopanoArgs = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/Args"),
                                                      wxT(HUGIN_APSIFT_ARGS));

#ifdef __WXMSW__
    // remember cwd.
    wxString cwd = wxGetCwd();
    wxString apDir = wxPathOnly(autopanoExe);
    if (apDir.Length() > 0) {
        wxSetWorkingDirectory(apDir);
    }
#endif

    // TODO: create a secure temporary filename here
    wxString ptofile = wxFileName::CreateTempFileName(wxT("ap_res"));
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);

    SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    autopanoArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    autopanoArgs.Replace(wxT("%f"), tmp);

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;

    long idx = autopanoArgs.Find(wxT("%namefile")) ;
    DEBUG_DEBUG("find %namefile in '"<< autopanoArgs.mb_str(wxConvLocal) << "' returned: " << idx);
    bool use_namefile = idx >=0;
    idx = autopanoArgs.Find(wxT("%i"));
    DEBUG_DEBUG("find %i in '"<< autopanoArgs.mb_str(wxConvLocal) << "' returned: " << idx);
    bool use_params = idx >=0;
    idx = autopanoArgs.Find(wxT("%s"));
    bool use_inputscript = idx >=0;

    if (! (use_namefile || use_params || use_inputscript)) {
        wxMessageBox(_("Please use  %namefile, %i or %s to specify the input files for autopano-sift"),
                     _("Error in Autopano command"), wxOK | wxICON_ERROR);
        return cps;
    }

    wxFile namefile;
    wxString namefile_name;
    if (use_namefile) {
        // create temporary file with image names.
        namefile_name = wxFileName::CreateTempFileName(wxT("ap_imgnames"), &namefile);
        DEBUG_DEBUG("before replace %namefile: " << autopanoArgs.mb_str(wxConvLocal));
        autopanoArgs.Replace(wxT("%namefile"), namefile_name);
        DEBUG_DEBUG("after replace %namefile: " << autopanoArgs.mb_str(wxConvLocal));
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            namefile.Write(wxString(pano.getImage(*it).getFilename().c_str(), HUGIN_CONV_FILENAME));
            namefile.Write(wxT("\r\n"));
            imgNr++;
        }
        // close namefile
        if (namefile_name != wxString(wxT(""))) {
            namefile.Close();
        }
    } else {
        string imgFiles;
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
            imgNr++;
        }
        autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), HUGIN_CONV_FILENAME));
    }

    wxString ptoinfile_name;
    if (use_inputscript) {
        wxFile ptoinfile;
        // create temporary project file
        ptoinfile_name = wxFileName::CreateTempFileName(wxT("ap_inproj"), &ptoinfile);
        autopanoArgs.Replace(wxT("%s"), ptoinfile_name);

#ifdef __GNUC__
        {
            __gnu_cxx::stdio_filebuf<char> fbuf(ptoinfile.fd(), ios::out, 100);
            ostream ptoinstream(&fbuf);
            pano.printPanoramaScript(ptoinstream, pano.getOptimizeVector(), pano.getOptions(), imgs, false);
        }
#else
        ptoinfile.Close();
        ofstream ptoinstream(ptoinfile_name.mb_str(wxConvFile));
        pano.printPanoramaScript(ptoinstream, pano.getOptimizeVector(), pano.getOptions(), imgs, false);
#endif
    }

#ifdef __WXMSW__
    if (autopanoArgs.size() > 32000) {
        wxMessageBox(_("autopano command line too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL | wxICON_ERROR );
        return cps;
    }
#endif

    wxString cmd = autopanoExe + wxT(" ") + autopanoArgs;
    DEBUG_DEBUG("Executing: " << autopanoExe.mb_str(wxConvLocal) << " " << autopanoArgs.mb_str(wxConvLocal));

    int ret = 0;

    // use MyExternalCmdExecDialog
    ret = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, 0,  _("finding control points"));

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"), wxOK | wxICON_ERROR);
        return cps;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
		     _("wxExecute Error"),
                     wxOK | wxICON_ERROR);
        return cps;
    }

    if (! wxFileExists(ptofile.c_str())) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\nThis is an indicator that the autopano call failed,\nor wrong command line parameters have been used.\n\nAutopano command: ")
                     + cmd, _("autopano failure"), wxOK | wxICON_ERROR );
        return cps;
    }

    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(HUGIN_CONV_FILENAME), pano);

#ifdef __WXMSW__
	// set old cwd.
	wxSetWorkingDirectory(cwd);
#endif

    if (namefile_name != wxString(wxT(""))) {
        namefile.Close();
        wxRemoveFile(namefile_name);
    }

    if (ptoinfile_name != wxString(wxT(""))) {
        wxRemoveFile(ptoinfile_name);
    }

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }

    return cps;
}


CPVector AutoPanoKolor::automatch(Panorama & pano, const UIntSet & imgs,
                              int nFeatures)
{
    CPVector cps;
#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/AutopanoExe"), wxT(HUGIN_APKOLOR_EXE));
    if(!wxFileExists(autopanoExe)) {
        wxLogError(_("autopano.exe not found. Please specify a valid path in the preferences"));
        return cps;
    }
#else
    // todo: selection of autopano on linux..
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/AutopanoExe"),wxT(HUGIN_APKOLOR_EXE));
#endif

    // write default autopano.kolor.com flags
    wxString autopanoArgs = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/Args"),
                                                      wxT(HUGIN_APKOLOR_ARGS));

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;
    string imgFiles;
    int imgNr=0;
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
    {
        imgMapping[imgNr] = *it;
        imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
        imgNr++;
    }

    wxString ptofilepath = wxFileName::CreateTempFileName(wxT("ap_res"));
    wxFileName ptofn(ptofilepath);
    wxString ptofile = ptofn.GetFullName();
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);
    SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    autopanoArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    autopanoArgs.Replace(wxT("%f"), tmp);

    autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), HUGIN_CONV_FILENAME));

    wxString tempdir = ptofn.GetPath();
	autopanoArgs.Replace(wxT("%d"), ptofn.GetPath());
    wxString cmd;
    cmd.Printf(wxT("%s %s"), utils::wxQuoteFilename(autopanoExe).c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 32766) {
        wxMessageBox(_("autopano command line too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL );
        return cps;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());

    int ret = 0;
    // use MyExternalCmdExecDialog
    ret = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, 0, _("finding control points"));

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"),
                      wxOK | wxICON_ERROR);
        return cps;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
		     _("wxExecute Error"),
                     wxOK | wxICON_ERROR);
        return cps;
    }

    ptofile = ptofn.GetFullPath();
    ptofile.append(wxT("0.oto"));
    if (! wxFileExists(ptofile.c_str()) ) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\nThis is an indicator that the autopano call failed,\nor wrong command line parameters have been used.\n\nAutopano command: ")
                     + cmd + _("\n current directory:") +
			         wxGetCwd(),
		             _("autopano failure"), wxCANCEL );
        return cps;
    }
    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(HUGIN_CONV_FILENAME), pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
    return cps;
}


