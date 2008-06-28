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

class FeatureMatchingLinearSearch : public FeatureMatchingAlgorihtm
{
	
public:
	// constructor and destructor
	FeatureMatchingLinearSearch(PanoramaData& panorama)
		: PanoramaAlgorithm(panorama), upper_bound_for_distance(100d) {};
	FeatureMatchingLinearSearch(PanoramaData& panorama, float ub)
		: PanoramaAlgorithm(panorama), upper_bound_for_distance(ub) {};
	virtual ~FeatureMatchingLinearSearch() {};
	
	// function to find the control points in the panorama
	virtual bool runAlgorithm()
	{
		o_controlPoints = match(o_panorama, upper_bound_for_distance);
		return true; // let's hope so.
	}
	
	// getter method for control points
	virtual const FPMVector & getControlPoints() const
	{ 
		// [TODO] if(!hasRunSuccessfully()) DEBUG;
		return o_controlPoints;
	}
	
	// matching function
	static HuginBase::FPMVector& match(const PanoramaData& pano, float upper_bound_for_distance);
	
protected:
	FPMVector o_controlPoints;
	float upper_bound_for_distance;
};


#endif // _H