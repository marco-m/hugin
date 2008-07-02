/**
 *  @file KDTreeKeypointMatcher.cpp
 *  gsoc2008_feature_matching
 *
 *  This class is a generic k-d tree wrapper
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "KDTreeKeypointMatcher.h"

namespace HuginBase {
	
/** 
* create KDTree from the feature descriptors stored in the panorama.
*
* @param pano Panorama from which the keypoints are taken.
*/
void KDTreeKeypointMatcher::create(const PanoramaData & pano)
{
	UIntSet imgs;
	// for each image in panorama
	for (int im=0; im < pano.getNrOfImages(); im++)
		imgs.insert(im);
	
	// call the create function with all images
	create(pano, &imgs);
}

/** 
* create KDTree from the feature descriptors stored in the panorama.
*
* @param pano Panorama from which the keypoints are taken.
* @param images Only use keypoints from the specified images.
*/
void KDTreeKeypointMatcher::create(const PanoramaData & pano, const UIntSet & imgs)
{
	int nKeypoints=0;
	int dim=0;
	
	// first, count how many descriptors we have, and the dimension of the space
	for (UIntSet::const_iterator it=imgs.begin(); it != imgs.end(); ++it) {
		const std::vector<Keypoint> & keypoints = pano.getImage(*it).getKeypoints();
		nKeypoints += keypoints.size();
		if (keypoints.size() > 0)
			dim = keypoints[0].descriptor.size();
	}
	assert(dim > 0);
	
	// prepare a big list of points, and insert all points
	if (m_allPoints) delete[] m_allPoints;
	m_allPoints = new ANNpoint[nKeypoints];
	
	ANNpointArray pointsPtr = m_allPoints;
	
	// for all images
	for (UIntSet::iterator it=imgs.begin(); it != imgs.end(); ++it) {
		
		// get all keypoints in this image
		const std::vector<Keypoint> & keypoints = pano.getImage(*it).getKeypoints();
		
		// iterate over the keypoints
		int j=0;
		for (vector<Keypoint>::const_iterator itkey=keypoints.begin();
			 itkey != keypoints.end(); ++itkey) 
		{
			// create an ImageKeypoint
			ImageKeypoint ik(*it, j, *itkey);
			
			// add to m_keypoints list
			m_keypoints.push_back(ik);
			
			// add as an ANNpoint
			ANNpoint *pt = const_cast<float *>(&(*(key.descriptor.begin())));
			*pointsPtr = pt;
			
			pointsPtr++;
			j++;
		}
	}
	
	// options: match distance.
	// create KDTree
	m_KDTree = new ANNkd_tree (m_allPoints,            // the data points
							   nKeypoints,             // number of points
							   dim);                   // dimension of space
}


// match a single keypoint
// TODO: kNN search instead of priority search
// TODO: modify the decision part
ImageKeypoint KDTreeKeypointMatcher::match(const Keypoint & key, unsigned int imageOfKeypoint)
{
	
	int nKeypoints = m_keypoints.size();
	int searchDepth = std::max(200, hugin_utils::roundi(log(nKeypoints)/log(1000)*130));
	DEBUG_DEBUG("search depth: " << searchDepth);
	annMaxPtsVisit(searchDepth);
	
	// perform nearest neighbor matching
	ANNcoord * acord = const_cast<float *>(&(*(key.descriptor.begin())));
	m_KDTree->annkPriSearch(acord,
							m_k,                 // number of near neighbors
							m_nnIdx,             // nearest neighbors (returned)
							m_dists,             // distance (returned)
							0.0);                // error bound
	
	// keep the list of rejected images
	UIntSet rejectedImages;
	
	// iterate over nearest neighbor search results
	for (int i=0; i < m_k-1; i++)
	{
		// skip if this keypoint is in the same image
		if (m_keypoints[m_nnIdx[i]].imageNr == imageOfKeypoint)
			continue;
		
		// skip if we have already rejected this image
		if (set_contains(rejectedImages, m_keypoints[m_nnIdx[i]].imageNr))
			continue;
		
		// check for nearest neighbors using 0.6 rule
		float dist1 = eucdist(key, m_keypoints[m_nnIdx[i]].keypoint);
		float dist2 = eucdist(key, m_keypoints[m_nnIdx[i+1]].keypoint);
		if (dist1/dist2 < 0.6)
		{
			return m_keypoints[m_nnIdx[i]];
		}
		else if (m_keypoints[m_nnIdx[i]].imageNr == m_keypoints[m_nnIdx[i+1]].imageNr) {
			rejectedImages.insert(m_keypoints[m_nnIdx[i]].imageNr);
		}

	}
	
	// All matches are bad. Need to check more nearest neighbours.
	// Be conservative and report no matches
	return NULL;
}

int KDTreeKeypointMatcher::getKeypointIdxOfMatch(unsigned int matchNr) const
{
	assert(matchNr < m_k);
	return (m_nnIdx[matchNr]);
}

}
