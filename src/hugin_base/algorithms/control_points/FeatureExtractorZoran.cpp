/*
 *  FeatureExtractorZoran.cpp
 */

#include "FeatureExtractorZoran.h"

std::vector<HuginBase::Keypoint> FeatureExtractorZoran::extractFeatures(std::string image)
{
    std::vector<HuginBase::Keypoint> result;
	
    APImage im1(image);
	
	im1.open();
    im1.integrate();
	
    HessianDetector hd1(&im1,0, HD_BOX_FILTERS,1);
	cout << "3";
    if(!hd1.detect()) {
        cout << "Detection of points failed!";
        return result;
    }
	
    Descriptor d1(&im1,&hd1);
    d1.setPoints(hd1.getPoints());
    //d.orientate();
    d1.createDescriptors();
	
    // copy descriptors to our data structure
    HuginBase::Keypoint p;
    p.descriptor.resize(d1.getDescriptors()->at(0).size());
    p.laplacianSign = 1;
	
    vector<vector<int> >::iterator iter1 = d1.interestPoints->begin();
    vector<vector<double> >::iterator iterDesc = d1.getDescriptors()->begin();
	
    //int c=0;
    while( iter1 != d1.interestPoints->end()) {
		vector<int > tmp2 = *iter1;
		vector<double> tmp3 = *iterDesc;
		vector<float> tmp4(tmp3.begin(), tmp3.end());
		p.pos = hugin_utils::FDiff2D(tmp2[1], tmp2[0]);
		double r = d1._getMaxima(tmp2[0], tmp2[1])*HD_INIT_KERNEL_SIZE;
		p.scale = 1/(r*r);
		p.descriptor = tmp4;
		result.push_back(p);
		iter1++; iterDesc++;
    }
	
    return result;
}

