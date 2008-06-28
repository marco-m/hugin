/**
 *  FeatureMatchingLinearSearch.cpp
 *  gsoc2008_feature_matching
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "FeatureMatchingLinearSearch.h"

// matching function
static HuginBase::FPMVector& FeatureMatchingLinearSearch::match(const PanoramaData& pano, float upper_bound_for_distance) {
	
	FPMVector allMatches;
	float nearest_keypoint_distance;
	Keypoint* nearest_point;
	int nearest_image;
	
	// for each image in panorama
	for (unsigned int im=0; im < pano.getNrOfImages(); im++)
	{
		// for each keypoint in this image
		const std::vector<Keypoint> & keypoints = pano.getImage(im).getKeypoints();
		for (vector<Keypoint>::const_iterator itkey=keypoints.begin();
			 itkey != keypoints.end(); ++itkey)
		{
			// find the nearest matching point
			nearest_keypoint_distance = numeric_limits<float>::infinity();
			nearest_point = NULL;
			nearest_image = -1;
			
			// iterate over the other images
			for (unsigned int im2=0; im2 < pano.getNrOfImages(); im2++)
			{
				if (im2==im) continue;
				// for each keypoint in this image
				const std::vector<Keypoint> & keypoints2 = pano.getImage(im2).getKeypoints();
				for (vector<Keypoint>::const_iterator itkey2=keypoints2.begin();
					 itkey2 != keypoints2.end(); ++itkey2)
				{
					
					// calculate euclidean distance
					float dist = eucdist(*itkey, *itkey2, upper_bound_for_distance);
					if (dist < nearest_keypoint_distance)
					{
						nearest_keypoint_distance = dist;
						nearest_point = itkey2;
						nearest_image = im2;
					}
				}
			}
			
			// if a match found within a neighborhood defined by upperbound,
			// add to the matching list
			if (nearest_point != NULL)
				allMatches.push_back(FeaturePointMatch(im, im2, itkey, itkey2));
		}
	}
	
	return allMatches;
}
