// -*- c-basic-offset: 4 -*-
/** @file lazysnapping.h
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
 *
 *  $Id: lazysnapping.h 3138 2008-06-21 03:20:03Z fmannan $
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
#ifndef LAZYSNAPPING_H
#define LAZYSNAPPING_H

#include <vector>
#include <string>
#include "../../core/ISegmentation.h"
#include "maxflow/graph.h"


const double SIGMA = 2.0f;
const double LAMBDA = 1.0f;
const int    NCLUSTERS = 64;
const int    CLUSTERCOORDDIM = 3; // 3:RGB, 5:RGB + (r,c)

class LazySnapping : public ISegmentation
{
    typedef float captype;

    wxBitmap *m_mask;
    HuginBase::ImageCache::ImageCacheRGB8Ptr m_img;
    unsigned char *m_data;
    std::vector<wxPoint> m_outline;
    int m_width, m_height, m_depth;
    Graph::node_id *m_nodes;
    Graph          *m_graph;
    int             m_cnodes;       //number of nodes it can be no. of pixels or nodes after preprocessing
    double          m_lambda, m_sigma;
    captype         m_K;

    typedef std::map<PixelCoord, ISegmentation::Label, cmp_pixel_coord> tSeed;
    typedef std::map<PixelColor, int> tObjColorMap;
    tSeed   m_seeds;
    tObjColorMap m_fgnd, m_bkgnd;

    typedef float ClusterCoordType;
    template<typename T, int CDIM = CLUSTERCOORDDIM>
    struct ClusterCoord
    {
        T coord[CDIM];
        ClusterCoord()
        {
            memset(coord, 0xff, sizeof(T) * CDIM);
        }
        ClusterCoord(T *coord)
        {
            memcpy(this->coord, coord, sizeof(T) * CDIM);
        }
        ClusterCoord(PixelColor clr, PixelCoord pc)
        {
            coord[0] = clr.r;
            coord[1] = clr.g;
            coord[2] = clr.b;
            if(CDIM == 5) {
                coord[3] = pc.y;
                coord[4] = pc.x;
            }
        }
        ClusterCoord(PixelColor clr, T r, T c)
        {
            coord[0] = clr.r;
            coord[1] = clr.g;
            coord[2] = clr.b;
            if(CDIM == 5) {
                coord[3] = r;
                coord[4] = c;
            }
        }
        template<typename Type, int CDIM2>
        friend bool operator<(const ClusterCoord<Type,CDIM> &a, const ClusterCoord<Type,CDIM2> &b);
        bool operator==(const ClusterCoord<T,CDIM> &rhs)
        {
            for(int i = 0; i < CDIM; i++)
                if(coord[i] != rhs.coord[i])
                    return false;
            return true;
        }
        T& operator[](int index)
        {
            if(index >= CDIM)
                throw std::exception();
            return coord[index];
        }
        double getDistSqr(const ClusterCoord<T,CDIM>& b)
        {
            double d = 0;
            for(int i = 0; i < CDIM; i++)
                d += (coord[i] - b.coord[i]) * (coord[i] - b.coord[i]);
            return d;
        }
    };
    //typedef std::map<ClusterCoord<ClusterCoordType>, ISegmentation::Label> tCluster;
    typedef std::vector<std::pair<ClusterCoord<ClusterCoordType>, ISegmentation::Label> > tCluster;
    tCluster m_clusters;        //centers of clusters
    int     *m_cluster_asgn;    //datapoint assignment to a cluster
    int     m_nclusters;

    void    watershed();
    void    preprocess();
    void    computeLikelihood(int p, captype &f, captype &b);
    captype computePrior(int p, int q);
    void    segment();                      //perform segmentation
    void    setMask(int node, Label label);

    //int findCluster(int p);              //finds the cluster that contains pixel p
    //ClusterCoord findCluster(ClusterCoord p);
    void    markCluster(PixelCoord p, Label label); //mark the cluster containing pixel p with label l

    void    buildGraph();
    void    updateGraph(std::vector<PixelCoord> coords, Label label);
    int     getNeighbor(int p, int n);
    double  getDistSqrPixel(const PixelCoord &p, const PixelCoord &q) const;
    double  getDistSqrPixel(int p, int q) const;
    double  getDistPixel(int p, int q) const;
    double  getDistSqrPixelIntensity(int p, int q) const;
    void    getRC(int p, int *r, int *c) const;
    PixelColor getPixelValue(int p) const;
    PixelColor getPixelValue(int r, int c) const;
    PixelColor getPixelValue(wxPoint pt) const;
public:
    LazySnapping();    
    LazySnapping(const std::string &filename);
    ~LazySnapping();

    void init();
    void reset();
    std::string name();

    void markPixels(std::vector<PixelCoord> coords, Label label);
    void setRegion(std::vector<PixelCoord> coords, Label label);
    void setImage(unsigned char* data, int row, int col, int depth);
    void setImage(const std::string &filename);
    std::vector<wxPoint> getBoundingPolygon() const;
    wxMask* getMask() const;
    wxBitmap* getMaskBitmap() const;
    wxBitmap* getImage() const;                 //get working image
    std::vector<wxPoint> getOutline() const;
};
#endif
