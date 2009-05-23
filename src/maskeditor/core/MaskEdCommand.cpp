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
#include "vigra/stdimage.hxx"

#include <vector>
#include "MaskEdCommand.h"
#include "MaskMgr.h"
#include "IMaskEdMemento.h"

#if defined(WIN32) && defined(__WXDEBUG__)
#include <crtdbg.h>
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

using namespace std;

BrushStrokeCmd::BrushStrokeCmd(MaskMgr *maskmgr, BrushStroke stroke, int index) 
: m_maskmgr(maskmgr), m_stroke(stroke), m_index(index)
{

}

void BrushStrokeCmd::execute()
{
    //store previous state (memento pattern)
    m_memento = m_maskmgr->getSegmentation(m_index)->createMemento();
    m_maskmgr->getSegmentation(m_index)->markPixels(m_stroke.pt, (ISegmentation::Label)m_stroke.label);
}

void BrushStrokeCmd::undo()
{
   //set to previous state (these should be done in the base class)
    m_maskmgr->getSegmentation(m_index)->setMemento(m_memento);
}

void BrushStrokeCmd::redo()
{

}

PolygonCmd::PolygonCmd(MaskMgr *maskmgr, MaskPoly poly, int index) 
: m_maskmgr(maskmgr), m_poly(poly), m_index(index)
{}

void PolygonCmd::execute()
{
    m_memento = m_maskmgr->getSegmentation(m_index)->createMemento();
    m_maskmgr->getSegmentation(m_index)->setRegion(m_poly.pt, (ISegmentation::Label)m_poly.label);
}

void PolygonCmd::undo()
{
    m_maskmgr->getSegmentation(m_index)->setMemento(m_memento);
}

void PolygonCmd::redo()
{
    m_maskmgr->getSegmentation(m_index)->setMemento(m_memento);
}

//
PolyVertexAddCmd::PolyVertexAddCmd(MaskPoly *poly, int x, int y) : m_poly(poly), x(x), y(y)
{}

void PolyVertexAddCmd::execute()
{
    m_poly->add(PixelCoord(x, y));
}

void PolyVertexAddCmd::undo()
{
    m_poly->pt.pop_back();
}

void PolyVertexAddCmd::redo()
{
    execute();
}


