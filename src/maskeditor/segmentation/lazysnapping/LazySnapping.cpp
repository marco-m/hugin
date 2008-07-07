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
#include <memory>
#include "huginapp/ImageCache.h"
#include "LazySnapping.h"
#include "kmeans/KMeans.hpp"
#include "watershed.cxx"

using namespace std;
using HuginBase::ImageCache;

#define SAFE_DELETE(x) { if(x) delete x; x = 0; }
double SQR(double x) { return x * x; }
/**
 * General utility functions
 */
template <typename T, int CDIM>
bool operator<(const LazySnapping::ClusterCoord<T,CDIM> &a, const LazySnapping::ClusterCoord<T,CDIM> &b)
{
    for(int i = 0; i < CDIM; i++)
        if(a[i] > b[i])
            return false;
    return true;
}
/**
 *
 */
LazySnapping::LazySnapping() 
: m_mask(0), m_data(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA),
m_nclusters(NCLUSTERS), m_K(-1), m_width(0), m_height(0), m_depth(0), m_cnodes(0),
m_cluster_asgn(0)
{
    m_name = "LazySnapping";
}
LazySnapping::LazySnapping(const string &filename) 
: m_mask(0), m_data(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA),
m_nclusters(NCLUSTERS), m_K(-1), m_width(0), m_height(0), m_depth(0), m_cnodes(0),
m_cluster_asgn(0)
{
    m_name = "LazySnapping";
    setImage(filename);
}
LazySnapping::~LazySnapping() 
{
    reset();
}

void LazySnapping::init()
{
    if(m_width > 0 && m_height > 0)
    {
        m_mask = new wxBitmap(m_width, m_height, 1);
        preprocess();
        buildGraph();
        wxMemoryDC dc(*m_mask);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
    }
}

void LazySnapping::reset()
{
    m_K = -1;
    m_width = 0;
    m_height = 0;
    m_depth = 0;
    m_cnodes = 0;
    m_outline.clear();
    SAFE_DELETE(m_nodes);
    SAFE_DELETE(m_graph);
    SAFE_DELETE(m_data);
    SAFE_DELETE(m_cluster_asgn);
}

void LazySnapping::getRC(int p, int *r, int *c) const
{
    *r = p/m_width;
    *c = p%m_width;
}

