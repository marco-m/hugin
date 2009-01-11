// -*- c-basic-offset: 4 -*-
/** @file LazySnappingMemento.cpp
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
//#include "ISegmentation.h"
#include "Pixel.h"
#include <algorithm>
#include <vector>
#include "LazySnappingMemento.h"

using namespace std;

LazySnappingMemento::LazySnappingMemento(wxBitmap *mask, unsigned char *mask_data, int width, int height, int prev_nseeds, 
                                         int fgnd_nseeds, int bkgnd_nseeds,  
                                         int fgnd_cluster_size, int bkgnd_cluster_size)
: m_mask(mask), m_mask_data(mask_data), m_width(width), m_height(height), m_nseeds(prev_nseeds),
m_fgnd_nseeds(fgnd_nseeds), m_bkgnd_nseeds(m_bkgnd_nseeds),
m_fgnd_cluster_size(fgnd_cluster_size), m_bkgnd_cluster_size(bkgnd_cluster_size)
{}

LazySnappingMemento::~LazySnappingMemento() {}

void LazySnappingMemento::setSeeds(const vector<PixelCoord> &seeds, int label)
{   
    m_label = label;
    //copy the newly added seeds
    copy(seeds.begin(), seeds.end(), back_insert_iterator<vector<PixelCoord> >(m_seeds));
}
