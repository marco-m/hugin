// -*- c-basic-offset: 4 -*-
/** @file PTOFileMgr.h
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

#ifndef PTOFILEMGR_H
#define PTOFILEMGR_H

#include <string>
#include <vector>

class PTOFileMgr
{
    std::string m_PTOfile;
    std::vector<std::string> m_files;

    static PTOFileMgr *m_instance;

    PTOFileMgr();
    ~PTOFileMgr();

    void init();
public:
    static PTOFileMgr* getInstance();

    void loadPTOFile(std::string filename);
    std::vector<std::string> getFiles();
    std::string getFile(int index);
};

#endif
