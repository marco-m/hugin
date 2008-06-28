/**
*  FeatureMatchingMultiKdTree.cpp
 *  gsoc2008_feature_matching
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "FeatureMatchingMultiKdTree.h"


// matching function
static HuginBase::FPMVector& FeatureMatchingMultiKdTree::match(const PanoramaData& pano) {
	
	FPMVector allMatches;
	
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
	for (int im=0; im < pano.getNrOfImages(); im++)
	{
		// for each keypoint in this image
		const std::vector<Keypoint> & keypoints = pano.getImage(im).getKeypoints();
		for (vector<Keypoint>::const_iterator itkey=keypoints.begin();
			 itkey != keypoints.end(); ++itkey)
		{
			// do not try to match this point, if it has already been matched.
			
			// find all the nearest matching point
							
			// mark matched points and remove them from further matching.
						
		}
	}
	
	return allMatches;
}
