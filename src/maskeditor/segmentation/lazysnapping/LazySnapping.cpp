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
//#include "kmeans/KMeans.hpp"
#include "KMlocal/KMlocal.h"
#include "watershed.cxx"
#include <wx/wx.h>

#define INF     1000000
using namespace std;
using HuginBase::ImageCache;

#define SAFE_DELETE(x) { if(x) delete x; x = 0; }
#define SAFE_DELETE_ARRAY(x) { if(x) delete [] x; x = 0; }

inline double SQR(double x) { return x * x; }
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
struct PixelCoordToWxPoint
{
     wxPoint operator()(const PixelCoord &p)
     {
        return wxPoint(p.x, p.y);
     }
};
/**
 * LazySnapping Implementation
 */
LazySnapping::LazySnapping() 
: m_mask(0), m_data(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA),
m_nclusters(NCLUSTERS), m_K(-1), m_width(0), m_height(0), m_depth(0), m_cnodes(0)
{
    m_name = "LazySnapping";
}
LazySnapping::LazySnapping(const string &filename) 
: m_mask(0), m_data(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA),
m_nclusters(NCLUSTERS), m_K(-1), m_width(0), m_height(0), m_depth(0), m_cnodes(0)
{
    m_name = "LazySnapping";
    setImage(filename);
}

LazySnapping::LazySnapping(const string &imgId, const vigra::BRGBImage *img, vigra::BImage *mask)
: m_mask(0), m_data(0), m_nodes(0), m_graph(0), m_lambda(LAMBDA), m_sigma(SIGMA),
m_nclusters(NCLUSTERS), m_K(-1), m_width(0), m_height(0), m_depth(0), m_cnodes(0)
{
    m_name = "LazySnapping";
    setImage(imgId, img, mask);
}

LazySnapping::~LazySnapping() 
{
    reset();
}

void LazySnapping::setMemento(IMaskEdMemento *memento)
{

}

IMaskEdMemento* LazySnapping::createMemento()
{
    return 0;
}

