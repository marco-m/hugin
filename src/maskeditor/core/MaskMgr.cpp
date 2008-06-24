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
#include "../segmentation/polyed_basic/PolyEd_Basic.h"
#include "../segmentation/lazysnapping/LazySnapping.h"

using namespace std;
using HuginBase::ImageCache;

MaskMgr* MaskMgr::m_instance = 0;
MaskMgr::MaskMgr() : m_segmentation_index(-1) 
{
    string opts[] = {"PolyEd_Basic", "LazySnapping" };
    m_segmentation_options.assign(opts, opts+1);
}
MaskMgr::~MaskMgr() 
{
    /*if(m_segmentation)
        m_segmentation->reset();
    delete m_segmentation;*/
}

//TODO: check for loading duplicate images 

MaskMgr* MaskMgr::getInstance()
{
    if(m_instance)
        return m_instance;
    return m_instance = new MaskMgr();
}

void MaskMgr::init()
{
    m_imgfiles.clear();
    for(vector<ISegmentation*>::iterator it = m_segmentation.begin(); it != m_segmentation.end(); it++)
        (*it)->reset();
    m_segmentation.clear();
}

void MaskMgr::setSegmentationOption(int index)
{
    /*m_segmentation = segmentation;
    m_segmentation->init();*/
    m_segmentation_index = index;
}

ISegmentation* MaskMgr::getSegmentation(int index) //get the segmenetation instance responsible for ith image
{
    return m_segmentation[index];
}

ISegmentation* MaskMgr::getSegmentation(const string &filename)
{
    vector<string>::iterator it = find(m_imgfiles.begin(), m_imgfiles.end(), filename);
    if(it != m_imgfiles.end())
        return getSegmentation(it - m_imgfiles.begin());
}

vector<string> MaskMgr::getSegmentationOptions()
{
    return m_segmentation_options;
}

void MaskMgr::loadImage(const string &filename)
{
    ImageCache::getInstance().getImage(filename);
    m_imgfiles.push_back(filename);
    if(m_segmentation_options[m_segmentation_index] ==  "PolyEd_Basic")
        m_segmentation.push_back(new PolyEd_Basic(filename));
    else if(m_segmentation_options[m_segmentation_index] == "LazySnapping")
        m_segmentation.push_back(new LazySnapping(filename));
}

void MaskMgr::loadImages(const vector<string> &filesv)
{
    //copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
        loadImage(*it);
}

void MaskMgr::loadMaskProject(const string &filename)
{
    init();
    MaskFileMgr::getInstance()->loadFile(filename);
    loadImages(MaskFileMgr::getInstance()->getImgFiles());
}

vector<string> MaskMgr::getImages()
{
    return m_imgfiles;
}
