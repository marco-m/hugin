// -*- c-basic-offset: 4 -*-
/** @file MaskEdApp.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id:  MaskEdApp.cpp $
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "MaskEdApp.h"
#include "MaskEdMainFrame.h"
#include <wx/xrc/xmlres.h>
#include <wx/fs_zip.h>
#include "hugin_utils/utils.h"

IMPLEMENT_APP(MaskEdApp)

bool MaskEdApp::OnInit()
{
    DEBUG_TRACE("MaskEdApp::OnInit()");

#if defined __WXMSW__
    TCHAR szModulePath[MAX_PATH];
    GetModuleFileName(0, szModulePath, sizeof(szModulePath) - 1);
    wxString MaskEdExeDir(szModulePath);
    wxString MaskEdRoot;
    wxFileName::SplitPath( MaskEdExeDir, &MaskEdRoot, NULL, NULL );
    wxFileName::SplitPath( MaskEdRoot, &MaskEdRoot, NULL, NULL );
    m_xrcPrefix = MaskEdRoot + wxT("/share/maskeditor/xrc/");
    m_utilsBinDir = MaskEdRoot + wxT("/bin/");
    
#elif defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // initialize paths
    {
        wxString thePath = MacGetPathToBundledResourceFile(CFSTR("xrc"));
        if (thePath == wxT("")) {
            wxMessageBox(_("xrc directory not found in bundle"), _("Fatal Error"));
            return false;
        }
        m_xrcPrefix = thePath + wxT("/");
    }

#else
    // add the locale directory specified during configure
    m_xrcPrefix = wxT(INSTALL_XRC_DIR);

#endif

    wxFileSystem::AddHandler(new wxArchiveFSHandler);

    // Init Image handlers
    wxInitAllImageHandlers();

    // Init XRC handlers
    wxXmlResource::Get()->InitAllHandlers();

    // Load XRC resources
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_frame.xrc"));
    
    MaskEdMainFrame *frame = new MaskEdMainFrame(NULL);
    frame->Show();
    SetTopWindow(frame);
    return true;
}
