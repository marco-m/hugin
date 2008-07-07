/**
 *  @file ControlPointEstimator.h
 *  gsoc2008_feature_matching
 *
 *  This class is used for estimating the RANSAC parameters
 *
 *  @author Onur Kucuktunc <onurcc@gmail.com>
 */

#include "RansacParameterEstimator.h"

class ControlPointEstimator : public RansacParameterEstimator
{
	
protected:
	
public:
	typedef std::vector<double> Param;
	
	// constructor
	ControlPointEstimator ()
		: RansacParameterEstimator(2) {}
	
    /**
	 *	Compute the line parameters defined by a control point.
	 * 
	 * @param data A vector containing a control point.
	 * @param parameters This vector is filled with the computed parameters
	 * @return fitting was succesfull
	 */
    virtual bool estimate(const ControlPoint &data,
                          Param &p) const
    {
        // estimate parameters
		
	}
	
	
};