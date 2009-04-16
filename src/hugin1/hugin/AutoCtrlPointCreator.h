// -*- c-basic-offset: 4 -*-
/** @file AutoCtrlPointCreator.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#ifndef _AUTOCTRLPOINTCREATOR_H
#define _AUTOCTRLPOINTCREATOR_H

#include <string>
#include <map>

/** Base class for control point creators.
 *
 */
class AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoCtrlPointCreator() {};

    /** dtor.
     */
    virtual ~AutoCtrlPointCreator() {};

    /** Do sift matching, calles the right routines, based
     *  on the matcher selected
     */
    virtual CPVector automatch(PT::Panorama & pano, const PT::UIntSet & imgs,
                           int nFeatures, wxWindow *parent=NULL);

protected:

    CPVector readUpdatedControlPoints(const std::string & file,
                                  PT::Panorama & pano);
private:

};

/** A matcher that uses Sebastians Nowozin's excellent sift matcher */
class AutoPanoSift : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoSift() {};

    /** dtor.
     */
    virtual ~AutoPanoSift() {} ;

    virtual CPVector automatch(PT::Panorama & pano, const PT::UIntSet & imgs,
                           int nFeatures, wxWindow *parent=NULL);

private:


};

/** A matcher that uses Alexandres sift matcher */
class AutoPanoKolor : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoKolor() {};

    /** dtor.
     */
    virtual ~AutoPanoKolor() {} ;

    virtual CPVector automatch(PT::Panorama & pano, const PT::UIntSet & imgs,
                           int nFeatures, wxWindow *parent=NULL);

private:


};



#endif // _AUTOCTRLPOINTCREATOR_H
