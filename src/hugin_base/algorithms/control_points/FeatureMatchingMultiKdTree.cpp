/**
*  FeatureMatchingMultiKdTree.cpp
 *  gsoc2008_feature_matching
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "FeatureMatchingMultiKdTree.h"

namespace HuginBase {

// matching function
CPVector FeatureMatchingMultiKdTree::match(const PanoramaData& pano) {
	
	CPVector allMatches;
	
	// prepare k-d trees
	KDTreeKeypointMatcher matcher[pano.getNrOfImages()];
	for (int im=0; im < pano.getNrOfImages(); im++)
	{
		UIntSet single_image;
		single_image.insert(im);
		matcher[im] = KDTreeKeypointMatcher(2); // k-d tree with k = 2
		matcher[im].create(pano, single_image);
	}
	
	// keep track of matched points, to avoid re-matching them.
	UIntSet matchedPoints; // TODO: this strategy cannot be applied to multiple case
	
	// for each image in panorama
	ImageKeypoint* onematch;
	for (int im=0; im < pano.getNrOfImages()-1; im++)
	{
		// for each keypoint in this image
		const std::vector<Keypoint> & keypoints = pano.getImage(im).getKeypoints();
		for (std::vector<Keypoint>::const_iterator itkey=keypoints.begin();
			 itkey != keypoints.end(); ++itkey)
		{
			float nearest_keypoint_distance = std::numeric_limits<float>::infinity();
			Keypoint nearest_point;
			int nearest_image = -1;
			
			// find the nearest matching point
			for (int tree=im+1; tree < pano.getNrOfImages(); tree++)
			{
				onematch = matcher[tree].match(*itkey, im);
				
				if (onematch!=NULL)
				{
					float dist = fm_eucdist(*itkey, onematch->keypoint);
					if (dist < nearest_keypoint_distance)
					{
						nearest_keypoint_distance = dist;
						nearest_point = onematch->keypoint;
						nearest_image = onematch->imageNr;
					}
				}
			}
			
			// if a match found within a neighborhood defined by upperbound,
			// add to the matching list
			if (nearest_image != -1) {
				allMatches.push_back(ControlPoint(im, itkey->pos.y, itkey->pos.x,
												  nearest_image, nearest_point.pos.y, nearest_point.pos.x));
				//printf("(%d,%f,%f) with (%d,%f,%f)\n",im, itkey->pos.y, itkey->pos.x,
				//	   nearest_image, nearest_point.pos.y, nearest_point.pos.x);
			}			
		}
	}
	
	return allMatches;
}

}
