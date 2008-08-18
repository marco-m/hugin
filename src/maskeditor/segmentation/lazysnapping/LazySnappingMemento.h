// -*- c-basic-offset: 4 -*-
/** @file LazySnappingMemento.h
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
#ifndef LAZYSNAPPINGMEMENTO_H
#define LAZYSNAPPINGMEMENTO_H
#include "IMaskEdMemento.h"
//#include "Pixel.h"
#include <vector>
//#include "LazySnapping.h"

class wxBitmap;

class LazySnappingMemento : public IMaskEdMemento
{
    wxBitmap *m_mask;
    int m_width, m_height, m_depth;
    std::vector<PixelCoord> m_seeds;
    int m_nseeds;        //previous number of seeds of type label
    int m_fgnd_nseeds, m_bkgnd_nseeds;
    int m_fgnd_cluster_size, m_bkgnd_cluster_size;
    int m_label;

    friend class LazySnapping;
public:
    LazySnappingMemento(wxBitmap *mask, int width, int height, int prev_nseeds, int m_fgnd_nseeds, int m_bkgnd_nseeds, int fgnd_cluster_size, int bkgnd_cluster_size);
    ~LazySnappingMemento();

    void setSeeds(const std::vector<PixelCoord> &seeds, int label);
};

#endif

