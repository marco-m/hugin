// -*- c-basic-offset: 4 -*-

/** @file Batch.cpp
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: Batch.cpp 3322 2008-08-18 1:10:07Z mkuder $
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Batch.h"
#include <wx/stdpaths.h>

BEGIN_EVENT_TABLE(Batch, wxFrame)
 EVT_END_PROCESS(-1, Batch::OnProcessTerminate)
END_EVENT_TABLE()

Batch::Batch(wxFrame* parent, wxString path, bool bgui) : wxFrame(parent,wxID_ANY,_T("Batch")){
	//default flag settings
	parallel = false;
	deleteFiles = false;
	shutdown = false;
	overwrite = true;
	verbose = false;
	gui = bgui;
	m_cancelled = false;
	m_paused = false;
	m_running = false;
	m_clearedInProgress = false;
	m_lastFile = wxT("");
	
	// Required to access the preferences of hugin
    //SetAppName(wxT("hugin"));

    // setup the environment for the different operating systems
#if defined __WXMSW__
    wxString huginExeDir = getExePath(path);
	
	wxString huginRoot;
    wxFileName::SplitPath(huginExeDir, &huginRoot, NULL, NULL);
	
	progs = getPTProgramsConfig(huginExeDir, wxConfigBase::Get());
#else
    // add the locale directory specified during configure
    PTPrograms progs = getPTProgramsConfig(wxT(""), wxConfigBase::Get());
#endif

	// update incompatible configuration entries.
    updateHuginConfig(wxConfigBase::Get());
}

void Batch::AddAppToBatch(wxString app)
{
	Project *newApp = new Project(app);
	m_projList.Add(newApp);
}

void Batch::AddProjectToBatch(wxString projectFile, wxString outputFile)
{
	wxFileName projectName(projectFile);
	wxFileName outName(outputFile);
	projectName.Normalize();
	outName.Normalize();
	
	if(outputFile.Cmp(_T(""))!=0)
	{
		Project *proj = new Project(projectName.GetFullPath(),outName.GetFullPath());
		m_projList.Add(proj);
		/*if(gui)
			((wxFrame*)GetParent())->SetStatusText(_T("Added project ")+projectFile+_T(" with output ")+outputFile);
		else if(verbose)
			cout << "Added project " << projectFile.char_str() << " with output " << outputFile.char_str() << endl;*/
	}
	else
	{	//on output set as "", it defaults to same path and name as project file
		Project *proj = new Project(projectName.GetFullPath(),projectName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + projectName.GetName());
		m_projList.Add(proj);
		/*if(gui)
			((wxFrame*)GetParent())->SetStatusText(_T("Added project ")+projectFile);
		else if(verbose)
			cout << "Added project " << projectFile.char_str() << endl;*/
	}
}

bool Batch::AllDone()
{
	for(unsigned int i=0; i<m_projList.GetCount(); i++)
	{
		if(m_projList.Item(i).status==Project::WAITING || 
		m_projList.Item(i).status==Project::RUNNING ||
		m_projList.Item(i).status==Project::PAUSED)
		{
			return false;
		}
	}
	return true;
}

void Batch::AppendBatchFile(wxString file)
{
	wxFileName aFile(file);
	if(aFile.FileExists())
	{
		m_lastFile = file;
		aFile.GetTimes(NULL,NULL,&m_lastmod);
		wxFileInputStream fileStream(file);
		wxString projectName = _T("");
		wxTextInputStream textStream(fileStream);

		//TO-DO: batch file error checking?
		//first line in file is idGenerator, we save it a temp variable, cause it gets set when adding projects
		long idGenTemp = 1;
		textStream.ReadLine().ToLong(&idGenTemp);
		//then for each project: project path, prefix, id, status, skip
		while((projectName = textStream.ReadLine()).Cmp(_(""))!=0)
		{
			//we add project to internal list
			AddProjectToBatch(projectName,textStream.ReadLine());
			textStream.ReadLine().ToLong(&m_projList.Last().id);
			long status;
			textStream.ReadLine().ToLong(&status);
			//if status was RUNNING or PAUSED, we set it to FAILED
			if(status==(long)Project::RUNNING || status==(long)Project::PAUSED)
				status=(long)Project::FAILED;
			m_projList.Last().status = (Project::Status)status;
			if(textStream.ReadLine().StartsWith(_T("T")))
				m_projList.Last().skip = true;
		}
		//we set the id generator we got from file
		Project::idGenerator = idGenTemp;
	}
}

