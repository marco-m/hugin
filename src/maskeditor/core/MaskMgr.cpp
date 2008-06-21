// -*- c-basic-offset: 4 -*-
/** @file MaskMgr.cpp
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

#include "huginapp/ImageCache.h"
#include "MaskMgr.h"
#include "MaskFileMgr.h"

using namespace std;
using HuginBase::ImageCache;

MaskMgr* MaskMgr::m_instance = 0;
MaskMgr::MaskMgr(){}
MaskMgr::~MaskMgr() {}

//TODO: check for loading duplicate images 

MaskMgr* MaskMgr::getInstance()
{
    if(m_instance)
        return m_instance;
    return m_instance = new MaskMgr();
}

void MaskMgr::Init()
{
    m_imgfiles.clear();
}

void MaskMgr::LoadImage(const string &filename)
{
    ImageCache::getInstance().getImage(filename);
    m_imgfiles.push_back(filename);
}

void MaskMgr::LoadImages(const vector<string> &filesv)
{
   /* for(vector<string>::iterator it = filesv.begin(); it != filesv.end(); it++)
        ImageCache::getInstance().getImage(*it);*/
    copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
}

void MaskMgr::LoadMaskProject(const string &filename)
{
    Init();
    MaskFileMgr::getInstance()->LoadFile(filename);
    LoadImages(MaskFileMgr::getInstance()->getImgFiles());
}

vector<string> MaskMgr::getImages()
{
    return m_imgfiles;
}
