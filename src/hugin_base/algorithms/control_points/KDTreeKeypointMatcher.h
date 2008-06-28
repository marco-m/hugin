/**
 *  @file KDTreeKeypointMatcher.h
 *  gsoc2008_feature_matching
 *
 *  This class is a generic k-d tree wrapper
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#ifndef _CTRLPNTSALGORITHMS_FEATURE_MATCHING_KDTREE_H
#define _CTRLPNTSALGORITHMS_FEATURE_MATCHING_KDTREE_H

#include <ANN/ANN.h>

struct ImageKeypoint
{
    ImageKeypoint(unsigned int image, unsigned int keyNr, const Keypoint & key)
	{
        imageNr = image;
        keypointNr = keyNr;
        keypoint = key;
	}

	unsigned int imageNr;
	unsigned int keypointNr;
	Keypoint keypoint;
};

class KDTreeKeypointMatcher {
	
public:
	// constructors
	KDTreeKeypointMatcher() 
	: m_KDTree(0), m_allPoints(0), m_k(5)
	{
		m_nnIdx = new ANNidx[m_k];
		m_dists = new ANNdist[m_k];
	};
	KDTreeKeypointMatcher(int k)
		: m_KDTree(0), m_allPoints(0)
	{
			assert(k>0);
			m_k = k;
			m_nnIdx = new ANNidx[m_k];
			m_dists = new ANNdist[m_k];
	};
	
	// destructor
	~KDTreeKeypointMatcher()
	{
		if (m_KDTree)
			delete m_KDTree;
		if (m_allPoints)
			delete[] m_allPoints;
		delete[] m_nnIdx;
		delete[] m_dists;
	}
	
	// initialization methods
	void create(const PanoramaData & pano);
	void create(const PanoramaData & pano, const UIntSet & imgs);
	
	// search methods
	ImageKeypoint match(const Keypoint & key, unsigned int imageOfKeypoint);
	
	// helper methods
	int getKeypointIdxOfMatch(unsigned int matchNr) const;
	
private:
	ANNkd_tree * m_KDTree;
	
	/// vector to hold all descriptors
	vector<ImageKeypoint> m_keypoints;
	
	/// array with pointers to all keypoint descriptors
	ANNpointArray m_allPoints;
	
	// query results
	ANNidxArray m_nnIdx;
	ANNdistArray m_dists;
	
	/// number of nearest neighbours
	unsigned int m_k;
};

#endif