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
#include <sstream>
#include "huginapp/ImageCache.h"
#include "../segmentation/polyed_basic/PolyEd_Basic.h"
#include "../segmentation/lazysnapping/LazySnapping.h"
#include "vigra/stdimage.hxx"
#include "MaskMgr.h"
#include "MaskFileMgr.h"

using namespace std;
using HuginBase::ImageCache;

MaskMgr* MaskMgr::m_instance = 0;
MaskMgr::MaskMgr() : m_segmentation_index(-1) 
{
    string opts[] = {"PolyEd_Basic", "LazySnapping" };
    m_segmentation_options.assign(opts, opts+2);
}
MaskMgr::~MaskMgr() 
{
    /*if(m_segmentation)
        m_segmentation->reset();
    delete m_segmentation;*/
    m_segmentation.clear();
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
    reload();
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
    return NULL;
}

vector<string> MaskMgr::getSegmentationOptions()
{
    return m_segmentation_options;
}

int MaskMgr::getSegmentationOptionSelected() const 
{
    return m_segmentation_index;
}

void MaskMgr::loadImage(const string &filename)
{
    //ImageCache::getInstance().getImage(filename);
    m_imgfiles.push_back(filename);
    if(m_segmentation_options[m_segmentation_index] ==  "PolyEd_Basic")
        m_segmentation.push_back(new PolyEd_Basic(filename));
    else if(m_segmentation_options[m_segmentation_index] == "LazySnapping")
        m_segmentation.push_back(new LazySnapping(filename));
}

void MaskMgr::loadImage(const vector<string> &filesv)
{
    //copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
        loadImage(*it);
}

void MaskMgr::loadImage(const string &imgId, const vigra::BRGBImage* img, vigra::BImage *alpha)
{
    m_imgfiles.push_back(imgId);
    if(m_segmentation_options[m_segmentation_index] ==  "PolyEd_Basic")
        m_segmentation.push_back(new PolyEd_Basic(imgId, img, alpha));
    else if(m_segmentation_options[m_segmentation_index] == "LazySnapping")
        m_segmentation.push_back(new LazySnapping(imgId, img, alpha));
}

void MaskMgr::loadImage(const std::vector<vigra::BRGBImage*> &imgs, std::vector<vigra::BImage*> &alphas)
{
    init();
    int i = 0;
    ostringstream imgId;
    if(alphas.size() == imgs.size()) {
        vector<vigra::BImage*>::iterator it_alpha = alphas.begin();
        for(vector<vigra::BRGBImage*>::const_iterator it = imgs.begin(); it != imgs.end(); it++, it_alpha++) 
        {
            imgId << i;
            loadImage(imgId.str(), *it, *it_alpha);
        }
    } else {
        for(vector<vigra::BRGBImage*>::const_iterator it = imgs.begin(); it != imgs.end(); it++, i++)
        {
            imgId << i;
            loadImage(imgId.str(), *it);
        }
    }
}

void MaskMgr::loadImage(std::vector<std::pair<vigra::BRGBImage*, vigra::BImage*> > &imgs)
{
    init();
    int i = 0;
    ostringstream imgId;
    for(vector<pair<vigra::BRGBImage*, vigra::BImage*> >::iterator it = imgs.begin(); it != imgs.end(); it++, i++)
    {
        imgId << i;
        loadImage(imgId.str(), it->first, it->second);
        imgId.clear();
    }
}

void MaskMgr::loadMaskProject(const string &filename)
{
    init();
    MaskFileMgr::getInstance()->loadFile(filename);
    loadImage(MaskFileMgr::getInstance()->getImgFiles());
}

void MaskMgr::reload()
{
    if(!m_imgfiles.empty()) {
        vector<string> tmp;
        copy(m_imgfiles.begin(), m_imgfiles.end(), back_insert_iterator<vector<string> >(tmp));
        /*m_segmentation.clear();
        m_imgfiles.clear();*/
        init();
        loadImage(tmp);
    }
}

vector<string> MaskMgr::getImages()
{
    return m_imgfiles;
}