void Batch::CancelBatch()
{
	m_cancelled = true;
	for(int i=0; i<GetRunningCount(); i++)
		CancelProject(i);
	m_running = false;
}
void Batch::CancelProject(int index)
{
	wxCommandEvent event;
	if(GetRunningCount()==1)
		m_paused = false;
	m_stitchFrames.Item(index)->OnCancel(event);
	if(GetRunningCount()==0)
		m_running = false;
}
void Batch::ChangePrefix(int index, wxString newPrefix)
{
	m_projList.Item(index).prefix = newPrefix;
}
bool Batch::CheckProjectExistence() //used only in console version
{	
	#ifdef __WXMSW__  //on windows we run a loop every second to check if running processes are still active
	bool exist = true;
	HANDLE process;
	DWORD exitState;
	while(exist)
	{
		exist = false;
		
		for(unsigned int i=0; i<m_stitchFrames.GetCount(); i++)
		{
			try
			{
				process = OpenProcess(PROCESS_QUERY_INFORMATION,true,m_stitchFrames.Item(i)->GetProcessId());
				GetExitCodeProcess(process,&exitState);
			}
			catch(::exception e)
			{
				exitState=1;
			}
			if(exitState==STILL_ACTIVE)
				exist=true;
			else if(exitState!=0)
				SetStatus(GetIndex(m_stitchFrames.Item(i)->GetProjectId()),Project::FAILED);
			CloseHandle(process);
		}
		wxSleep(1);		
	}
	#else //not __WXMSW__, on Linux we wait for each of the processes to complete
	int status;
	int pid;
	for(unsigned int i=0; i<m_stitchFrames.GetCount(); i++)
	{
		pid = m_stitchFrames.Item(i)->GetProcessId();
		if(waitpid(pid,&status,0)==-1)
			SetStatus(GetIndex(m_stitchFrames.Item(i)->GetProjectId()),Project::FAILED);	//we set to failed if waitpid terminated with an error
		if(!WIFEXITED(status) || WEXITSTATUS(status)!=0)
			SetStatus(GetIndex(m_stitchFrames.Item(i)->GetProjectId()),Project::FAILED);	//we set to failed if child terminated abnormally or with a bad exit code
	}
	#endif

	wxProcessEvent event;
	for(int i=m_stitchFrames.GetCount()-1; i>=0; i--)
	{
		event.SetId(m_stitchFrames.Item(i)->GetProjectId());
		if(GetStatus(GetIndex(m_stitchFrames.Item(i)->GetProjectId()))==Project::FAILED)
			event.SetTimestamp(-1); //a failed exit code cannot be set to a wxWidgets event, so we fake it inside event's timestamp
		OnProcessTerminate(event);
	}
	return true;
}

int Batch::ClearBatch()
{
	if(m_stitchFrames.GetCount()!=0)
	{
		if(gui)
		{
			wxMessageDialog message(this, _("Cannot clear batch in progress.\nDo you want to cancel it?"), _("In progress"), wxYES | wxCANCEL | wxICON_INFORMATION);
			if(message.ShowModal()==wxID_YES)
			{
				CancelBatch();

				//we set a flag so we don't process terminating events
				m_clearedInProgress = true;
				Project::idGenerator=1;
				m_projList.Clear();
				if(gui)
					((wxFrame*)GetParent())->SetStatusText(_("Cleared batch."));
				else if(verbose)
					cout << "Cleared batch." << endl;
				return 2;
			}
		}
		else if(verbose)
			cout << "Error: Cannot clear batch in progress." << endl;
		return 1;
		//TO-DO: return
		/*wxMessageDialog message(this, _T("Cannot clear batch in progress.\nDo you want to cancel it?"), _T("In progress"), wxYES | wxCANCEL | wxICON_INFORMATION);
		if(message.ShowModal()==wxID_YES)
		{
			OnButtonCancel(event);
		}*/
	}
	else
	{
		Project::idGenerator=1;
		m_projList.Clear();
		if(gui)
			((wxFrame*)GetParent())->SetStatusText(_("Cleared batch."));
		else if(verbose)
			cout << "Cleared batch." << endl;
		return 0;
	}
}

