// -*- c-basic-offset: 4 -*-
/** @file MaskPoly.h
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

#ifndef MASKPOLY_H
#define MASKPOLY_H

#include "Pixel.h"
#include <algorithm>

#define MASKPOLYRANGE 4

struct MaskPoly
{
    int label;
    int iMouseOver;
    std::vector<PixelCoord> pt;

    void clear() { iMouseOver = -1; pt.clear(); }
    void add(PixelCoord newpt) { pt.push_back(newpt); }
    std::vector<PixelCoord>::iterator begin() { return pt.begin(); }
    std::vector<PixelCoord>::iterator end() { return pt.end(); }
    std::vector<PixelCoord>::size_type size() { return pt.size(); }
    int  findVertex(const PixelCoord &p)
    {   
        iMouseOver = -1;
        int i = 0;
        for(std::vector<PixelCoord>::iterator it = pt.begin(); it != pt.end(); it++, i++)
        {
            if(abs(it->x - p.x) <= MASKPOLYRANGE && abs(it->y - p.y) <= MASKPOLYRANGE) {
                iMouseOver = i;
                break;
            }
        }
        return iMouseOver;
    }

    bool isMouseOver(PixelCoord &p) 
    {
        return (iMouseOver != -1) ? (p == pt[iMouseOver]) : false;
    }

    bool isMouseOver(int i)
    {
        return iMouseOver == i;
    }
};
#endif
