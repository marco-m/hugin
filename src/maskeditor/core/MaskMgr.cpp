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
#include "vigra/impex.hxx"
#include "MaskMgr.h"
#include "MaskFileMgr.h"
#include <wx/wx.h>
#include "hugin_utils/utils.h"

#if defined(WIN32) && defined(__WXDEBUG__)
#include <crtdbg.h>
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

using namespace std;
using namespace vigra;
using namespace hugin_utils;
using HuginBase::ImageCache;

MaskMgr* MaskMgr::m_instance = 0;

MaskMgr::MaskMgr() : m_segmentation_index(-1) 
{
	// TODO: read the possible options from a configuration file
	// ..
    string opts[] = {"PolyEd_Basic", "LazySnapping" };
    m_segmentation_options.assign(opts, opts+2);
}

MaskMgr::~MaskMgr() 
{
    init();
	m_segmentation_options.clear();
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
    m_loadtype = LOADFILE;
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
    m_loadtype = LOADMEM;
    m_imgfiles.push_back(imgId);
    if(m_segmentation_options[m_segmentation_index] ==  "PolyEd_Basic")
        m_segmentation.push_back(new PolyEd_Basic(imgId, img, alpha));
    else if(m_segmentation_options[m_segmentation_index] == "LazySnapping")
        m_segmentation.push_back(new LazySnapping(imgId, img, alpha));
}

//void MaskMgr::loadImage(const std::vector<vigra::BRGBImage*> &imgs, std::vector<vigra::BImage*> &alphas)
//{
//    init();
//    int i = 0;
//    ostringstream imgId;
//    if(alphas.size() == imgs.size()) {
//        vector<vigra::BImage*>::iterator it_alpha = alphas.begin();
//        for(vector<vigra::BRGBImage*>::const_iterator it = imgs.begin(); it != imgs.end(); it++, it_alpha++) 
//        {
//            imgId << i;
//            loadImage(imgId.str(), *it, *it_alpha);
//        }
//    } else {
//        for(vector<vigra::BRGBImage*>::const_iterator it = imgs.begin(); it != imgs.end(); it++, i++)
//        {
//            imgId << i;
//            loadImage(imgId.str(), *it);
//        }
//    }
//}

void MaskMgr::loadImage(std::vector<std::pair<vigra::BRGBImage*, vigra::BImage*> > &imgs, bool breload)
{
    init();
    int i = 0;
    ostringstream imgId;
    if(!breload) {
        m_imgs_alpha.clear();
        copy(imgs.begin(), imgs.end(), back_insert_iterator<std::vector<std::pair<vigra::BRGBImage*, vigra::BImage*> > >(m_imgs_alpha));
    }
    m_imgfiles.clear();
    for(vector<pair<vigra::BRGBImage*, vigra::BImage*> >::iterator it = m_imgs_alpha.begin(); it != m_imgs_alpha.end(); it++, i++)
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

void MaskMgr::saveMaskProject(const string &filename)
{
    MaskFileMgr::getInstance()->saveFile(filename, m_imgfiles);
}

void MaskMgr::reload()
{
    if(m_loadtype == LOADFILE) {
        if(!m_imgfiles.empty()) {
            vector<string> tmp;
            copy(m_imgfiles.begin(), m_imgfiles.end(), back_insert_iterator<vector<string> >(tmp));
            /*m_segmentation.clear();
            m_imgfiles.clear();*/
            init();
            loadImage(tmp);
        }
    }else {
        loadImage(m_imgs_alpha, true);
    }
}

vector<string> MaskMgr::getImages()
{
    return m_imgfiles;
}

struct BRGBToBImage
{
public:
    vigra::BImage::value_type operator() (const vigra::BRGBImage::value_type &p) const
    {
        return ((p.red() + p.green() + p.blue())/3) > 0 ? 255 : 0;
    }
};

void MaskMgr::saveMask(const std::string &prefix, const std::string &fileExt)
{
    int i = 0;
    wxBitmap *mask = 0;
    string pathprefix = prefix;//, filename;
    for(std::vector<ISegmentation*>::iterator it = m_segmentation.begin(); it != m_segmentation.end(); it++, i++)
    {
        stringstream ss;
        //pathprefix = getPathPrefix(m_imgfiles[i]);
        //filename = stripExtension(stripPath(m_imgfiles[i]));
        pathprefix = stripExtension(m_imgfiles[i]);
        //ss << prefix << "_" << i << "." << fileExt;
        ss << pathprefix << "_mask." << fileExt;
        vigra::ImageExportInfo exi( ss.str().c_str() ); 

        mask = (*it)->getMaskBitmap();
        wxImage img = mask->ConvertToImage();
        vigra::BasicImageView<RGBValue<unsigned char> > panoImg8((RGBValue<unsigned char> *)img.GetData(), img.GetWidth(), img.GetHeight());
        vigra::BImage bimg(mask->GetWidth(), mask->GetHeight());
        vigra::transformImage(srcImageRange(panoImg8), destImage(bimg), BRGBToBImage());
        //vec.push_back(bimg);

        vigra::exportImage(vigra::srcImageRange(bimg), exi);
    }
}

void MaskMgr::saveMaskMetaData(const std::string &prefix)
{
    int i = 0;
    
    for(std::vector<ISegmentation*>::iterator it = m_segmentation.begin(); it != m_segmentation.end(); it++, i++)
    {
        stringstream ss;
        ss << prefix << "_" << i << ".ls";
        
        (*it)->saveMaskMetaData(ss.str());
    }
}
