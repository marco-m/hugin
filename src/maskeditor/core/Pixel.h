// -*- c-basic-offset: 4 -*-
/** @file Pixel.h
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

#ifndef PIXEL_H
#define PIXEL_H
#include <wx/wx.h>
/* struct PixelCoord
    {
        int r, c;
        PixelCoord(int r, int c) : r(r), c(c) {}
        PixelCoord(wxPoint pt) : r(pt.y), c(pt.x) {}
        wxPoint operator=(PixelCoord &p) 
        {
            return wxPoint(p.c, p.r);
        }
    };*/
typedef wxPoint PixelCoord;
struct PixelColor
{
    unsigned char r, g, b;
    PixelColor(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0): r(r), g(g), b(b){}
    friend bool operator<(const PixelColor &pa, const PixelColor &pb);
};

struct Pixel 
{
    PixelCoord coord;
    PixelColor rgb;
    Pixel(PixelCoord coord, PixelColor rgb) : coord(coord), rgb(rgb)  {}   
    Pixel(PixelCoord coord, unsigned char r, unsigned char g, unsigned char b) : coord(coord)
    {
        rgb.r = r;
        rgb.g = g;
        rgb.b = b;
    }
    friend bool operator<(const Pixel &pa, const Pixel &pb);
};
struct cmp_pixel_coord 
{
    bool operator()(const PixelCoord &pa, const PixelCoord &pb) const
    {
        if(pa.x == pb.x)
            return pa.y < pb.y;
        return pa.x < pb.x;
    }
};
//struct cmp_pixel_color
//{
//    bool operator()(const PixelColor &pa, const PixelColor &pb) const
//    {
//        if(pa.rgb[0] == pb.rgb[0]) {
//            if(pa.rgb[1] == pb.rgb[1])
//                return pa.rgb[2] < pb.rgb[2];
//            return pa.rgb[1] < pb.rgb[1];
//        }
//        return pa.rgb[0] < pb.rgb[0];
//    }
//};

#endif