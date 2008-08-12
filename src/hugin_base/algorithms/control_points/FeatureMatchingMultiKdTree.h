/**
*  @file FeatureMatchingMultiKdTree.h
 *  gsoc2008_feature_matching
 *
 *  Matching keypoints in a panorama using multiple k-d trees.
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#ifndef _CTRLPNTSALGORITHMS_FEATURE_MATCHING_MULTIKD_H
#define _CTRLPNTSALGORITHMS_FEATURE_MATCHING_MULTIKD_H

#include <algorithm/PanoramaAlgorithm.h>
#include <panodata/Panorama.h>
#include "KDTreeKeypointMatcher.h"
#include <limits>

namespace HuginBase {
	
class FeatureMatchingMultiKdTree : public PanoramaAlgorithm
{
	
public:
	// constructor and destructor
	FeatureMatchingMultiKdTree(PanoramaData& panorama)
	: PanoramaAlgorithm(panorama) {};
	virtual ~FeatureMatchingMultiKdTree() {};
	
	// function to find the control points in the panorama
	virtual bool runAlgorithm()
	{
		o_controlPoints = match(o_panorama);
		return true;
	}
	
	// getter method for control points
	virtual const CPVector& getControlPoints() const
	{ 
		// [TODO] if(!hasRunSuccessfully()) DEBUG;
		return o_controlPoints;
	}
	
	//
	virtual bool modifiesPanoramaData() const {
		return o_successful;
	}
	
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
	
	// matching function
	CPVector match(const PanoramaData& pano);
	
protected:
	CPVector o_controlPoints;
};

}


#endif