void LazySnapping::preprocess()
{
    //create clusters
    m_cnodes = m_width * m_height;
    ClusterCoordType *data = new ClusterCoordType[m_cnodes * CLUSTERCOORDDIM];
    //assert(m_depth == 3);
    for(int i = 0; i < m_cnodes; i++)
    {
        *(data + i * CLUSTERCOORDDIM) = (ClusterCoordType) *(m_data + i * m_depth);
        *(data + i * CLUSTERCOORDDIM + 1) = (ClusterCoordType) *(m_data + i * m_depth + 1);
        *(data + i * CLUSTERCOORDDIM + 2) = (ClusterCoordType) *(m_data + i * m_depth + 2);
        //*(data + i * CLUSTERCOORDDIM + 3) = (ClusterCoordType) r;
        //*(data + i * CLUSTERCOORDDIM + 4) = (ClusterCoordType) c;
    }

    KMeans<ClusterCoordType> kmeans(data, m_width * m_height, CLUSTERCOORDDIM, m_nclusters);
    ClusterCoordType *clusters = 0;
    kmeans.doClustering(&clusters, &m_cluster_asgn);
    m_clusters.clear();
    for(int i = 0; i < m_nclusters; i++)
    {
        m_clusters.push_back(
            make_pair(
            ClusterCoord<ClusterCoordType,CLUSTERCOORDDIM>((clusters + i * CLUSTERCOORDDIM)), 
            NOLABEL)
            );
    }
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

inline double LazySnapping::getDistSqrPixel(const PixelCoord &p, const PixelCoord &q) const
{
    return SQR(p.x - q.x) + SQR(p.y - q.y);
}

inline double LazySnapping::getDistSqrPixel(int p, int q) const
{
    int r1,c1,r2,c2;
    getRC(p, &r1, &c1);
    getRC(q, &r2, &c2);
    PixelCoord pp(c1, r1);
    PixelCoord qq(c2, r2);
    return getDistSqrPixel(pp, qq);
}

inline double LazySnapping::getDistPixel(int p, int q) const 
{   
    return sqrt(getDistSqrPixel(p, q));
}

double LazySnapping::getDistSqrPixelIntensity(int p, int q) const
{
    PixelColor clr1, clr2;
    
    clr1 = getPixelValue(p);
    clr2 = getPixelValue(q);

    return SQR(clr1.r - clr2.r)+SQR(clr1.g - clr2.g)+ SQR(clr1.b - clr2.b) ;
}

int LazySnapping::getNeighbor(int p, int n)
{
   static const int N8[4][2] = {0,  1,
                 1,  1,
                 1,  0,
                 1, -1,
                 /*0, -1,
                -1, -1,
                -1,  0,
                -1,  1,*/};
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
    //return exp(-getDistSqrPixelIntensity(p, q)/(2*m_sigma*m_sigma)) * (1.0/getDistPixel(p,q));
    return 1.0/getDistSqrPixelIntensity(p, q);
}

void LazySnapping::computeLikelihood(int p, captype &f, captype &b)
{
    double df = -1;  //min distance from foreground clusters
    double db = -1;  //min distance from background clusters
    double d;
    int r, c;
    getRC(p, &r, &c);
    //PixelCoord pp(c, r);
    PixelColor clr = getPixelValue(p);
    //ClusterCoordType pp[CLUSTERCOORDDIM] = {clr.r, clr.g, clr.b};
    ClusterCoord<ClusterCoordType> cc(clr, r, c);
    for(tCluster::iterator it = m_clusters.begin(); it != m_clusters.end(); it++)
    {
        //d = getDistSqrPixel(it->first, pp);
        //d = EuclideanDistSqr<ClusterCoord<ClusterCoordType>, double>(&(it->first), &cc, CLUSTERCOORDDIM);
        d = cc.getDistSqr(it->first);
        if(it->second == FGND) {
            if(df < 0 || d < df)
                df = d;
        } else if(db < 0 || d < db) {
            db = d;
        }
    }
    //df = sqrt(df);
    //db = sqrt(db);
    f = df/(df + db);
    b = db/(df + db);
}

void LazySnapping::buildGraph()
{
    SAFE_DELETE(m_graph);
    SAFE_DELETE(m_nodes);
    m_graph = new Graph();
    m_cnodes = m_width * m_height;
    m_nodes = new Graph::node_id[m_cnodes];
    int i, j, n;
    for(i = 0; i < m_cnodes; i++)
        m_nodes[i] = m_graph->add_node();

    captype cap, rev_cap;
    for(i = 0; i < m_cnodes; i++)
        for(n = 0; n < 4; n++)
           if((j = getNeighbor(i, n)) > -1)
            {
                cap = rev_cap = computePrior(i, j);
                if(m_K < 0 || cap > m_K)
                    m_K = cap;
                m_graph->add_edge(m_nodes[i], m_nodes[j], cap, rev_cap);
            }
}

void LazySnapping::markPixels(vector<PixelCoord> coords, Label label)
{
    for(vector<PixelCoord>::iterator it = coords.begin(); it != coords.end(); it++)
    {
        if(m_seeds.find(*it) == m_seeds.end()) //if the pixel wasn't previously marked
        {
            m_seeds[*it] = label;
            //update stats
            PixelColor p = getPixelValue(*it);
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
            //mark clusters
            markCluster(*it, label); /*pc = findCluster(*it);*/
        }
    }   
    //update graph
    updateGraph(coords, label);

    //perform segmentation
    segment();
}

void LazySnapping::markCluster(PixelCoord p, Label label)
{
    int i = p.y * m_width + p.x;
    if(m_clusters[m_cluster_asgn[i]].second == NOLABEL)
        m_clusters[m_cluster_asgn[i]].second = label;
    else if(m_clusters[m_cluster_asgn[i]].second != label)
        assert(0);
}

//ClusterCoord LazySnapping::findCluster(PixelCoord p)
//{
//    tCluster::iterator it = m_clusters.begin();
//    PixelCoord center = it->first;
//    double dmin = getDistSqrPixel(p, center);
//    double d;
//    for(it++; it != m_clusters.end(); it++)
//    {
//        d = getDistSqrPixel(p, it->first);
//        if(d < dmin){
//            dmin = d;
//            center = it->first;
//        }
//    }
//    return center;
//}
//
//ClusterCoord LazySnapping::findCluster(int p)
//{
//    int r, c;
//    getRC(p, &r, &c);
//    PixelCoord pp(c, r);
//    return findCluster(pp);
//}
//template<typename T, int CDIM>
//ClusterCoord<T,CDIM> LazySnapping::findCluster(ClusterCoord p)
//{
//    /*tCluster::iterator it = m_clusters.begin();
//    PixelCoord center = it->first;
//    double dmin = getDistSqrPixel(p, center);
//    double d;
//    for(it++; it != m_clusters.end(); it++)
//    {
//        d = getDistSqrPixel(p, it->first);
//        if(d < dmin){
//            dmin = d;
//            center = it->first;
//        }
//    }*/
//    ClusterCoord<T,CDIM> center;
//    return center;
//}

//int LazySnapping::findCluster(int p)
//{
//    return m_cluster_asgn[p];
//}

void LazySnapping::updateGraph(vector<PixelCoord> coords, Label label)
{
    captype f, b;
    int i, r, c;
    for(vector<PixelCoord>::iterator it = coords.begin(); it != coords.end(); it++)
    {
        i = (*it).y * m_width + (*it).x;
        if(label == FGND) {
            m_graph->add_tweights(m_nodes[i], m_K, 0);
        } else {
            m_graph->add_tweights(m_nodes[i], 0, m_K);
        }
    }
    
    for(i = 0; i < m_cnodes; i++)
    {
        getRC(i, &r, &c);
        PixelCoord p(c, r);
        if(m_seeds.find(p) == m_seeds.end()) { //if current pixel is not marked as seed
            computeLikelihood(i, f, b);
            m_graph->add_tweights(m_nodes[i], f, b);
        }
    }
}

void LazySnapping::segment()
{
    Graph::flowtype flow = m_graph->maxflow();
    //go through each node and find out what segment it is in
    for(int i = 0; i < m_cnodes; i++)
    {
        if(m_graph->what_segment(m_nodes[i]) == Graph::SOURCE) {
            setMask(i, FGND);
        } else {
            setMask(i, BKGND);
        }
    }
}

void LazySnapping::setMask(int node, Label label)
{
 //   assert(0);
    int r, c;
    getRC(node, &r, &c);
    wxMemoryDC dc(*m_mask);
    //if(label == BKGND)
        dc.SetPen(*wxBLACK);
    //else
    //    dc.SetPen(*wxWHITE);
    dc.DrawPoint(c, r);
    assert(dc.IsOk());
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

wxBitmap* LazySnapping::getImage() const
{
    return new wxBitmap((const char*)m_data, m_width, m_height, m_depth);
}
