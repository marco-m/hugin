// -*- c-basic-offset: 4 -*-
/** @file MaskMgr.h
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

#ifndef MASKMGR_H
#define MASKMGR_H

#include "vigra/stdimage.hxx"
#include <vector>
#include <string>
#include "ISegmentation.h"

class MaskMgr /*: public Singleton<MaskMgr>*/
{
    std::vector<std::string>    m_imgfiles;
    std::vector<ISegmentation*> m_segmentation; //each image will have its own instance
    std::vector<std::string> m_segmentation_options; //this should be done dynamically
    typedef std::vector<std::pair<vigra::BRGBImage*, vigra::BImage*> > tImgAlphaPair;
    tImgAlphaPair m_imgs_alpha;
    
    enum tLoadType { LOADFILE, LOADMEM };
    tLoadType                m_loadtype;
    int                      m_segmentation_index;
    static MaskMgr          *m_instance;
    MaskMgr();
    ~MaskMgr();

    void init();
public:
    static MaskMgr *getInstance();

    void saveMask(const std::string &prefix, const std::string &fileExt = "tif");

    void setSegmentationOption(int index);  // set segmentation technique
    ISegmentation* getSegmentation(int index); //get the segmentation instance used for a particular input image
    ISegmentation* getSegmentation(const std::string &filename);
    std::vector<std::string> getSegmentationOptions();
    int getSegmentationOptionSelected() const;
    void loadPTOFile(const std::string &filename);

    void loadImage(const std::string &imgId, const vigra::BRGBImage* img, vigra::BImage *alpha = NULL);
    //void loadImage(const std::vector<vigra::BRGBImage*> &imgs, std::vector<vigra::BImage*> &alphas, bool breload = false);
    void loadImage(std::vector<std::pair<vigra::BRGBImage*, vigra::BImage*> > &imgs, bool reload = false);
    void loadImage(const std::vector<std::string> &filesv);
    void loadImage(const std::string &filename);
    void loadMaskProject(const std::string &filename);
    void reload();  //reloads all images

    std::vector<std::string> getImages();
};

#endif
