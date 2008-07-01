// -*- c-basic-offset: 4 -*-
/** @file lazysnapping.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id: lazysnapping.cpp 3138 2008-06-21 03:20:03Z fmannan $
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
#include "LazySnapping.h"
#include <memory>

using namespace std;
using HuginBase::ImageCache;

#define SAFE_DELETE(x) { if(x) delete x; x = 0; }
double SQR(double x) { return x * x; }

LazySnapping::LazySnapping() : m_mask(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA)
{
    init();
}
LazySnapping::LazySnapping(const string &filename) : m_mask(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA)
{
    init();
    setImage(filename);
}
LazySnapping::~LazySnapping() 
{
    reset();
}

void LazySnapping::init()
{
    m_name = "LazySnapping";
    m_mask = new wxBitmap(m_width, m_height, 1);
    buildGraph();
    wxMemoryDC dc(*m_mask);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
}

void LazySnapping::reset()
{
    SAFE_DELETE(m_nodes);
    SAFE_DELETE(m_graph);
    SAFE_DELETE(m_data);
    //m_img.reset();
}

void LazySnapping::getRC(int p, int *r, int *c) const
{
    *r = p/m_width;
    *c = p%m_width;
}

PixelColor LazySnapping::getPixelValue(int p) const 
{
    PixelColor color;
    color.r = m_data[p];
    color.g = m_data[p + 1];
    color.b = m_data[p + 2];
    return color;
}

PixelColor LazySnapping::getPixelValue(int r, int c) const 
{
    return getPixelValue( (r * m_width + c) * m_depth );
}

PixelColor LazySnapping::getPixelValue(wxPoint pt) const 
{
    return getPixelValue(pt.y, pt.x);
}

inline double LazySnapping::distPixel(int p, int q) const 
{   
    int r1,c1,r2,c2;
    getRC(p, &r1, &c1);
    getRC(q, &r2, &c2);
    
    return sqrt((double) (r1-r2)*(r1-r2) + (c1-c2)*(c1-c2));
}

double LazySnapping::distSqrPixelIntensity(int p, int q) const
{
    PixelColor clr1, clr2;
    
    clr1 = getPixelValue(p);
    clr2 = getPixelValue(q);

    return SQR(clr1.r - clr2.r)+SQR(clr1.g - clr2.g)+ SQR(clr1.b - clr2.b) ;
}

int LazySnapping::getNeighbor(int p, int n)
{
   static const int N8[8][2] = {-1, -1,
                -1,  0,
                -1,  1,
                 0, -1,
                 0,  1,
                 1, -1,
                 1,  0,
                 1,  1,};
   int r, c;
   r = p/m_width + N8[n][0];
   c = p%m_width + N8[n][1];
   if(r >= 0 && r < m_height && c >= 0 && c < m_width)
   {
        return r * m_width + c;
   }
   return -1;
}

LazySnapping::captype LazySnapping::computePrior(int p, int q)
{
    return exp(-distSqrPixelIntensity(p, q)/(2*m_sigma*m_sigma)) * (1.0/distPixel(p,q));
}

LazySnapping::captype LazySnapping::computeLikelihood(int p)
{
    return 0;
}

void LazySnapping::buildGraph()
{
    reset();
    m_graph = new Graph();
    int cnodes = m_width * m_height;
    m_nodes = new Graph::node_id[cnodes];
    int i, j, n;
    for(i = 0; i < cnodes; i++)
        m_nodes[i] = m_graph->add_node();

    captype cap, rev_cap;
    for(i = 0; i < cnodes; i++)
        for(n = 0; n < 4; n++)
           if((j = getNeighbor(i, n)) > -1)
            {
                cap = rev_cap = computePrior(i, j);
                m_graph->add_edge(m_nodes[i], m_nodes[j], cap, rev_cap);
            }
}

void LazySnapping::markPixels(vector<PixelCoord> coords, Label label)
{
    for(vector<PixelCoord>::iterator it = coords.begin(); it != coords.end(); it++)
    {
        if(m_seeds.find(*it) != m_seeds.end())
        {
            m_seeds[*it] = label;
            //update stats
            PixelColor p(0, 0, 0);
            if(label == BKGND) {
                if(m_bkgnd.find(p) != m_bkgnd.end())
                    m_bkgnd[p] = m_bkgnd[p] + 1;
                else
                    m_bkgnd[p] = 0;
            } else {
                if(m_fgnd.find(p) != m_fgnd.end())
                    m_fgnd[p] = m_fgnd[p] + 1;
                else 
                    m_fgnd[p] = 0;
            }
        }
    }   
    //update graph
    
    //perform segmentation
}

void LazySnapping::setRegion(vector<PixelCoord> coords, Label label)
{}
void LazySnapping::setImage(unsigned char* data, int row, int col, int depth)
{
    reset();
    m_width = col;
    m_height = row;
    m_depth = depth;
    m_data = new unsigned char [m_width * m_height * m_depth];
    memcpy(m_data, data, sizeof(unsigned char) * m_width * m_height * m_depth);
    init();
}
void LazySnapping::setImage(const std::string &filename)
{
    m_filename = filename;
    if(m_mask)
        delete m_mask;
    ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
    HuginBase::ImageCache::ImageCacheRGB8Ptr m_img = e->get8BitImage();
    m_width = m_img->width();
    m_height = m_img->height();
    m_depth = 3;
    m_data = new unsigned char[m_width * m_height * m_depth];
    memcpy(m_data, m_img->data(), sizeof(unsigned char) * m_width * m_height * m_depth);
    init();    
}
wxMask* LazySnapping::getMask() const
{
    return new wxMask(*m_mask, *wxWHITE);
}

wxBitmap* LazySnapping::getMaskBitmap() const
{
    return m_mask;
}

vector<wxPoint> LazySnapping::getOutline() const
{
    return m_outline;
}

