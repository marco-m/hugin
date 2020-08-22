// -*- c-basic-offset: 4 -*-
/** @file PreviewCameraTool.cpp
 *
 *  @author T. Modes
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#ifdef _WIN32
#include "wx/msw/wrapwin.h"
#endif
#include "PreviewCameraTool.h"
#include "GLViewer.h"

void PreviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_WHEEL, this);
}

void PreviewCameraTool::ChangeZoomLevel(bool zoomIn, hugin_utils::FDiff2D scrollPos)
{
    VisualizationState*  state = static_cast<VisualizationState*>(helper->GetVisualizationStatePtr());
    if (zoomIn)
    {
        state->SetZoomLevel((state->GetZoomLevel()) * 1.2);
        scrollPos.x = scrollPos.x / state->GetOptions()->getWidth();
        scrollPos.y = scrollPos.y / state->GetOptions()->getHeight();
        state->SetViewingCenter(scrollPos);
    }
    else
    {
        state->SetZoomLevel((state->GetZoomLevel()) / 1.2);
    };
    state->SetDirtyViewport();
    state->ForceRequireRedraw();
    state->Redraw();
}

void PreviewCameraTool::MouseWheelEvent(wxMouseEvent &e)
{
    if (e.GetWheelRotation() != 0 && helper->IsMouseOverPano())
    {
        ChangeZoomLevel(e.GetWheelRotation() > 0, helper->GetMousePanoPosition());
    }
}

