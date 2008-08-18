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

#include <vigra/stdimage.hxx>

#include <vector>
#include <string>
#include "maxflow/graph.h"
#include "KMlocal/KMlocal.h"
#include "../../core/ISegmentation.h"

const double SIGMA = 2.0f;
const double LAMBDA = 50.0f;
const int    NCLUSTERS = 64;
const int    CLUSTERCOORDDIM = 5; // 3:RGB, 5:RGB + (r,c)

class LazySnappingMemento;

class LazySnapping : public ISegmentation
{
    LazySnappingMemento *m_memento;

    typedef float captype;

    wxBitmap *m_mask;
    //wxBitmap *m_tmp_bmp;         //used for drawing lines and getting all pixel from it
    //HuginBase::ImageCache::ImageCacheRGB8Ptr m_img;
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
    std::vector<PixelCoord> m_fgnd_seeds, m_bkgnd_seeds;

    typedef KMcoord ClusterCoordType;
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
    //typedef std::vector<std::pair<ClusterCoord<ClusterCoordType>, ISegmentation::Label> > tCluster;
    typedef std::vector<ClusterCoord<ClusterCoordType> > tCluster;
    tCluster m_fgnd_clusters, m_bkgnd_clusters;        //centers of clusters
    //int     *m_cluster_asgn;    //datapoint assignment to a cluster
    int      m_nclusters;
    //double  *m_cluster_sqDist;

    void    watershed();
    void    preprocess();
    void    computeLikelihood(int p, captype &f, captype &b);
    captype computePrior(int p, int q);
    void    segment();                      //perform segmentation
    void    setMask(int node, Label label);

    void    updateCluster(vector<PixelCoord> coords, Label label);
    void    updateCluster(vector<PixelCoord> &seeds, tCluster &cluster);
    void    assignPoints(KMdata &dataPts, const std::vector<PixelCoord> &seeds);
    void    getPixelsOnLine(const vector<PixelCoord> coords, vector<PixelCoord> &line);

    void    buildGraph();
    void    updateGraph(std::vector<PixelCoord> coords, Label label);
    int     getNeighbor(int p, int n);
    double  getDistSqrPixel(const PixelCoord &p, const PixelCoord &q) const;
    double  getDistSqrPixel(int p, int q) const;
    double  getDistPixel(int p, int q) const;
    double  getDistSqrPixelIntensity(int p, int q) const;
    int     getIndex(int r, int c) const;
    void    getRC(int p, int *r, int *c) const;
    PixelColor getPixelValue(int p) const;
    PixelColor getPixelValue(int r, int c) const;
    PixelColor getPixelValue(PixelCoord pt) const;
public:
    LazySnapping();    
    LazySnapping(const std::string &filename);
    LazySnapping(const std::string &imgId, const vigra::BRGBImage *img, vigra::BImage *mask);
    ~LazySnapping();

    void init(vigra::BImage *mask = 0);
    void reset();
    std::string name();

    void setMemento(IMaskEdMemento *memento);
    IMaskEdMemento* createMemento();

    void saveMaskMetaData(const std::string &filename);

    void markPixels(std::vector<PixelCoord> coords, Label label);
    void setRegion(std::vector<PixelCoord> coords, Label label);
    void setImage(unsigned char* data, int row, int col, int depth);
    void setImage(const std::string &filename);
    void setImage(const std::string &imgId, const vigra::BRGBImage* img, vigra::BImage *mask = 0);
    std::vector<wxPoint> getBoundingPolygon() const;
    wxMask* getMask() const;
    wxBitmap* getMaskBitmap() const;
    wxBitmap* getImage() const;                 //get working image
    std::vector<wxPoint> getOutline() const;
};
#endif
