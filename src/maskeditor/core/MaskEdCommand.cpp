// -*- c-basic-offset: 4 -*-
/** @file MaskEdCommand.cpp
 *
 *  @author Fahim Mannan <fmannan@gmail.com>
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
#include <vector>
#include "MaskEdCommand.h"

using namespace std;

BrushStrokeCmd::BrushStrokeCmd(MaskMgr *maskmgr, BrushStroke stroke, int index) 
: m_maskmgr(maskmgr), m_stroke(stroke), m_index(index)
{

}

void BrushStrokeCmd::execute()
{
    //store previous state (memento pattern)
    //m_maskmgr->getSegmentation()->getMemento()...
    //...
    m_maskmgr->getSegmentation(m_index)->markPixels(m_stroke.pt, (ISegmentation::Label)m_stroke.label);
}

void BrushStrokeCmd::undo()
{
   //set to previous state (these should be done in the base class)
   //m_maskmgr->getSegmentation()->setState(...)...
}

void BrushStrokeCmd::redo()
{

}

PolygonCmd::PolygonCmd(MaskMgr *maskmgr, MaskPoly poly, int index) 
: m_maskmgr(maskmgr), m_poly(poly), m_index(index)
{}

void PolygonCmd::execute()
{
    m_maskmgr->getSegmentation(m_index)->setRegion(m_poly.pt, (ISegmentation::Label)m_poly.label);
}

void PolygonCmd::undo()
{

}

void PolygonCmd::redo()
{

}
