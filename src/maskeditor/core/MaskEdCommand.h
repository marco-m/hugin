// -*- c-basic-offset: 4 -*-
/** @file MaskEdCommand.h
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

#ifndef MASKEDCOMMAND_H
#define MASKEDCOMMAND_H

#include <string>
#include "appbase/Command.h"
#include "appbase/CommandHistory.h"
#include "MaskMgr.h"
#include "BrushStroke.h"
#include "MaskPoly.h"

class BrushStrokeCmd : public AppBase::Command<std::string>
{
    MaskMgr         *m_maskmgr;
    BrushStroke     m_stroke;
    int             m_index;
public:
    BrushStrokeCmd(MaskMgr *maskmgr, BrushStroke stroke, int index);
    void execute();
    void undo();
    void redo();
};

class PolygonCmd : public AppBase::Command<std::string>
{
    MaskPoly    m_poly;
    MaskMgr     *m_maskmgr;
    int         m_index;
public:
    PolygonCmd(MaskMgr *maskmgr, MaskPoly m_poly, int m_index);
    void execute();
    void undo();
    void redo();
};
//
//class BrushStrokeCmdHist : public AppBase::CommandHistory<BrushStrokeCmd>
//{
//
//};
#endif