void LazySnapping::init(vigra::BImage *mask)
{
    if(m_width > 0 && m_height > 0)
    {
        if(!mask)
            m_mask = new wxBitmap(m_width, m_height, 1);
        else {
            assert(mask->width() == m_width && mask->height() == m_height);
            m_mask = new wxBitmap((const char*)mask->data(), mask->width(), mask->height());
        }
        /*SAFE_DELETE(m_tmp_bmp);
        m_tmp_bmp = new wxBitmap(m_width, m_height, 1);*/
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
    SAFE_DELETE_ARRAY(m_nodes);
    SAFE_DELETE(m_graph);
    SAFE_DELETE_ARRAY(m_data);
}

void LazySnapping::getRC(int p, int *r, int *c) const
{
    *r = p/m_width;
    *c = p%m_width;
}

int LazySnapping::getIndex(int r, int c) const
{
    return r * m_width + c;
}

void LazySnapping::preprocess()
{
    m_cnodes = m_width * m_height;
}

PixelColor LazySnapping::getPixelValue(int p) const 
{
    PixelColor color;
    color.r = *(m_data + p * m_depth);
    color.g = *(m_data + p * m_depth + 1);
    color.b = *(m_data + p * m_depth + 2);
    return color;
}

PixelColor LazySnapping::getPixelValue(int r, int c) const 
{
    return getPixelValue( (r * m_width + c) );
}

PixelColor LazySnapping::getPixelValue(PixelCoord pt) const 
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
    double d = getDistSqrPixelIntensity(p, q);
    return 1.0/(d + 1.0);
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
    for(tCluster::iterator it = m_fgnd_clusters.begin(); it != m_fgnd_clusters.end(); it++)
    {
        //d = getDistSqrPixel(it->first, pp);
        //d = EuclideanDistSqr<ClusterCoord<ClusterCoordType>, double>(&(it->first), &cc, CLUSTERCOORDDIM);
        d = cc.getDistSqr(*it);
        if(df < 0 || d < df)
           df = d;
    }
    for(tCluster::iterator it = m_bkgnd_clusters.begin(); it != m_bkgnd_clusters.end(); it++)
    {
        d = cc.getDistSqr(*it);
        if(db < 0 || d < db)
           db = d;
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
    m_K = -1;
    for(i = 0; i < m_cnodes; i++)
        for(n = 0; n < 4; n++)
           if((j = getNeighbor(i, n)) > -1)
            {
                cap = rev_cap = m_lambda * computePrior(i, j);
                if(m_K < 0 || cap > m_K)
                    m_K = cap;
                m_graph->add_edge(m_nodes[i], m_nodes[j], cap, rev_cap);
            }
}

void LazySnapping::getPixelsOnLine(const vector<PixelCoord> coords, vector<PixelCoord> &line)
{
    PixelCoord pa,pb;
    int dx, dy;
    float m, t;
    vector<PixelCoord>::const_iterator it = coords.begin();
    pa = *it;
    for(it++; it != coords.end(); it++)
    {
        line.push_back(pa);
        pb = *it;        
        //DDA line drawing algorithm
        pa.y = m_height - pa.y;
        pb.y = m_height - pb.y;
        dx = pb.x - pa.x;
        dy = pb.y - pa.y; // y coord: m_height - y;
        t = 0.5;
        if(abs(dx) > abs(dy)) {
            m = dy /(float) dx;
            dx = dx < 0 ? -1 : 1;
            m *= dx;
            t += pa.y;
            while(pa.x != pb.x)
            {
                pa.x += dx;
                t += m;
                line.push_back(PixelCoord(pa.x, m_height - t));
            }
        } else if(dy != 0) {
            m = dx /(float) dy;
            dy = dy < 0 ? -1 : 1;
            m *= dy;
            t += pa.x;
            while(pa.y != pb.y)
            {
                pa.y += dy;
                t += m;
                line.push_back(PixelCoord(t, m_height - pa.y));
            }
        }
        //
        pa = pb;
    }
}

void LazySnapping::markPixels(vector<PixelCoord> coords, Label label)
{
    vector<PixelCoord> newcoords;
    for(vector<PixelCoord>::iterator it = coords.begin(); it != coords.end(); it++)
    {
        if(m_seeds.find(*it) == m_seeds.end()) //if the pixel wasn't previously marked
        {
            m_seeds[*it] = label;
            newcoords.push_back(*it);
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
            //markCluster(*it, label); /*pc = findCluster(*it);*/
        }
    }  
    //update cluster
    updateCluster(newcoords, label);

    if(m_fgnd_seeds.empty() || m_bkgnd_seeds.empty()) return;

    //update graph
    updateGraph(newcoords, label);

    //perform segmentation
    segment();
}

void LazySnapping::assignPoints(KMdata &dataPts, const vector<PixelCoord> &seeds)
{
    KMpointArray	pa = dataPts.getPts();
    int i,j;
    i = 0;
    for(vector<PixelCoord>::const_iterator it = seeds.begin(); it != seeds.end(); it++, i++)
    {
        j = it->y * m_width + it->x;
        pa[i][0] = (KMcoord) *(m_data + j * m_depth);
        pa[i][1] = (KMcoord) *(m_data + j * m_depth + 1);
        pa[i][2] = (KMcoord) *(m_data + j * m_depth + 2);
        if(CLUSTERCOORDDIM == 5) {
            pa[i][3] = (KMcoord) it->y;
            pa[i][4] = (KMcoord) it->x;
        }
    } 
}

void LazySnapping::updateCluster(vector<PixelCoord> coords, Label label)
{
    if(label == FGND) {
        copy(coords.begin(), coords.end(), back_insert_iterator<vector<PixelCoord> >(m_fgnd_seeds));
        updateCluster(m_fgnd_seeds, m_fgnd_clusters);
    } else {
        copy(coords.begin(), coords.end(), back_insert_iterator<vector<PixelCoord> >(m_bkgnd_seeds));
        updateCluster(m_bkgnd_seeds, m_bkgnd_clusters);
    }
}

void LazySnapping::updateCluster(std::vector<PixelCoord> &seeds, LazySnapping::tCluster &clusters)
{
    if(seeds.empty()) 
        return;
    KMterm term(100, 0, 0, 0, 0.10, 0.10, 3, 0.50, 10, 0.95);

    KMdata dataPts(CLUSTERCOORDDIM, seeds.size());

    //fillup the points
    assignPoints(dataPts, seeds);
    
    dataPts.buildKcTree();

    KMfilterCenters ctrs(m_nclusters, dataPts);

    KMlocalLloyds   kmAlg(ctrs, term);
    
    ctrs = kmAlg.execute();
    
    //m_cluster_asgn = new int[m_cnodes];
    //m_cluster_sqDist = new double[m_cnodes];
    //ctrs.getAssignments(m_cluster_asgn, m_cluster_sqDist);
    
    clusters.clear();
    
    KMcenterArray pctrs = ctrs.getCtrPts();
    
    for(int i = 0; i < m_nclusters; i++)
    {
        clusters.push_back(
            ClusterCoord<ClusterCoordType,CLUSTERCOORDDIM>(pctrs[i]));
    }
}

void LazySnapping::updateGraph(vector<PixelCoord> coords, Label label)
{
    captype f, b;
    int i, r, c;
    for(vector<PixelCoord>::iterator it = coords.begin(); it != coords.end(); it++)
    {
        i = (*it).y * m_width + (*it).x;
        if(label == FGND) {
            m_graph->set_tweights(m_nodes[i], INF, 0);
        } else {
            m_graph->set_tweights(m_nodes[i], 0, INF);
        }
    }
    
    for(i = 0; i < m_cnodes; i++)
    {
        getRC(i, &r, &c);
        PixelCoord p(c, r);
        if(m_seeds.find(p) == m_seeds.end()) { //if current pixel is not marked as seed
            computeLikelihood(i, f, b);
            m_graph->add_tweights(m_nodes[i], b, f);
        }
    }
}

void LazySnapping::segment()
{
    Graph::flowtype flow = m_graph->maxflow();
    //go through each node and find out the segment it is in
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
    int r, c;
    getRC(node, &r, &c);
    wxMemoryDC dc(*m_mask);
    assert(dc.IsOk());
    wxPen pen = dc.GetPen();
    wxBrush brush = dc.GetBrush();
    if(label == FGND) {
        dc.SetPen(*wxBLACK);
        dc.SetBrush(*wxBLACK);
    }else {
        dc.SetPen(*wxWHITE);
        dc.SetBrush(*wxWHITE);
    }
    dc.DrawPoint(c, r);
    //dc.DrawCircle(c, r, 5);
    dc.SetPen(pen);
    dc.SetBrush(brush);
    assert(dc.IsOk());
}

void LazySnapping::setRegion(vector<PixelCoord> coords, Label label)
{
    wxMemoryDC dc(*m_mask);
    assert(dc.IsOk());
    
    if(label == ISegmentation::FGND)
        dc.SetBrush(*wxBLACK_BRUSH);
    else
        dc.SetBrush(*wxWHITE_BRUSH);
    wxPoint *coords_pt = new wxPoint[coords.size()];
    //copy(coords.begin(), coords.end(), coords_pt);
    transform(coords.begin(), coords.end(), coords_pt, PixelCoordToWxPoint());
    dc.DrawPolygon(coords.size(), coords_pt);
    delete []coords_pt;
}
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

void LazySnapping::setImage(const std::string &imgId, const vigra::BRGBImage* img, vigra::BImage *mask)
{
    m_filename = imgId;
    assert(img);
    m_width = img->width();
    m_height = img->height();
    m_depth = 3;
    m_data = new unsigned char[m_width * m_height * m_depth];
    memcpy(m_data, img->data(), sizeof(unsigned char) * m_width * m_height * m_depth);
    init(mask);    
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
