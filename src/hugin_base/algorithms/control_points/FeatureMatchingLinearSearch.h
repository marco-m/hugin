/**
 *  @file FeatureMatchingLinearSearch.h
 *  gsoc2008_feature_matching
 *
 *  This class implements the straightforward way of matching keypoints in a
 *  given panorama.
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#ifndef _CTRLPNTSALGORITHMS_FEATURE_MATCHING_LINEAR_H
#define _CTRLPNTSALGORITHMS_FEATURE_MATCHING_LINEAR_H

#include <algorithm/PanoramaAlgorithm.h>
#include <panodata/Panorama.h>
#include <cmath>
#include <limits>

namespace HuginBase {

class FeatureMatchingLinearSearch : public PanoramaAlgorithm
{
	
public:
	// constructor and destructor
	FeatureMatchingLinearSearch(PanoramaData& panorama)
		: PanoramaAlgorithm(panorama), upper_bound_for_distance(150.0) {};
	FeatureMatchingLinearSearch(PanoramaData& panorama, float ub)
		: PanoramaAlgorithm(panorama), upper_bound_for_distance(ub) {};
	virtual ~FeatureMatchingLinearSearch() {};
	
	// function to find the control points in the panorama
	virtual bool runAlgorithm()
	{
		o_controlPoints = match(o_panorama, upper_bound_for_distance); //TODO
		return true; // let's hope so.
	}
	
	// getter method for control points
	virtual const CPVector& getControlPoints() const
	{ 
		// [TODO] if(!hasRunSuccessfully()) DEBUG;
		//CPVector v;
		//ControlPoint cp(0, 2263.00, 1830.00, 1, 534.30, 1482.18);
		//o_controlPoints.push_back(cp); // test
		//o_controlPoints = v;
		return o_controlPoints;
	}
	
	// matching function
	virtual CPVector match(const PanoramaData& pano, float upper_bound_for_distance);
	
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
	
protected:
	CPVector o_controlPoints;
	float upper_bound_for_distance;
};

}

#endif // _H