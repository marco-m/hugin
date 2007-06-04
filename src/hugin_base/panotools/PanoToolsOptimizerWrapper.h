// -*- c-basic-offset: 4 -*-
/** @file PanoToolsOptimizerWrapper.h
 *
 *  @brief wraps around PTOptimizer
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTOptimise.h 1951 2007-04-15 20:54:49Z dangelo $
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _PTOPTIMISE_H
#define _PTOPTIMISE_H

#include <sstream>

#include "PT/Panorama.h"
#include "PT/PanoToolsInterface.h"
#include "PT/ImageGraph.h"

#include <boost/graph/breadth_first_search.hpp>

namespace PTools
{
    
    /*
     void optimize_PT(const PT::Panorama & pano,
                      const PT::UIntVector &imgs,
                      const PT::OptimizeVector & optvec,
                      PT::VariableMapVector & vars,
                      PT::CPVector & cps,
                      int maxIter=1000);
     */


    /** optimize the images \p imgs, for variables \p optvec, using \p vars
     *  as start. saves the control point distances in \p cps.
     *
     * \param panorama description
     * \param imgs vector with all image numbers that should be used.
     * \param optvect vector of vector of variable names
     * \param cps control points
     * \param progDisplay progress display
     *
     
     */
    void optimize(PT::Panorama & pano,
                  const char * script = 0);



} // namespace




#endif // _PTOPTIMISE_H
