// -*- c-basic-offset: 4 -*-
/** @file MaskEdPrefDlg.cpp
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
#include <string>
#include <vector>
#include "../core/MaskMgr.h"
#include "MaskEdPrefDlg.h"
#include <wx/xrc/xmlres.h>

using namespace std;

BEGIN_EVENT_TABLE(MaskEdPrefDlg, wxDialog)
    EVT_BUTTON(wxID_OK, MaskEdPrefDlg::OnOk)
END_EVENT_TABLE()
MaskEdPrefDlg::MaskEdPrefDlg(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("MaskEdPrefDlg"));
    vector<string> options = MaskMgr::getInstance()->getSegmentationOptions();
    for(vector<string>::iterator it = options.begin(); it != options.end(); it++)
        XRCCTRL(*this, "m_comboSegChoice", wxComboBox)->Append(wxString(it->c_str(), wxConvUTF8));
    int index = MaskMgr::getInstance()->getSegmentationOptionSelected();
    XRCCTRL(*this, "m_comboSegChoice", wxComboBox)->SetValue(wxString(options[index].c_str(), wxConvUTF8));
}

MaskEdPrefDlg::~MaskEdPrefDlg() {}

void MaskEdPrefDlg::OnOk(wxCommandEvent &event)
{
   m_ichoice = XRCCTRL(*this, "m_comboSegChoice", wxComboBox)->GetSelection();
   MaskMgr::getInstance()->setSegmentationOption(m_ichoice);
   EndModal(0); 
}
