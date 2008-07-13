// -*- c-basic-offset: 4 -*-
/** @file MaskEdThumbnailPreviewCtrl.cpp
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

#include "MaskEdThumbnailPreviewCtrl.h"

MaskEdThumbnailPreviewCtrl::MaskEdThumbnailPreviewCtrl(wxWindow *parent,
               wxWindowID id,
               const wxString& label,
               const wxString& imgFilename,
               const wxPoint& pos,
               const wxSize& size,
               long style,
               const wxValidator& validator,
               const wxString& name)
               : wxCheckBox(parent, id, label, pos, size, style, validator, name) 
{

}

MaskEdThumbnailPreviewCtrl::~MaskEdThumbnailPreviewCtrl() {}