bool Batch::CompareProjectsInLists(int stitchListIndex, int batchListIndex)
{
	return m_stitchFrames.Item(stitchListIndex)->GetProjectId() == m_projList.Item(batchListIndex).id;
}

int Batch::GetFirstAvailable()
{
	unsigned int i = 0;
	while(i<m_projList.Count())
	{
		if(m_projList.Item(i).skip || m_projList.Item(i).status!=Project::WAITING)
			i++;
		else
			break;
	}
	if((m_projList.Count() == 0) || (i == m_projList.Count()))
	{
		//no projects are available anymore
		return -1;
	}
	else
	{
		return i;
	}
}

int Batch::GetIndex(int id)
{
	for(unsigned int i=0; i<m_projList.GetCount(); i++)
	{
		if(m_projList.Item(i).id==id)
			return i;
	}
	return -1;
}


Project* Batch::GetProject(int index)
{
	return (Project*)&m_projList.Item(index);
}
int Batch::GetProjectCount()
{
	return m_projList.GetCount();
}
int Batch::GetProjectCountByPath(wxString path)
{
	int count = 0;
	for(unsigned int i=0; i<m_projList.GetCount(); i++)
	{
		if(!m_projList.Item(i).skip && (path.Cmp(m_projList.Item(i).path)==0))
			count++;
	}
	return count;
}


int Batch::GetRunningCount()
{
	return m_stitchFrames.GetCount();
}

Project::Status Batch::GetStatus(int index)
{
	if((unsigned int)index<m_projList.GetCount())
		return m_projList.Item(index).status;
	else if(gui)
		wxMessageBox(wxString::Format(_("Error: Could not get status, project with index %d is not in list."),index),_("Error!"),wxOK | wxICON_INFORMATION );
	else if(verbose)
		cout << "Error: Could not get status, project with index " << index << " is not in list." << endl;
	return Project::MISSING;
}
bool Batch::IsPaused()
{
	return m_paused;
}

void Batch::ListBatch()
{
	if(m_projList.GetCount() == 0)
		cout << "Batch is empty." << endl;
	else
	{
		cout << "List of projects in batch:" << endl <<
				"[ID] [project path] [output filename] [status]" << endl <<
				"-------------------------------------" << endl;
		for(unsigned int i=0; i<m_projList.GetCount(); i++)
		{
			cout << m_projList.Item(i).id << "  "	<< (const char*)m_projList.Item(i).path.char_str()  << "  " << (const char*)m_projList.Item(i).prefix.char_str()
			<< "  " << (const char*)m_projList.Item(i).GetStatusText().char_str() << endl;
		}
	}
}


int Batch::LoadBatchFile(wxString file)
{
	int clearCode = ClearBatch();
	if(clearCode==0)
	{
		AppendBatchFile(file);
		return 0;
	}
	else if(clearCode==2)
	{
		AppendBatchFile(file);
		return 2;
	}
	else if(gui)
		wxMessageBox(_("Error: Could not load batch file."));
	else if(verbose)
		cout << "Error: Could not load batch file." << endl;
	return 1;
}


int Batch::LoadTemp()
{
	wxDir* workingDir = new wxDir(wxStandardPaths::Get().GetUserConfigDir());
	wxString pending;
	wxString fileTemp = _T(".ptbt*");
	wxString temp = _T("");
	//we check for existing temporary files
	if(workingDir->GetFirst(&temp,fileTemp,wxDIR_FILES | wxDIR_HIDDEN))
	{
		//we find the last existing tempfile (there should be at most two, but we check for multiple just in case)
		while(workingDir->GetNext(&pending))
		{
			wxFileName tempFile(temp);
			wxFileName pendingFile(pending);
			wxDateTime* create1 = new wxDateTime();
			wxDateTime* create2 = new wxDateTime();
			if(tempFile.FileExists() && pendingFile.FileExists())
			{
				tempFile.GetTimes(NULL,NULL,create1);
				pendingFile.GetTimes(NULL,NULL,create2);
				if(create2->IsLaterThan(*create1))
				{
					wxRemoveFile(temp);
					temp=wxString(pending);
				}
			}
			else
			{
				//wxMessageBox( _T("Error reading temporary file"),_T("Error!"),wxOK | wxICON_INFORMATION );
				return 1;
			}
		}
	}
	//we load the data from the temp file
	AppendBatchFile(workingDir->GetName()+wxFileName::GetPathSeparator()+temp);
	if(verbose && !gui)
		cout << "Loaded temp file." << endl;
	return 0;
}


