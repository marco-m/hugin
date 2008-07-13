// -*- c-basic-offset: 4 -*-
/** @file KMeans.h
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

#ifndef KMEANS_H
#define KMEANS_H

#include <boost/random.hpp>
#include <time.h>
#include <fstream>

template <typename T>
class KMeans
{
    int   m_nelems;
    int   m_ndim;
    T    *m_data;
    T    *m_clusters;
    int  *m_asgns;          //cluster assignments
    int   m_nclusters;
    bool  m_bupdated;

    boost::mt19937 rng;
    static const double EPS;
public:
    KMeans(T *data = 0, int nelem = 0, int dim = 1, int nclusters = 1);
    ~KMeans();

    void setData(T *data, int nelem, int dim);
    void doClustering(T **clusters, int **asgn, int nclusters = -1); // if -1 then use the no. of clusters set initially
    void getClusters(T **clusters, int **asgn);
};
/**
 * Utility functions
 *
 */
template<typename T, typename Compare>
T* getExtremas(T* data, int nelem, int ndim)
{
    T* exts = new T[ndim];
    Compare comparator;
    memcpy(exts, data, sizeof(T) * ndim);
    int i,j,n;
    for(i = 1; i < nelem; i++)
        for(j = 0; j < ndim; j++)
        {
            n = i * ndim  + j;
            if(comparator(data[n], exts[j]))
                exts[j] = data[n];
        }
    return exts;
}

template <typename T, typename RetType>
RetType EuclideanDistSqr(T *a, T *b, int dim)
{
    RetType d = 0;
    for(int i = 0; i < dim; i++)
        d += (a[i] - b[i]) * (a[i] - b[i]);
    return d;
}

/**
 * K-means implementation
 */
template<typename T>
const double KMeans<T>::EPS = 1E-04;    

template<typename T>
KMeans<T>::KMeans(T *data, int nelem, int dim, int nclusters) : m_data(data),m_nelems(nelem), m_ndim(dim),  
m_nclusters(nclusters), m_clusters(0), m_asgns(0), m_bupdated(true)
{
    rng.seed(time(0));
}

template<typename T>
KMeans<T>::~KMeans()
{
    delete m_clusters;
}

template<typename T>
void KMeans<T>::setData(T *data, int nelem, int dim) 
{
    m_data = data;
    m_nelems = nelem;
    m_ndim = dim;
    m_bupdated = true;
}

template<typename T>
void KMeans<T>::getClusters(T **clusters, int **asgn)
{
    if(m_bupdated)
        cluster(clusters, asgn);
}

template<typename T>
void KMeans<T>::doClustering(T **clusters, int **asgn, int nclusters)
{
    if(nclusters > 0 && nclusters != m_nclusters) {  //if valid no. of clusters then update
        m_nclusters = nclusters;
        m_bupdated = true;
    }
    if(!m_bupdated) {
        if(*clusters)
            delete *clusters;
        *clusters = new T[m_nclusters * m_ndim];
        memcpy(*clusters, m_clusters, sizeof(T) * m_nclusters * m_ndim);

        if(*asgn)
            delete *asgn;
        *asgn = new int[m_nelems];
        memcpy(*asgn, m_asgns, sizeof(int) * m_nelems);
        return;
    }
    
    m_bupdated = false;
    
    if(m_clusters)
        delete m_clusters;
    m_clusters = new T[m_nclusters * m_ndim];
    
    if(m_asgns)
        delete m_asgns;
    m_asgns = new int[m_nelems];
    memset(m_asgns, 0xff, sizeof(int) * m_nelems); //init to -1

    //cluster computation
    //generate centroids
    T* minvals = getExtremas<T, std::less<T> >(m_data, m_nelems, m_ndim);
    T* maxvals = getExtremas<T, std::greater<T> >(m_data, m_nelems, m_ndim);
    int i, j;
#define KMEANS_LOG
#ifdef KMEANS_LOG
    ofstream fout("data.txt");
    //input data
    for(i = 0; i < m_nelems; i++)
    {
        for(j = 0; j < m_ndim; j++)
        {
            fout << m_data[i*m_ndim + j] << " ";
        }
        fout << endl;
    }
#endif
    
    for(i = 0; i < m_ndim; i++) //for each dimension
    {
        boost::uniform_real<T> unidist(minvals[i], maxvals[i]);
        boost::variate_generator<boost::mt19937&, boost::uniform_real<T> > sampler(rng, unidist);
        for(j = 0; j < m_nclusters; j++) //consider each row j as the centroid of jth cluster and the columns are the dimensions
        {
            m_clusters[j * m_ndim + i] = sampler();
#ifdef KMEANS_LOG
            fout << m_clusters[j * m_ndim + i] << " ";
#endif
        }
    }
#ifdef KMEANS_LOG
    fout << endl;
#endif
    //naive K-means implementation
    //update centroid until convergence
    bool bUpdated = true;
    double *dists = new double[m_nelems]; //distance of ith datapoint from its assigned cluster's center
    T   *newcenters = new T[m_nclusters * m_ndim];
    int *npts = new int[m_nclusters];     //number of points assigned to each cluster
    double d;
    while(bUpdated)
    {   
        bUpdated = false;
        //assign datapoints to clusters
        for(i = 0 ; i < m_nelems; i++)
        {
            for(j = 0; j < m_nclusters; j++)
            {
                d = EuclideanDistSqr<T,double>(m_data + i * m_ndim, m_clusters + j * m_ndim, m_ndim);
                if(m_asgns[i] < 0 || d < dists[i]) {
                    dists[i] = d;
                    m_asgns[i] = j;
                }
            }
        }
        //update cluster centers
        memset(newcenters, 0, sizeof(T) * m_nclusters * m_ndim);
        memset(npts, 0, sizeof(int) * m_nclusters);
        for(i = 0; i < m_nelems; i++)
        {
            newcenters[m_asgns[i]] += dists[i];
            npts[m_asgns[i]]++;
        }
        for(i = 0; i < m_nclusters; i++)
        {
            newcenters[i] /= npts[i];
            if(EuclideanDistSqr<T, double>(newcenters + i * m_ndim, m_clusters + i * m_ndim, m_ndim) > EPS) {
                bUpdated = true;
                memcpy(m_clusters + i * m_ndim, newcenters + i * m_ndim, sizeof(T) * m_ndim);
            }
        }
    }

    if(*clusters)
        delete *clusters;
    *clusters = new T[m_nclusters * m_ndim];
    memcpy(*clusters, m_clusters, sizeof(T) * m_nclusters * m_ndim);

    if(*asgn)
        delete *asgn;
    *asgn = new int[m_nelems];
    memcpy(*asgn, m_asgns, sizeof(int) * m_nelems);
#ifdef KMEANS_LOG
    fout.close();
#endif
}
#endif
