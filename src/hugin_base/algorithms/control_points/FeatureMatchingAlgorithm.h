/**
 *  @file FeatureMatchingAlgorithm.h
 *  gsoc2008_feature_matching
 *
 *  This class is a superclass for feature matching algorithms
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#ifndef _CTRLPNTSALGORITHMS_FEATURE_MATCHING_ALGO_H
#define _CTRLPNTSALGORITHMS_FEATURE_MATCHING_ALGO_H

#include <algorithm/PanoramaAlgorithm.h>
#include <panodata/Panorama.h>

struct FeaturePointMatch
{
	
	FeaturePointMatch(unsigned int _im1, unsigned int _im2, 
					  Keypoint* _k1, Keypoint* _k2)
	{
        im1 = _im1;
        im2 = _im2;
		k1 = _k1;
		k2 = _k2;
	}
	
	// variables
	unsigned int im1, im2;
	Keypoint k1*, k2*;
};

// Euclidean distance calculation for two keypoints
static float eucdist(const Keypoint & p1, const Keypoint & p2)
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
static float eucdist(const Keypoint & p1, const Keypoint & p2, float ub)
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
			return numeric_limits<float>::infinity();
	}
	
	return pow(sum,0.5f);
}

class FeatureMatchingAlgorithm : public PanoramaAlgorithm
{
	
}

typedef std::vector<FeaturePointMatch> FPMVector;


#endif