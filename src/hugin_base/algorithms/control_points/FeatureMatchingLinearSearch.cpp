/**
 *  FeatureMatchingLinearSearch.cpp
 *  gsoc2008_feature_matching
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "FeatureMatchingLinearSearch.h"

namespace HuginBase {
	
// matching function
CPVector FeatureMatchingLinearSearch::match(const PanoramaData& pano, float upper_bound_for_distance) {
	
	CPVector allMatches;
	float nearest_keypoint_distance, old_nearest_keypoint_distance;
	Keypoint nearest_point;
	int nearest_image;
	
	// for each image in panorama
	for (unsigned int im=0; im < pano.getNrOfImages()-1; im++)
	{
		// for each keypoint in this image
		const std::vector<Keypoint> & keypoints = pano.getImage(im).getKeypoints();
		for (std::vector<Keypoint>::const_iterator itkey=keypoints.begin();
			 itkey != keypoints.end(); ++itkey)
		{
			// find the nearest matching point
			nearest_keypoint_distance = std::numeric_limits<float>::infinity();
			nearest_image = -1;
			
			// iterate over the other images
			for (unsigned int im2=im+1; im2 < pano.getNrOfImages(); im2++)
			{
				if (im2==im) continue;
				// for each keypoint in this image
				const std::vector<Keypoint> & keypoints2 = pano.getImage(im2).getKeypoints();
				for (std::vector<Keypoint>::const_iterator itkey2=keypoints2.begin();
					 itkey2 != keypoints2.end(); ++itkey2)
				{
					
					// calculate euclidean distance
					//float dist = fm_eucdist_ub(*itkey, *itkey2, upper_bound_for_distance);
					float dist = fm_eucdist(*itkey, *itkey2);
					if (dist < nearest_keypoint_distance)
					{
						// save the distance of second nearest match
						old_nearest_keypoint_distance = nearest_keypoint_distance;
						
						nearest_keypoint_distance = dist;
						nearest_point = *itkey2;
						nearest_image = im2;
					}
				}
			}
			
			// if a good match found, add to the matching list
			if (nearest_image != -1 && 
				nearest_keypoint_distance/old_nearest_keypoint_distance < 0.6) {
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