bool Batch::NoErrors()
{
	for(unsigned int i=0; i<m_projList.GetCount(); i++)
	{
		if(m_projList.Item(i).status==Project::FAILED)
		{
			return false;
		}
	}
	return true;
}
void Batch::OnProcessTerminate(wxProcessEvent & event)
{
	//we find the right pointer to remove
	unsigned int i = 0;
	while(i < m_stitchFrames.GetCount() &&
	m_stitchFrames.Item(i)->GetProjectId()!=event.GetId())
	{
		i++;
	}
	m_stitchFrames.RemoveAt(i);
	if(m_clearedInProgress)
	{
		if(m_stitchFrames.GetCount()==0)
		{
			m_paused = false;
			m_running = false;
			m_cancelled = false;
			m_clearedInProgress = false;
		}
	}
	else
	{
		if(m_stitchFrames.GetCount()==0)
			m_paused = false;
		i = GetIndex(event.GetId());
		if (event.GetExitCode() != 0 || event.GetTimestamp()==-1) //timestamp is used as a fake exit code because it cannot be set manually
			m_projList.Item(i).status=Project::FAILED;
		else		
			m_projList.Item(i).status=Project::FINISHED;
		if(!m_cancelled && !m_paused)
		{
			if(AllDone())
			{
				m_running = false;
				if(NoErrors())
				{
					if(gui)
						//SetStatusText(_T("Project \"")+m_projList.Item(i).path+_T("\" finished. Batch successfully completed."));
						SetStatusText(_("Batch successfully completed."));
					else
						//cout << "Project \"" << m_projList.Item(i).path.char_str() << "\" finished. Batch successfully completed." << endl;
						cout << "Batch successfully completed." << endl;
				}
				else
				{
					if(gui)
						//SetStatusText(_T("Project \""+m_projList.Item(i).path)+_T("\" finished. Batch completed with errors."));
						SetStatusText(_("Batch completed with errors."));
					else
						//cout << "Project \"" << m_projList.Item(i).path.char_str() << "\" finished. Batch completed with errors." << endl;
						cout << "Batch completed with errors." << endl;
				}
				if(shutdown)	//after we are finished we turn off the computer if checked
				{
					if(gui)
					{
						wxProgressDialog progress(_("Initializing shutdown..."), _("Shutting down..."),49,this,
							wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_CAN_SKIP);
						progress.Fit();
						int i = 0;
						bool skip = false;
						while(progress.Update(i, _("Shutting down..."),&skip))
						{	
							if(skip || i==50)
							{
								/*wxMessageDialog message(this,_T("Krneksa"));
								message.ShowModal();
								break;*/
								wxShutdown(wxSHUTDOWN_POWEROFF);
							}
							i++;
							#if defined __WXMSW__
							Sleep(200);
							#else
							sleep(200);
							#endif
						}
						progress.Close();
					}
					else
					{
						if(!wxShutdown(wxSHUTDOWN_POWEROFF))
						{
							if(gui)
								#ifdef __WXMSW__
									wxMessageBox(_("Error shutting down."));
								#else
									wxMessageBox(_("Error shutting down. Do you have root privileges?"));
								#endif	
							else
								#ifdef __WXMSW__
									cout << "Error shutting down." << endl;
								#else
									cout << "Error shutting down. Do you have root privileges?" << endl;
								#endif	
						}
					}
				}
			}
			else
			{
				if(parallel)	//if we are running in parallel
				{
					//the last executed process in parallel runs next
					if(GetRunningCount() == 0)
					{
						//SetStatusText(_T("Project \""+m_projList.Item(i).path)+_T("\" finished. Running next project..."));
						RunNextInBatch();
					}
					else
					{
						//SetStatusText(_T("Project \""+m_projList.Item(i).path)+_T("\" finished. Waiting for all in parallel to complete..."));
					}
				}
				else
				{
					//SetStatusText(_T("Project \""+m_projList.Item(i).path)+_T("\" finished. Running next project..."));
					RunNextInBatch();
				}
			}
		}
		else
		{	//after all processes have ended on a cancel, we reset the boolean back to false
			//if(stitchFrames.GetCount()==0)
			if(GetRunningCount()==0)
			{
				m_cancelled=false;
			}
		}
	}
}
bool Batch::OnStitch(wxString scriptFile, wxString outname, int id)		//was previously without parameter
{    
    //if( parser.GetParamCount() == 0 && wxIsEmpty(scriptFile)) 
    //{
	if(wxIsEmpty(scriptFile))
	{
        wxString defaultdir = wxConfigBase::Get()->Read(wxT("/actualPath"),wxT(""));
        wxFileDialog dlg(0,
                         _("Specify project source project file"),
                         defaultdir, wxT(""),
                         _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*)|*"),
                         wxOPEN, wxDefaultPosition);

        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK) {
            wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
		
			//scriptFile = dlg.GetPath();
			wxFileDialog dlg2(0,_("Specify output prefix"),
                     wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                     wxT(""), wxT(""),
                     wxSAVE, wxDefaultPosition);
			dlg2.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
			if (dlg2.ShowModal() == wxID_OK) {
				//wxConfig::Get()->Write(wxT("/actualPath"), dlg2.GetDirectory());  // remember for later
				outname = dlg2.GetPath();
			} else { // bail
				wxLogError( _("No output prefix specified"));
				return false;
			}
            scriptFile = dlg.GetPath();
        } else { // bail
            return false;
			wxLogError( _("No project files specified"));
        }
	}

	if(!gui)
		cout << "Stitching with input file " << (const char *)scriptFile.mb_str(wxConvLocal) << "..." << endl;

    // check output filename
    wxFileName outfn(outname);
    wxString ext = outfn.GetExt();
    // remove extension if it indicates an image file
    if (ext.CmpNoCase(wxT("jpg")) == 0 || ext.CmpNoCase(wxT("jpeg")) == 0 || 
        ext.CmpNoCase(wxT("tif")) == 0 || ext.CmpNoCase(wxT("tiff")) == 0 || 
        ext.CmpNoCase(wxT("png")) == 0 || ext.CmpNoCase(wxT("exr")) == 0 ||
        ext.CmpNoCase(wxT("pnm")) == 0 || ext.CmpNoCase(wxT("hdr")) == 0) 
    {
        outfn.ClearExt();
        outname = outfn.GetFullPath();
    }
	
	RunStitchFrame *stitchFrame = new RunStitchFrame(this, wxT("Hugin Stitcher"), wxDefaultPosition, wxSize(640,600)); //,wxGetApp().m_projList, projListBox );
	//stitchFrame->SetEventHandler(this);
	stitchFrame->SetProjectId(id);
	if(verbose && gui)
	{
		stitchFrame->Show( true );
		wxTheApp->SetTopWindow( stitchFrame );
	}
	
    wxFileName basename(scriptFile);
    stitchFrame->SetTitle(wxString::Format(_("%s - Stitching"), basename.GetName().c_str()));
	if(overwrite)
		stitchFrame->m_stitchPanel->SetOverwrite(true);

    bool n = stitchFrame->StitchProject(scriptFile, outname, progs);
	if(n)
		m_stitchFrames.Add(stitchFrame);
	else
		stitchFrame->Close();
    /*if (parser.Found(wxT("d")) ) {
        wxRemoveFile(scriptFile);
    }*/
    return n;
	
}




