// -*- c-basic-offset: 4 -*-
/** @file PTOFileMgr.cpp
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

#include <fstream>
#include <sstream>
#include "PTOFileMgr.h"

using namespace std;

PTOFileMgr *PTOFileMgr::m_instance = 0;

PTOFileMgr::PTOFileMgr() {}
PTOFileMgr::~PTOFileMgr() {}

PTOFileMgr* PTOFileMgr::getInstance()
{
    if(m_instance)
        return m_instance;
    return m_instance = new PTOFileMgr();
}

void PTOFileMgr::init()
{
    m_files.clear();
    m_PTOfile = "";
} 

void PTOFileMgr::loadPTOFile(std::string filename)
{
    init();
    try {
        ifstream fin(filename.c_str());
        string buffer, token;
        stringstream ss;
        m_PTOfile = filename;
        while(!fin.eof())
        {
            getline(fin, buffer, '\n');
            ss.clear();
            ss.str(buffer);

            ss >> token;
            if(token == "i") {
                int i = buffer.find_last_of("n\"", 0);
                int j = buffer.find_last_of("\"", i+2);
                m_files.push_back(buffer.substr(i+2, j-i-2));
                continue;
            }
        }
    }catch(exception &e) {
        init();
        throw e;
    }

}

vector<string> PTOFileMgr::getFiles()
{
    return m_files;
}

string PTOFileMgr::getFile(int index)
{
    if(index < 0 || index >= m_files.size() || m_files.empty())
        throw;// exception("invalid index");
    return m_files[index];
}   
