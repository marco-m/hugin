// -*- c-basic-offset: 4 -*-

/** @file CPDetectorConfig.cpp
 *
 *  @brief implementation of CPDetectorSetting, CPDetectorConfig and CPDetectorDialog classes, 
 *         which are for storing and changing settings of different CP detectors
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This is free software; you can redistribute it and/or
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

#include "hugin/CPDetectorConfig.h"
#include <config.h>
#include "base_wx/huginConfig.h"
#include "hugin/config_defaults.h"
#include <hugin/CPDetectorConfig_default.h>
#include "hugin/huginApp.h"

#if defined MAC_SELF_CONTAINED_BUNDLE 
  #include "base_wx/platform.h"
  #include <wx/dir.h>
  #include <CoreFoundation/CFBundle.h>
#endif


void CPDetectorConfig::Read(wxConfigBase *config)
{
    settings.Clear();
    int count=config->Read(wxT("/AutoPano/AutoPanoCount"),0l);
    default_generator=config->Read(wxT("/AutoPano/Default"),0l);
    if(count>0)
    {
        for(int i=0;i<count;i++)
            ReadIndex(config, i);
    };
    if(settings.GetCount()==0)
        ResetToDefault();
    if(default_generator>=settings.GetCount())
        default_generator=0;
};

void CPDetectorConfig::ReadIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    if(config->HasGroup(path))
    {
        CPDetectorSetting* gen=new CPDetectorSetting;
        gen->Read(config,path);
        settings.Add(gen);
        if(i==default_generator)
            default_generator=settings.Index(*gen,true);
    };
};

void CPDetectorConfig::Write(wxConfigBase *config)
{
    int count=settings.Count();
    config->Write(wxT("/AutoPano/AutoPanoCount"),count);
    config->Write(wxT("/AutoPano/Default"),(int)default_generator);
    if(count>0)
    {
        for(int i=0;i<count;i++)
            WriteIndex(config, i);
    };
};

void CPDetectorConfig::WriteIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    settings[i].Write(config,path);
};

void CPDetectorConfig::ResetToDefault()
{
    settings.Clear();
    int maxIndex=sizeof(default_cpdetectors)/sizeof(cpdetector_default)-1;
    for(int i=0;i<=maxIndex;i++)
        settings.Add(new CPDetectorSetting(i));
    default_generator=0;
};

void CPDetectorConfig::FillControl(wxControlWithItems *control,bool select_default,bool show_default)
{
    control->Clear();
    for(unsigned int i=0;i<settings.GetCount();i++)
    {
        wxString s=settings[i].GetCPDetectorDesc();
        if(show_default && i==default_generator)
            s=s+wxT(" (")+_("Default")+wxT(")");
        control->Append(s);
    };
    if(select_default)
        control->SetSelection(default_generator);
};

void CPDetectorConfig::Swap(int index)
{
    CPDetectorSetting* setting=settings.Detach(index);
    settings.Insert(setting,index+1);
    if(default_generator==index)
        default_generator=index+1;
    else 
        if(default_generator==index+1)
            default_generator=index;
};

void CPDetectorConfig::SetDefaultGenerator(unsigned int new_default_generator)
{
    if(new_default_generator<GetCount())
        default_generator=new_default_generator;
    else
        default_generator=0;
};

#include <wx/arrimpl.cpp> 
WX_DEFINE_OBJARRAY(ArraySettings);

CPDetectorSetting::CPDetectorSetting(int new_type)
{
    if(new_type>=0)
    {
        int maxIndex=sizeof(default_cpdetectors)/sizeof(cpdetector_default)-1;
        if (new_type<=maxIndex)
        {
            type=default_cpdetectors[new_type].type;
            desc=default_cpdetectors[new_type].desc;
            prog=default_cpdetectors[new_type].prog;
            args=default_cpdetectors[new_type].args;
            prog_matcher=default_cpdetectors[new_type].prog_matcher;
            args_matcher=default_cpdetectors[new_type].args_matcher;
            if(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack)
            {
                prog_stack=default_cpdetectors[new_type].prog_stack;
                args_stack=default_cpdetectors[new_type].args_stack;
            }
            else
            {
                prog_stack=wxEmptyString;
                args_stack=wxEmptyString;
            };
            option=default_cpdetectors[new_type].option;
        };
    }
    else
    {
        type=CPDetector_AutoPanoSift;
        desc=wxEmptyString;
        prog=wxEmptyString;
        args=wxEmptyString;
        prog_matcher=wxEmptyString;
        args_matcher=wxEmptyString;
        prog_stack=wxEmptyString;
        args_stack=wxEmptyString;
        option=true;
    };
    CheckValues();
};

void CPDetectorSetting::CheckValues()
{
    if(type==CPDetector_AutoPano)
    {
        if(!prog_matcher.IsEmpty())
        {
            prog_matcher=wxEmptyString;
            args_matcher=wxEmptyString;
        };
    };
};

void CPDetectorSetting::Read(wxConfigBase *config, wxString path)
{
    type=(CPDetectorType)config->Read(path+wxT("/Type"),default_cpdetectors[0].type);
    desc=config->Read(path+wxT("/Description"),default_cpdetectors[0].desc);
    prog=config->Read(path+wxT("/Program"),default_cpdetectors[0].prog);
    args=config->Read(path+wxT("/Arguments"),default_cpdetectors[0].args);
    prog_matcher=config->Read(path+wxT("/ProgramMatcher"),default_cpdetectors[0].prog_matcher);
    args_matcher=config->Read(path+wxT("/ArgumentsMatcher"),default_cpdetectors[0].args_matcher);
    if(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack)
    {
        prog_stack=config->Read(path+wxT("/ProgramStack"),default_cpdetectors[0].prog_stack);
        args_stack=config->Read(path+wxT("/ArgumentsStack"),default_cpdetectors[0].args_stack);
    }
    else
    {
        prog_stack=wxEmptyString;
        args_stack=wxEmptyString;
    };
    option=config->Read(path+wxT("/Option"),default_cpdetectors[0].option);
    CheckValues();
};

void CPDetectorSetting::Write(wxConfigBase *config, wxString path)
{
    config->Write(path+wxT("/Type"),type);
    config->Write(path+wxT("/Description"),desc);
    config->Write(path+wxT("/Program"),prog);
    config->Write(path+wxT("/Arguments"),args);
    config->Write(path+wxT("/ProgramMatcher"),prog_matcher);
    config->Write(path+wxT("/ArgumentsMatcher"),args_matcher);
    if(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack)
    {
        config->Write(path+wxT("/ProgramStack"),prog_stack);
        config->Write(path+wxT("/ArgumentsStack"),args_stack);
    };
    config->Write(path+wxT("/Option"),option);
};

// dialog for showing settings of one autopano setting

BEGIN_EVENT_TABLE(CPDetectorDialog,wxDialog)
    EVT_BUTTON(wxID_OK, CPDetectorDialog::OnOk)
    EVT_BUTTON(XRCID("prefs_cpdetector_program_select"),CPDetectorDialog::OnSelectPath)
    EVT_BUTTON(XRCID("prefs_cpdetector_program_descriptor_select"),CPDetectorDialog::OnSelectPathDescriptor)
    EVT_BUTTON(XRCID("prefs_cpdetector_program_matcher_select"),CPDetectorDialog::OnSelectPathMatcher)
    EVT_BUTTON(XRCID("prefs_cpdetector_program_stack_select"),CPDetectorDialog::OnSelectPathStack)
    EVT_CHOICE(XRCID("prefs_cpdetector_type"),CPDetectorDialog::OnTypeChange)
    EVT_CHOICEBOOK_PAGE_CHANGING(XRCID("choicebook_steps"),CPDetectorDialog::OnStepChanging)
END_EVENT_TABLE()

CPDetectorDialog::CPDetectorDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("cpdetector_dialog"));
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    //restore frame position and size
    RestoreFramePosition(this,wxT("CPDetectorDialog"));

    m_edit_desc = XRCCTRL(*this, "prefs_cpdetector_desc", wxTextCtrl);
    m_edit_prog = XRCCTRL(*this, "prefs_cpdetector_program", wxTextCtrl);
    m_edit_args = XRCCTRL(*this, "prefs_cpdetector_args", wxTextCtrl);
    m_edit_prog_descriptor = XRCCTRL(*this, "prefs_cpdetector_program_descriptor", wxTextCtrl);
    m_edit_args_descriptor = XRCCTRL(*this, "prefs_cpdetector_args_descriptor", wxTextCtrl);
    m_edit_prog_matcher = XRCCTRL(*this, "prefs_cpdetector_program_matcher", wxTextCtrl);
    m_edit_args_matcher = XRCCTRL(*this, "prefs_cpdetector_args_matcher", wxTextCtrl);
    m_edit_prog_stack = XRCCTRL(*this, "prefs_cpdetector_program_stack", wxTextCtrl);
    m_edit_args_stack = XRCCTRL(*this, "prefs_cpdetector_args_stack", wxTextCtrl);
    m_check_option = XRCCTRL(*this, "prefs_cpdetector_option", wxCheckBox);
    m_cpdetector_type = XRCCTRL(*this, "prefs_cpdetector_type", wxChoice);
    m_choice_step = XRCCTRL(*this, "choicebook_steps", wxChoicebook);
    m_cpdetector_type->SetSelection(1);
    ChangeType();
};

CPDetectorDialog::~CPDetectorDialog()
{
    StoreFramePosition(this,wxT("CPDetectorDialog"));
};

void CPDetectorDialog::OnOk(wxCommandEvent & e)
{
#ifdef __WXMAC__
    if(m_cpdetector_type->GetSelection()==0)
    {
        wxMessageBox(_("Autopano from http://autopano.kolor.com is not available for OSX"), 
                     _("Using Autopano-Sift instead"),wxOK|wxICON_EXCLAMATION, this); 
        m_cpdetector_type->SetSelection(1);
    };
#endif
    bool valid=true;
    valid=valid && (m_edit_desc->GetValue().Trim().Len()>0);
    if(m_choice_step->GetSelection()==0)
    {
        //detector with one step
        valid=valid && (m_edit_prog->GetValue().Trim().Len()>0);
        valid=valid && (m_edit_args->GetValue().Trim().Len()>0);
    }
    else
    {
        //detector with two steps
        valid=valid && (m_edit_prog_descriptor->GetValue().Trim().Len()>0);
        valid=valid && (m_edit_prog_matcher->GetValue().Trim().Len()>0);
        valid=valid && (m_edit_args_descriptor->GetValue().Trim().Len()>0);
        valid=valid && (m_edit_args_matcher->GetValue().Trim().Len()>0);
    };
    int type=m_cpdetector_type->GetSelection();
    if(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack)
        if(m_edit_prog_stack->GetValue().Trim().Len()>0)
            valid=valid && (m_edit_args_stack->GetValue().Trim().Len()>0);
    if(valid)        
        this->EndModal(wxOK);
    else
        wxMessageBox(_("At least one input field is empty.\nPlease check your inputs."),
            _("Warning"),wxOK | wxICON_ERROR,this);
};

void CPDetectorDialog::UpdateFields(CPDetectorConfig* cpdet_config,int index)
{
    m_edit_desc->SetValue(cpdet_config->settings[index].GetCPDetectorDesc());
    //program names and arguments
    if(cpdet_config->settings[index].IsTwoStepDetector())
    {
        m_choice_step->SetSelection(1);
        m_edit_prog_descriptor->SetValue(cpdet_config->settings[index].GetProg());
        m_edit_prog_matcher->SetValue(cpdet_config->settings[index].GetProgMatcher());
        m_edit_args_descriptor->SetValue(cpdet_config->settings[index].GetArgs());
        m_edit_args_matcher->SetValue(cpdet_config->settings[index].GetArgsMatcher());
    }
    else
    {
        m_choice_step->SetSelection(0);
        m_edit_prog->SetValue(cpdet_config->settings[index].GetProg());
        m_edit_args->SetValue(cpdet_config->settings[index].GetArgs());
    };
    int type=cpdet_config->settings[index].GetType();
    if(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack)
    {
        m_edit_prog_stack->SetValue(cpdet_config->settings[index].GetProgStack());
        m_edit_args_stack->SetValue(cpdet_config->settings[index].GetArgsStack());
    };
    m_cpdetector_type->SetSelection(type);
    m_check_option->SetValue(cpdet_config->settings[index].GetOption());
    ChangeType();
};

void CPDetectorDialog::UpdateSettings(CPDetectorConfig* cpdet_config,int index)
{
    cpdet_config->settings[index].SetCPDetectorDesc(m_edit_desc->GetValue().Trim());
    cpdet_config->settings[index].SetType((CPDetectorType)m_cpdetector_type->GetSelection());
    if(m_choice_step->GetSelection()==0)
    {
        cpdet_config->settings[index].SetProg(m_edit_prog->GetValue().Trim());
        cpdet_config->settings[index].SetArgs(m_edit_args->GetValue().Trim());
    }
    else
    {
        cpdet_config->settings[index].SetProg(m_edit_prog_descriptor->GetValue().Trim());
        cpdet_config->settings[index].SetArgs(m_edit_args_descriptor->GetValue().Trim());
        cpdet_config->settings[index].SetProgMatcher(m_edit_prog_matcher->GetValue().Trim());
        cpdet_config->settings[index].SetArgsMatcher(m_edit_args_matcher->GetValue().Trim());
    };
    CPDetectorType type=cpdet_config->settings[index].GetType();
    if(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack)
    {
        cpdet_config->settings[index].SetProgStack(m_edit_prog_stack->GetValue().Trim());
        cpdet_config->settings[index].SetArgsStack(m_edit_args_stack->GetValue().Trim());
    };
    cpdet_config->settings[index].SetOption(m_check_option->IsChecked());
};

void CPDetectorDialog::OnTypeChange(wxCommandEvent &e)
{
    ChangeType();
};

void CPDetectorDialog::ChangeType()
{
    int type=m_cpdetector_type->GetSelection();
    if(type==CPDetector_AutoPano)
    {
        m_choice_step->SetSelection(0);
        twoStepAllowed=false;
    }
    else
        twoStepAllowed=true;
    bool isActive=(type==CPDetector_AutoPanoSiftStack || type==CPDetector_AutoPanoSiftMultiRowStack);
    XRCCTRL(*this,"panel_stack",wxPanel)->Enable(isActive);
    switch(type)
    {
        case CPDetector_AutoPanoSiftMultiRow:
        case CPDetector_AutoPanoSiftMultiRowStack:
            m_check_option->SetLabel(_("Try to connect all overlapping images."));
            m_check_option->Enable(true);
            m_check_option->Show(true);
            XRCCTRL(*this, "prefs_cpdetector_no_option",wxStaticText)->Show(false);
            break;
        case CPDetector_AutoPanoSiftPreAlign:
            m_check_option->SetLabel(_("Only work on image pairs without control points."));
            m_check_option->Enable(true);
            m_check_option->Show(true);
            XRCCTRL(*this, "prefs_cpdetector_no_option",wxStaticText)->Show(false);
            break;
        default:
            XRCCTRL(*this, "prefs_cpdetector_no_option",wxStaticText)->Show(true);
            m_check_option->Enable(false);
            m_check_option->Show(false);
            break;
    };
    m_check_option->GetParent()->Layout();
    Layout();
};


bool CPDetectorDialog::ShowFileDialog(wxString & prog)
{
    wxFileName executable(prog);
#ifdef MAC_SELF_CONTAINED_BUNDLE
    wxString autopanoPath = MacGetPathToUserAppSupportAutoPanoFolder();
#endif
    wxFileDialog dlg(this,_("Select control point detector program"), 
#ifdef MAC_SELF_CONTAINED_BUNDLE
            autopanoPath,
#else
            executable.GetPath(),
#endif
            executable.GetFullName(),
#ifdef __WXMSW__
            _("Executables (*.exe,*.vbs,*.cmd, *.bat)|*.exe;*.vbs;*.cmd;*.bat"),
#else
            wxT(""),
#endif
            wxOPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        prog=dlg.GetPath();
        return true;
    }
    else
        return false;
};

void CPDetectorDialog::OnSelectPath(wxCommandEvent &e)
{
    wxString prog=m_edit_prog->GetValue();
    if (ShowFileDialog(prog))
        m_edit_prog->SetValue(prog);
};

void CPDetectorDialog::OnSelectPathDescriptor(wxCommandEvent &e)
{
    wxString prog=m_edit_prog_descriptor->GetValue();
    if (ShowFileDialog(prog))
        m_edit_prog_descriptor->SetValue(prog);
};

void CPDetectorDialog::OnSelectPathMatcher(wxCommandEvent &e)
{
    wxString prog=m_edit_prog_matcher->GetValue();
    if (ShowFileDialog(prog))
        m_edit_prog_matcher->SetValue(prog);
};

void CPDetectorDialog::OnSelectPathStack(wxCommandEvent &e)
{
    wxString prog=m_edit_prog_stack->GetValue();
    if (ShowFileDialog(prog))
        m_edit_prog_stack->SetValue(prog);
};

void CPDetectorDialog::OnStepChanging(wxChoicebookEvent &e)
{
    if(!twoStepAllowed && e.GetOldSelection()==0)
    {
        wxBell();
        e.Veto();
    };
};