void Batch::PauseBatch()
{
	if(!m_paused)
	{
		m_paused = true;
		for(int i=0; i<GetRunningCount(); i++)
		{
			m_stitchFrames.Item(i)->m_stitchPanel->PauseStitch();
		}
		for(unsigned int i=0; i<m_projList.GetCount(); i++)
		{
			if(m_projList.Item(i).status==Project::RUNNING)
				m_projList.Item(i).status=Project::PAUSED;
		}
	}
	else
	{
		m_paused = false;
		for(int i=0; i<GetRunningCount(); i++)
		{
			m_stitchFrames.Item(i)->m_stitchPanel->ContinueStitch();
		}
		for(unsigned int i=0; i<m_projList.GetCount(); i++)
		{
			if(m_projList.Item(i).status==Project::PAUSED)
				m_projList.Item(i).status=Project::RUNNING;
		}
	}
}


void Batch::RemoveProject(int id)
{
	int index;
	if((index=GetIndex(id)) != -1)
	{
		RemoveProjectAtIndex(GetIndex(id));
	}
	else if(gui)
		wxMessageBox(wxString::Format(_("Error removing, project with id %d is not in list."),id),_("Error!"),wxOK | wxICON_INFORMATION );
	else if(verbose)
		cout << "Error: Project with id " << id << " is not in list." << endl;
}
void Batch::RemoveProjectAtIndex(int selIndex)
{
	//we delete only successful project files and no applications
	if(deleteFiles
	&& m_projList.Item(selIndex).id>=0 
	&& m_projList.Item(selIndex).status==Project::FINISHED)	
	{
		wxFileName file(m_projList.Item(selIndex).path);
		if(file.FileExists())
		{
			if(!wxRemoveFile(file.GetFullPath()))
			{
				if(gui)
					wxMessageBox( _("Error: Could not delete project file ")+file.GetFullPath(),_("Error!"),wxOK | wxICON_INFORMATION );
				else if(verbose)
					cout << "Error: Could not delete project file " << (const char*)file.GetFullPath().char_str() << endl;
			}
		}
	}
	m_projList.RemoveAt(selIndex);
	if(m_projList.GetCount()==0) //reset the id generator on empty list
		Project::idGenerator=1;
}

