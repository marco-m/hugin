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

#include <panodata/Panorama.h>
#include "KDTreeKeypointMatcher.h"

namespace HuginBase {
	
class FeatureMatchingMultiKdTree : public FeatureMatchingAlgorithm
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
		return true; // let's hope so.
	}
	
	// getter method for control points
	virtual const CPVector& getControlPoints() const
	{ 
		// [TODO] if(!hasRunSuccessfully()) DEBUG;
		return o_controlPoints;
	}
	
	// matching function
	static HuginBase::CPVector& match(const PanoramaData& pano);
	
protected:
	CPVector o_controlPoints;
};

}


#endif