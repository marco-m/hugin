// -*- c-basic-offset: 4 -*-
/** @file MaskFileMgr.h
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Rev$
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

#ifndef MASKFILEMGR_H
#define MASKFILEMGR_H

#include <string>
#include <vector>
/**
 *  @class MaskFileMgr
 *  @brief singleton class used for managing mask project files
 */
class MaskFileMgr
{
    std::string m_maskfile;
    std::vector<std::string> m_imgfiles;
    std::string m_ptofile;

    static MaskFileMgr *m_instance;

    MaskFileMgr();
    ~MaskFileMgr();
    void Init();
public:
    static MaskFileMgr* getInstance();

    void LoadFile(std::string filename);

    std::string getPTOFile();
    std::vector<std::string> getImgFiles();
    int getImgFileCount();
    std::string getImgFile(int index);

};
#endif