void Batch::RunBatch()
{
	if(!gui && m_projList.GetCount() == 0)
		cout << "Batch is empty." << endl;
	else if(!m_running)
	{
		if(gui)
			((wxFrame*)GetParent())->SetStatusText(_("Running batch..."));
		else
			cout << "Running batch..." << endl;
		bool cancelRun = false;
		//check for missing project files
		wxFileName file = wxFileName();
		for(unsigned int i=0; i<m_projList.Count(); i++)
		{
			if(m_projList.Item(i).id >= 0)		//we don't check file existence for command line instructions
			{
				file.Assign(m_projList.Item(i).path);
				if(!file.FileExists())
				{
					/*wxMessageDialog message(this,_T("Project file \"")+m_projList.Item(i).path+_T("\" is missing, project will not be executed."), _T("Project missing"),
						  wxOK | wxCANCEL | wxICON_INFORMATION );
					if(message.ShowModal() != wxID_OK)
					{
						cancelRun = true;
					}*/
					
				}
			}
		}

		//bool value;
		//if we didn't cancel and not all project are marked for skip
		if(!cancelRun)
		{
			m_running = true;
			RunNextInBatch();
		}
	}
	else if(gui)
		((wxFrame*)GetParent())->SetStatusText(_("Batch already in progress."));
}

