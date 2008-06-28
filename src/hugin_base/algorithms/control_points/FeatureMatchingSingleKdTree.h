/**
 *  @file FeatureMatchingSingleKdTree.h
 *  gsoc2008_feature_matching
 *
 *  Matching keypoints in a panorama using single k-d tree.
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#ifndef _CTRLPNTSALGORITHMS_FEATURE_MATCHING_SINGLEKD_H
#define _CTRLPNTSALGORITHMS_FEATURE_MATCHING_SINGLEKD_H

#include <algorithm/PanoramaAlgorithm.h>
#include <panodata/Panorama.h>
#include "KDTreeKeypointMatcher.h"

namespace HuginBase {
	
	class FeatureMatchingSingleKdTree : public FeatureMatchingAlgorithm
	{
		
	public:
		// constructor and destructor
		FeatureMatchingSingleKdTree(PanoramaData& panorama)
			: PanoramaAlgorithm(panorama) {};
		virtual ~FeatureMatchingSingleKdTree() {};
		
		// function to find the control points in the panorama
		virtual bool runAlgorithm()
		{
			o_controlPoints = match(o_panorama);
			return true; // let's hope so.
		}
		
		// getter method for control points
		virtual const FPMVector & getControlPoints() const
		{ 
			// [TODO] if(!hasRunSuccessfully()) DEBUG;
			return o_controlPoints;
		}
		
		// matching function
		static HuginBase::FPMVector& match(const PanoramaData& pano);
		
	protected:
		FPMVector o_controlPoints;
	};

}


#endif