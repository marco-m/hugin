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
	
	class FeatureMatchingSingleKdTree : public PanoramaAlgorithm
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
		virtual const CPVector& getControlPoints() const
		{ 
			// [TODO] if(!hasRunSuccessfully()) DEBUG;
			return o_controlPoints;
		}
		
		//
		virtual bool modifiesPanoramaData() const {
			return o_successful;
		}
		
		// matching function
		virtual CPVector match(const PanoramaData& pano);
		
	protected:
		CPVector o_controlPoints;
	};

}


#endif