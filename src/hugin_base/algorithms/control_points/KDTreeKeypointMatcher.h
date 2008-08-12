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
#include <panodata/Panorama.h>
#include <limits>

namespace HuginBase {
	
struct ImageKeypoint
{
    ImageKeypoint(unsigned int image, unsigned int keyNr, const HuginBase::Keypoint& key)
	{
        imageNr = image;
        keypointNr = keyNr;
        keypoint = key;
	}

	unsigned int imageNr;
	unsigned int keypointNr;
	HuginBase::Keypoint keypoint;
};

class KDTreeKeypointMatcher {
	
public:
	// constructors
	KDTreeKeypointMatcher() 
	: m_KDTree(0), m_allPoints(0), m_k(2)
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
		printf("Destructor of KDTreeKeypointMatcher called\n");
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
	ImageKeypoint* match(const HuginBase::Keypoint& key, unsigned int imageOfKeypoint);
	
	// helper methods
	int getKeypointIdxOfMatch(unsigned int matchNr) const;
	
	// Euclidean distance calculation for two keypoints
	virtual float fm_eucdist(const HuginBase::Keypoint& p1, const HuginBase::Keypoint& p2)
	{
		float sum = 0;
		std::vector<float>::const_iterator it2 = p2.descriptor.begin();
		
		for(std::vector<float>::const_iterator it1 = p1.descriptor.begin();
			it1 != p1.descriptor.end(); ++it1, ++it2)
		{
			float d = *it1 - *it2;
			d *= d;
			sum += d;
		}
		
		return pow(sum,0.5f);
	}
	
	// Euclidean distance calculation for two keypoints with an upper bound given
	virtual float fm_eucdist_ub(const HuginBase::Keypoint& p1, const HuginBase::Keypoint& p2, float ub)
	{
		float sum = 0;
		std::vector<float>::const_iterator it2 = p2.descriptor.begin();
		float ub2 = ub * ub;
		
		for(std::vector<float>::const_iterator it1 = p1.descriptor.begin();
			it1 != p1.descriptor.end(); ++it1, ++it2)
		{
			float d = *it1 - *it2;
			d *= d;
			sum += d;
			
			// check if the distance exceeds upper bound
			if (sum > ub2)
				return std::numeric_limits<float>::infinity();
		}
		
		return pow(sum,0.5f);
	}
	
private:
	ANNkd_tree * m_KDTree;
	
	/// vector to hold all descriptors
	std::vector<ImageKeypoint> m_keypoints;
	
	/// array with pointers to all keypoint descriptors
	ANNpointArray m_allPoints;
	
	// query results
	ANNidxArray m_nnIdx;
	ANNdistArray m_dists;
	
	/// number of nearest neighbours
	unsigned int m_k;
};

}

#endif