// -*- c-basic-offset: 4 -*-
/** @file Pixel.cpp
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
#include "Pixel.h"

bool operator<(const PixelColor &pa, const PixelColor &pb)
{
    if(pa.r == pb.r) {
        if(pa.g == pb.g)
            return pa.b < pb.b;
        else 
            return pa.g < pb.g;
    }
    return pa.r < pb.r;
}

bool operator<(const Pixel &pa, const Pixel &pb)
{
    if(pa.coord == pb.coord) {
        return pa.rgb < pb.rgb;
    } else if(pa.coord.y == pb.coord.y)
        return pa.coord.x < pb.coord.x;
    return pa.coord.y < pb.coord.y;
}

