/**
 *  FeatureMatchingSingleKdTree.cpp
 *  gsoc2008_feature_matching
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "FeatureMatchingSingleKdTree.h"

namespace HuginBase {
	
	using namespace std;
	using namespace hugin_utils;
	
	// matching function
	static HuginBase::FPMVector& FeatureMatchingSingleKdTree::match(const PanoramaData& pano) {
		
		FPMVector allMatches;
		
		// prepare kd tree
		KDTreeKeypointMatcher matcher;
		matcher.create(pano);
		
		// keep track of matched points, to avoid re-matching them.
		UIntSet matchedPoints;
		
		// for each image in panorama
		for (int im=0; im < pano.getNrOfImages(); im++)
		{
			// for each keypoint in this image
			const std::vector<Keypoint> & keypoints = pano.getImage(*it).getKeypoints();
			for (vector<Keypoint>::const_iterator itkey=keypoints.begin();
				 itkey != keypoints.end(); ++itkey)
			{
				// do not try to match this point, if it has already been matched.
				if (set_contains(matchedPoints, pnr)) continue;
				
				// find the nearest matching point
				ImageKeypoint onematch = matcher.match(*itkey, *it);
				if (onematch != NULL)
					allMatches.push_back(FeaturePointMatch(im, onematch.imageNr, itkey, onematch.keypoint));
				
				// mark matched points and remove them from further matching.
				for (unsigned j=0; j < matches.size(); j++) {
					// mark controlpoint as matched.
					matchedPoints.insert(matcher.getKeypointIdxOfMatch(j));
				}				
			}
		}
		
		return allMatches;
	}
	
}