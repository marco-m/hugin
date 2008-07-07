/**
 *  FeatureMatchingSingleKdTree.cpp
 *  gsoc2008_feature_matching
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "FeatureMatchingSingleKdTree.h"

namespace HuginBase {
	
	// matching function
	CPVector FeatureMatchingSingleKdTree::match(const PanoramaData& pano) {
		
		CPVector allMatches;
		
		// prepare kd tree
		KDTreeKeypointMatcher matcher(5);
		matcher.create(pano);
		
		// keep track of matched points, to avoid re-matching them.
		UIntSet matchedPoints;
		
		ImageKeypoint* onematch;
		// for each image in panorama
		for (int im=0; im < pano.getNrOfImages(); im++)
		{
			// for each keypoint in this image
			const std::vector<Keypoint> & keypoints = pano.getImage(im).getKeypoints();
			for (std::vector<Keypoint>::const_iterator itkey=keypoints.begin();
				 itkey != keypoints.end(); ++itkey)
			{
				
				// find the nearest matching point
				onematch = matcher.match(*itkey, im);
				
				// do not try to match this point, if it has already been matched.
				if (onematch!=NULL && !set_contains(matchedPoints, matcher.getKeypointIdxOfMatch(0)))
				{
					allMatches.push_back(ControlPoint(im, itkey->pos.y, itkey->pos.x,
													  onematch->imageNr, onematch->keypoint.pos.y, onematch->keypoint.pos.x));
					printf("(%d,%f,%f) with (%d,%f,%f)\n",im, itkey->pos.y, itkey->pos.x,
						   onematch->imageNr, onematch->keypoint.pos.y, onematch->keypoint.pos.x);
					
					// mark this controlpoint as matched
					matchedPoints.insert(matcher.getKeypointIdxOfMatch(0));
				}
								
			}
		}
		
		return allMatches;
	}
	
}