void Batch::RunNextInBatch()
{
	//((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
	bool value;
	bool repeat = true;
	unsigned int i;
	while(((i=GetFirstAvailable())!=-1) && repeat)
	{
		//execute command line instructions
		if(m_projList.Item(i).id<0)  
		{
			//if(wxFile::Exists(_T("h:\\pictures\\some2.tif")))
			//	wxMessageBox( _T("y"),_T("y"),wxOK | wxICON_INFORMATION );
			if(gui)
				SetStatusText(_("Running command \"")+m_projList.Item(i).path+_T("\""));
			else
				cout << "Running command \"" << (const char*)m_projList.Item(i).path.char_str() << "\"" << endl;
			m_projList.Item(i).status=Project::RUNNING;
			if(!gui)	//we create a fake stitchFrame, so program waits for app to complete
			{
				//cout << "creating stitchframe" << endl;
				RunStitchFrame *stitchFrame = new RunStitchFrame(this, wxT("Hugin Stitcher"), wxDefaultPosition, wxSize(640,600));
				stitchFrame->SetProjectId(m_projList.Item(i).id);
				
				//cout << "adding" << endl;
				repeat = false;
				int pid = wxExecute(m_projList.Item(i).path, wxEXEC_ASYNC);
				stitchFrame->SetProcessId(pid);
				m_stitchFrames.Add(stitchFrame);
				//cout << "pid is " << pid << endl;
			}
			else
			{
				if(wxExecute(m_projList.Item(i).path, wxEXEC_SYNC)==0)
					m_projList.Item(i).status=Project::FINISHED;
				else
					m_projList.Item(i).status=Project::FAILED;
			}
		}
		else
		{
			if(!parallel)	//we run in sequence
			{
				//wxMessageBox( _T("2"),_T("2"),wxOK | wxICON_INFORMATION );
				m_projList.Item(i).status=Project::RUNNING;
				//SetStatusText(_T("Stitching project \"")+m_projList.Item(i).path+_T("\""));
				m_running = true;
				value = OnStitch(m_projList.Item(i).path, m_projList.Item(i).prefix, m_projList.Item(i).id);
				if(!value)
					m_projList.Item(i).status=Project::FAILED;
				else
					repeat = false;
			}
			else
			{
				while((i = GetFirstAvailable())!=-1)
				{
					//projListBox->Select(i); //select the currently processing project
					if(m_projList.Item(i).id<0)
						break;
					m_projList.Item(i).status=Project::RUNNING;
					//SetStatusText(_T("Stitching..."));
					m_running = true;
					value = OnStitch(m_projList.Item(i).path, m_projList.Item(i).prefix, m_projList.Item(i).id);
					if(!value)
						m_projList.Item(i).status=Project::FAILED;
					else
						repeat = false;
				}
				//projListMutex->Unlock();
			}
		}
	}
	if(AllDone())
		m_running = false;
	else if(!gui)
		CheckProjectExistence();
}

void Batch::SaveBatchFile(wxString file)
{
	wxFileOutputStream fileStream(file);
	wxTextOutputStream textStream(fileStream);
	//we write current idGenerator to file
	wxString line = _T("");
	line << Project::idGenerator;
	textStream.WriteString(line+_T("\n"));
	//then for each project: project path, prefix, id, status, skip
	for(unsigned int i = 0; i< m_projList.GetCount(); i++)
	{
		textStream.WriteString(m_projList.Item(i).path+_T("\n"));
		textStream.WriteString(m_projList.Item(i).prefix+_T("\n"));
		line = _T("");
		line << m_projList.Item(i).id;
		textStream.WriteString(line+_T("\n"));
		line = _T("");
		line << m_projList.Item(i).status;
		textStream.WriteString(line+_T("\n"));
		if(m_projList.Item(i).skip)
			textStream.WriteString(_T("T\n"));
		else
			textStream.WriteString(_T("F\n"));
	}
	fileStream.Close();
	m_lastFile = file;
	wxFileName aFile(file);
	aFile.GetTimes(NULL,NULL,&m_lastmod);
}

void Batch::SaveTemp()
{
	//wxMessageBox( _T("saving temp file"),_T(""),wxOK | wxICON_INFORMATION );
	wxDir* workingDir = new wxDir(wxStandardPaths::Get().GetUserConfigDir());
	wxString fileTemp = _T(".ptbt*");
	//we get the old temp file
	fileTemp = workingDir->FindFirst(workingDir->GetName(),fileTemp,wxDIR_FILES | wxDIR_HIDDEN);
	wxFileName oldFile(fileTemp);
	//we alternate between 0 and 1
	wxString suffix;
	if(fileTemp.EndsWith(_T("0")))
		suffix = _T("1");
	else
		suffix = _T("0");
	SaveBatchFile(wxStandardPaths::Get().GetUserConfigDir()+wxFileName::GetPathSeparator()+_T(".ptbt")+suffix);
	if(verbose && !gui)
		cout << "Saved temp file." << endl;
	//we remove the previous temp file
	if(oldFile.FileExists())
		wxRemoveFile(fileTemp);
}


void Batch::SetStatus(int index,Project::Status status)
{
	if((unsigned int)index<m_projList.GetCount())
		m_projList.Item(index).status = status;
	else if(gui)
		wxMessageBox(wxString::Format(_("Error: Could not set status, project with index %d is not in list."),index),_("Error!"),wxOK | wxICON_INFORMATION );
	else if(verbose)
		cout << "Error: Could not set status, project with index " << index << " is not in list." << endl;
}
void Batch::SwapProject(int index)
{
	Project* proj = m_projList.Detach(index+1);
	m_projList.Insert(proj,index);
}
