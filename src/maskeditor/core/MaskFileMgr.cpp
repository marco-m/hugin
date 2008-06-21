// -*- c-basic-offset: 4 -*-
/** @file MaskFileMgr.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id$
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
#include <wx/wx.h>
#include <wx/filename.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include "MaskFileMgr.h"

using namespace std;

MaskFileMgr* MaskFileMgr::m_instance = 0;

MaskFileMgr::MaskFileMgr() {}
MaskFileMgr::~MaskFileMgr() {}

MaskFileMgr* MaskFileMgr::getInstance()
{
    if(m_instance)
        return m_instance;
    return m_instance = new MaskFileMgr();
}

void MaskFileMgr::Init()
{
    m_maskfile = "";
    m_ptofile = "";
    m_imgfiles.clear();
}

void MaskFileMgr::LoadFile(std::string filename)
{
    if(filename.empty())
        throw;// exception("invalid filename");
    wxString path(filename.c_str(), wxConvUTF8);
    wxString prefix;
    
    wxFileName::SplitPath(path, &prefix, NULL, NULL);
    string   strprefix = string(prefix.mb_str()) + "/";
    Init();
    try
    {
        ifstream fin(filename.c_str());
        string buffer, token, imgfilename;
        stringstream ss;
        m_maskfile = filename;
        while(!fin.eof())
        {
            getline(fin, buffer, '\n');
            ss.clear();
            ss.str(buffer);
            if(ss >> token) {
                if(token[0] == '#') //beginning of comment
                    continue; //skip
                else if(token == "pto") { //pto file
                    ss >> m_ptofile;
                    m_ptofile = strprefix + m_ptofile;
                }else if(token == "ri") {
                    ss >> imgfilename;
                    m_imgfiles.push_back(strprefix + imgfilename);
                }
            }
        }
    }catch(exception &e) {
        Init();
        throw e;
    }
}

vector<string> MaskFileMgr::getImgFiles()
{
    return m_imgfiles;
}

string MaskFileMgr::getImgFile(int index)
{
    if(index < 0 || index >= m_imgfiles.size() || m_imgfiles.empty())
        throw;// exception("invalid index");
    return m_imgfiles[index];
}

string MaskFileMgr::getPTOFile()
{
    return m_ptofile;
}
