// -*- c-basic-offset: 4 -*-

/** @file RawImport.cpp
 *
 *  @brief implementation of dialog and functions to import RAW images to project file
 *
 *  @author T. Modes
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "hugin/RawImport.h"
#include "panoinc.h"
#include "base_wx/platform.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "hugin/huginApp.h"
#include "base_wx/CommandHistory.h"
#include "base_wx/wxPanoCommand.h"
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include <exiv2/easyaccess.hpp>
#include <exiv2/xmpsidecar.hpp>

/** base class for implementation of Raw import functions */
class RawImport
{
public:
    RawImport(const char* id) :m_exeCtrlXRC(id) {};
    /** image extension of converted files */
    virtual wxString GetImageExtension() { return wxString("tif"); }
    /** reads additional parameters from dialog into class */
    virtual bool ProcessAdditionalParameters(wxDialog* dlg) { return true; }
    /** return true if program supports overwritting output, otherwise false */
    virtual bool SupportsOverwrite() { return true; }
    /** checks if valid executable was given in dialog 
     *  either absolute path or when relative path is given check PATH */
    bool CheckExe(wxDialog* dlg)
    {
        const wxFileName exePath(XRCCTRL(*dlg, m_exeCtrlXRC, wxTextCtrl)->GetValue());
        if (exePath.IsAbsolute())
        {
            if (exePath.FileExists())
            {
                m_exe = exePath.GetFullPath();
                return true;
            }
            else
            {
                wxMessageBox(wxString::Format(_("Executable \"%s\" not found.\nPlease specify a valid executable."), exePath.GetFullPath()),
#ifdef _WIN32
                    _("Hugin"),
#else
                    wxT(""),
#endif
                    wxOK | wxICON_INFORMATION, dlg);
                return false;
            };
        };
        wxPathList pathlist;
#ifdef __WXMSW__
        pathlist.Add(huginApp::Get()->GetUtilsBinDir());
#endif
        pathlist.AddEnvList(wxT("PATH"));
        m_exe = pathlist.FindAbsoluteValidPath(exePath.GetFullPath());
        if (m_exe.IsEmpty())
        {
            wxMessageBox(wxString::Format(_("Executable \"%s\" not found in PATH.\nPlease specify a valid executable."), exePath.GetFullPath()),
#ifdef _WIN32
                _("Hugin"),
#else
                wxT(""),
#endif
                wxOK | wxICON_INFORMATION, dlg);
            return false;
        };
        return true;
    };
    /** return command for processing of reference image */
    virtual HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) = 0;
    /** read output of processing of reference image to read in white balance of reference image */
    virtual bool ProcessReferenceOutput(const wxArrayString& output) = 0;
    /** return commands for processing of all other images with white balance read by RawImport::ProcessReferenceOutput */
    virtual HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) = 0;
    /** add additional PanoCommand::PanoCommand to vector if needed 
     *  hint: we need to pass old and new image number because we can't guarantee that the image order
     *  remains unchanged by PanoCommand::wxAddImagesCmd */
    virtual void AddAdditionalPanoramaCommand(std::vector<PanoCommand::PanoCommand*>& cmds, HuginBase::Panorama* pano, const int oldImageCount, const int addedImageCount) {};
protected:
    wxString m_exe;
private:
    const char* m_exeCtrlXRC;
};

/** special class for raw import with dcraw */
class DCRawImport :public RawImport
{
public:
    DCRawImport() : RawImport("raw_dcraw_exe") {};
    wxString GetImageExtension() override { return wxString("tiff"); }
    bool ProcessAdditionalParameters(wxDialog* dlg) override 
    {
        m_additionalParameters = XRCCTRL(*dlg, "raw_dcraw_parameter", wxTextCtrl)->GetValue().Trim(true).Trim(false);
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxString args(" -w -v -4 -T ");
        if (!m_additionalParameters.IsEmpty())
        {
            args.Append(m_additionalParameters);
            args.Append(" ");
        };
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        args.Empty();
        args.Append("-overwrite_original -tagsfromfile ");
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        args.Append(" -all:all ");
        args.Append(HuginQueue::wxEscapeFilename(imageFilename));
        queue->push_back(new HuginQueue::OptionalCommand(HuginQueue::GetExternalProgram(wxConfig::Get(), huginApp::Get()->GetUtilsBinDir(), "exiftool"), args, wxString::Format(_("Updating EXIF data for %s"), imageFilename)));
        return queue;
    };
    bool ProcessReferenceOutput(const wxArrayString& output) override
    {
        for (auto& s : output)
        {
            int pos = s.Find("multipliers");
            if (pos >= 0)
            {
                m_wb = "-r " + s.Mid(pos + 12);
                return true;
            };
        };
        return false;
    };
    HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxConfigBase* config = wxConfig::Get();
        const wxString binDir = huginApp::Get()->GetUtilsBinDir();
        for (size_t i = 0; i < rawFilenames.size(); ++i)
        {
            wxString args(m_wb);
            args.Append(" -v -4 -T ");
            if (!m_additionalParameters.IsEmpty())
            {
                args.Append(m_additionalParameters);
                args.Append(" ");
            };
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            queue->push_back(new HuginQueue::NormalCommand(m_exe, args,wxString::Format(_("Executing: %s %s"), m_exe, args)));
            args.Empty();
            args.Append("-overwrite_original -tagsfromfile ");
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            args.Append(" -all:all ");
            args.Append(HuginQueue::wxEscapeFilename(imageFilenames[i]));
            queue->push_back(new HuginQueue::OptionalCommand(HuginQueue::GetExternalProgram(config, binDir, "exiftool"), args, wxString::Format(_("Updating EXIF data for %s"), imageFilenames[i])));
        }
        return queue;
    };
    void AddAdditionalPanoramaCommand(std::vector<PanoCommand::PanoCommand*>& cmds, HuginBase::Panorama* pano, const int oldImageCount, const int addedImageCount) override
    {
        // set response to linear for newly added images
        HuginBase::UIntSet imgs;
        fill_set(imgs, oldImageCount, oldImageCount + addedImageCount - 1);
        cmds.push_back(new PanoCommand::ChangeImageResponseTypeCmd(*pano, imgs, HuginBase::SrcPanoImage::RESPONSE_LINEAR));
    };
private:
    wxString m_additionalParameters;
    wxString m_wb;
};

/** class for RawTherapee raw import */
class RTRawImport :public RawImport
{
public:
    RTRawImport() : RawImport("raw_rt_exe") {};
    bool ProcessAdditionalParameters(wxDialog* dlg) override
    {
        m_processingProfile = XRCCTRL(*dlg, "raw_rt_processing_profile", wxTextCtrl)->GetValue().Trim(true).Trim(false);
        if (!m_processingProfile.IsEmpty() && !wxFileName::FileExists(m_processingProfile))
        {
            wxMessageBox(wxString::Format(_("Processing profile \"%s\" not found.\nPlease specify a valid file or leave field empty for default settings."), m_processingProfile),
#ifdef _WIN32
                _("Hugin"),
#else
                wxT(""),
#endif
                wxOK | wxICON_INFORMATION, dlg);
            return false;
        }
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxString args("-O " + HuginQueue::wxEscapeFilename(imageFilename));
        if (m_processingProfile.IsEmpty())
        {
            args.Append(" -d");
        }
        else
        {
            args.Append(" -p " + HuginQueue::wxEscapeFilename(m_processingProfile));
        };
        // apply some special settings, especially disable all crop and rotation settings
        args.Append(" -s -p " + HuginQueue::wxEscapeFilename(
            wxString(std::string(hugin_utils::GetDataDir() + "hugin_rt.pp3").c_str(), HUGIN_CONV_FILENAME)));
        args.Append(" -tz -Y -c ");
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        m_usedProcessingProfile = imageFilename + ".pp3";
        queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        return queue;
    };
    bool ProcessReferenceOutput(const wxArrayString& output) override
    {
        // we need to change the WB setting in the processing profile so the white balance of the reference image
        // is used and not the stored WB from each individual image
        wxFileConfig config(wxEmptyString, wxEmptyString, m_usedProcessingProfile);
        config.Write("/White Balance/Setting", "Custom");
        config.Flush();
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        for (size_t i = 0; i < rawFilenames.size(); ++i)
        {
            wxString args("-o " + HuginQueue::wxEscapeFilename(imageFilenames[i]));
            args.Append(" -p "+HuginQueue::wxEscapeFilename(m_usedProcessingProfile));
            args.Append(" -tz -Y -c ");
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        }
        return queue;
    };
private:
    wxString m_processingProfile;
    wxString m_usedProcessingProfile;
};

/** special class for Darktable raw import */
class DarkTableRawImport : public RawImport
{
public:
    DarkTableRawImport() : RawImport("raw_darktable_exe") {}
    ~DarkTableRawImport()
    {
        // destructor, clean up generated files;
        if (!m_usedSettings.IsEmpty())
        {
            wxRemoveFile(m_usedSettings);
        };
    }
    bool SupportsOverwrite() override { return false; }
    HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxString args(" --verbose ");
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        args.Append(" ");
#ifdef __WXMSW__
        // darktable has problems with Windows paths as output
        // so change separator before
        wxString file(imageFilename);
        file.Replace("\\", "/", true);
        args.Append(HuginQueue::wxEscapeFilename(file));
#else
        args.Append(HuginQueue::wxEscapeFilename(imageFilename));
#endif
        // switch --bpp does not work, so using this strange workaround
        args.Append(" --core --conf plugins/imageio/format/tiff/bpp=16");
        m_refImage = imageFilename;
        queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        return queue;
    };
    bool ProcessReferenceOutput(const wxArrayString& output) override
    {
        // extract XMP package for converted reference image and store in temp
        Exiv2::Image::AutoPtr image;
        try
        {
            image = Exiv2::ImageFactory::open(std::string(m_refImage.mb_str(HUGIN_CONV_FILENAME)));
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Exiv2: Error reading metadata (" << e.what() << ")" << std::endl;
            return false;
        }
        try
        {
            image->readMetadata();
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Caught Exiv2 exception " << e.what() << std::endl;
            return false;
        };
        m_usedSettings = wxFileName::CreateTempFileName("hugdt");
        Exiv2::Image::AutoPtr image2;
        try
        {
            image2 = Exiv2::ImageFactory::create(Exiv2::ImageType::xmp, std::string(m_usedSettings.mb_str(HUGIN_CONV_FILENAME)));
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Exiv2: Error creating temp xmp file (" << e.what() << ")" << std::endl;
            return false;
        }
        image2->setXmpData(image->xmpData());
        image2->writeMetadata();
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        for (size_t i = 0; i < rawFilenames.size(); ++i)
        {
            wxString args("--verbose ");
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            args.Append(" ");
            args.Append(HuginQueue::wxEscapeFilename(m_usedSettings));
            args.Append(" ");
#ifdef __WXMSW__
            // darktable has problems with Windows paths as output
            // so change separator before
            wxString file(imageFilenames[i]);
            file.Replace("\\", "/", true);
            args.Append(HuginQueue::wxEscapeFilename(file));
#else
            args.Append(HuginQueue::wxEscapeFilename(imageFilenames[i]));
#endif
            // switch --bpp does not work, so using this strange workaround
            args.Append(" --core --conf plugins/imageio/format/tiff/bpp=16");
            queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        }
        return queue;
    };
private:
    wxString m_refImage;
    wxString m_usedSettings;
};


/** dialog class for showing progress of raw import */
class RawImportProgress :public wxDialog
{
public:
    RawImportProgress(wxWindow * parent, std::shared_ptr<RawImport>& converter, const wxArrayString& rawImages, const wxArrayString& images, const int refImg) : 
        wxDialog(parent, wxID_ANY, _("Import Raw Images"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
        m_converter(converter), m_rawImages(rawImages), m_images(images), m_refImg(refImg), m_isRunning(false)
    {
        DEBUG_ASSERT(m_rawImages.size() > 0);
        DEBUG_ASSERT(m_rawImages.size() == m_images.size());
        if (m_refImg < 0)
        {
            m_refImg = 0;
        };
        if (m_refImg >= m_rawImages.size())
        {
            m_refImg = 0;
        };
        wxBoxSizer * topsizer = new wxBoxSizer(wxVERTICAL);
        m_progressPanel = new MyExecPanel(this);
        topsizer->Add(m_progressPanel, 1, wxEXPAND | wxALL, 2);

        wxBoxSizer* bottomsizer = new wxBoxSizer(wxHORIZONTAL);
#if wxCHECK_VERSION(3,1,0)
        m_progress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL | wxGA_PROGRESS);
#else
        m_progress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);
#endif
        bottomsizer->Add(m_progress, 1, wxEXPAND | wxALL, 10);
        m_cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));
        bottomsizer->Add(m_cancelButton, 0, wxALL | wxALIGN_RIGHT, 10);
        m_cancelButton->Bind(wxEVT_BUTTON, &RawImportProgress::OnCancel, this);
        topsizer->Add(bottomsizer, 0, wxEXPAND);
#ifdef __WXMSW__
        // wxFrame does have a strange background color on Windows..
        this->SetBackgroundColour(m_progressPanel->GetBackgroundColour());
#endif
        SetSizer(topsizer);
        RestoreFramePosition(this, "RawImportProgress");
        Bind(EVT_QUEUE_PROGRESS, &RawImportProgress::OnProgress, this);
        Bind(wxEVT_INIT_DIALOG, &RawImportProgress::OnInitDialog, this);
    };
    virtual ~RawImportProgress()
    {
        StoreFramePosition(this, "RawImportProgress");
    }

protected:
    void OnProcessReferenceTerminate(wxProcessEvent& e) 
    {
        if (e.GetExitCode() == 0)
        {
            if (!m_converter->ProcessReferenceOutput(m_progressPanel->GetLogAsArrayString()))
            {
                wxMessageBox(_("Could not process of output of reference image.\nCan't process further images."),
#ifdef _WIN32
                    _("Hugin"),
#else
                    wxT(""),
#endif
                    wxOK | wxICON_INFORMATION, this);

            };
            Unbind(wxEVT_END_PROCESS, &RawImportProgress::OnProcessReferenceTerminate, this);
            m_progress->SetValue(hugin_utils::roundi(1.0 / m_images.size()));
            wxArrayString rawFiles(m_rawImages);
            rawFiles.RemoveAt(m_refImg);
            if (rawFiles.IsEmpty())
            {
                EndModal(wxID_OK);
            };
            wxArrayString imgFiles(m_images);
            imgFiles.RemoveAt(m_refImg);
            Bind(wxEVT_END_PROCESS, &RawImportProgress::OnProcessTerminate, this);
            m_progressPanel->ExecQueue(m_converter->GetCmdQueueForImport(rawFiles, imgFiles));
        }
        else
        {
            m_isRunning = false;
            m_cancelButton->SetLabel(_("Close"));
        };
    };
    void OnProcessTerminate(wxProcessEvent& e)
    {
        if (e.GetExitCode() == 0)
        {
            EndModal(wxID_OK);
        }
        else
        {
            m_isRunning = false;
            m_cancelButton->SetLabel(_("Close"));
        };
    };
    void OnProgress(wxCommandEvent& event)
    {
        if (event.GetInt() >= 0)
        {
            m_progress->SetValue(hugin_utils::roundi((1 + event.GetInt() / 100.0f * (m_images.size() - 2)) * 100.0f / m_images.size()));
        };
    };
    void OnCancel(wxCommandEvent & event)
    {
        if (m_isRunning)
        {
            m_progressPanel->KillProcess();
            m_isRunning = false;
        }
        else
        {
            EndModal(wxID_CANCEL);
        };
    };
    void OnInitDialog(wxInitDialogEvent& e)
    {
        // start processing reference image
        Bind(wxEVT_END_PROCESS, &RawImportProgress::OnProcessReferenceTerminate, this);
        m_progressPanel->ExecQueue(m_converter->GetCmdQueueForReference(m_rawImages[m_refImg], m_images[m_refImg]));
    }

private:
    std::shared_ptr<RawImport> m_converter;
    wxArrayString m_rawImages, m_images;
    int m_refImg;
    bool m_isRunning;
    wxGauge* m_progress;
    wxButton* m_cancelButton;
    MyExecPanel * m_progressPanel;
};

BEGIN_EVENT_TABLE(RawImportDialog, wxDialog)
    EVT_BUTTON(XRCID("raw_add_images"), RawImportDialog::OnAddImages)
    EVT_BUTTON(XRCID("raw_remove_image"), RawImportDialog::OnRemoveImage)
    EVT_BUTTON(XRCID("raw_set_wb_ref"), RawImportDialog::OnSetWBReference)
    EVT_BUTTON(XRCID("raw_dcraw_exe_select"), RawImportDialog::OnSelectDCRAWExe)
    EVT_BUTTON(XRCID("raw_rt_exe_select"), RawImportDialog::OnSelectRTExe)
    EVT_BUTTON(XRCID("raw_rt_processing_profile_select"), RawImportDialog::OnSelectRTProcessingProfile)
    EVT_BUTTON(XRCID("raw_darktable_exe_select"), RawImportDialog::OnSelectDarktableExe)
    EVT_BUTTON(wxID_OK, RawImportDialog::OnOk)
    EVT_INIT_DIALOG(RawImportDialog::OnInitDialog)
END_EVENT_TABLE()

#ifdef __WXMSW__
#define DEFAULT_DCRAW_EXE "dcraw.exe"
#define DEFAULT_RAWTHERAPEE_EXE "rawtherapee-cli.exe"
#define DEFAULT_DARKTABLE_EXE "darktable-cli.exe"
#else
#define DEFAULT_DCRAW_EXE "dcraw"
#define DEFAULT_RAWTHERAPEE_EXE "rawtherapee-cli"
#define DEFAULT_DARKTABLE_EXE "darktable-cli"
#endif

RawImportDialog::RawImportDialog(wxWindow *parent, HuginBase::Panorama* pano)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("import_raw_dialog"));
    SetMinSize(wxSize(400, 300));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    RestoreFramePosition(this, "RawImportDialog");
    wxConfigBase* config = wxConfig::Get();
    const long splitterPos = config->Read("/RawImportDialog/SplitterPos", 200);
    XRCCTRL(*this, "raw_splitter_window", wxSplitterWindow)->SetSashPosition(splitterPos);
    // dcraw
    wxString s = config->Read("/RawImportDialog/dcrawExe", DEFAULT_DCRAW_EXE);
    wxTextCtrl* ctrl = XRCCTRL(*this, "raw_dcraw_exe", wxTextCtrl);
    ctrl->SetValue(s);
    ctrl->AutoCompleteFileNames();
    s = config->Read("/RawImportDialog/dcrawParameter", "");
    ctrl = XRCCTRL(*this, "raw_dcraw_parameter", wxTextCtrl);
    ctrl->SetValue(s);
    // RawTherapee
    s = config->Read("/RawImportDialog/RTExe", DEFAULT_RAWTHERAPEE_EXE);
    ctrl = XRCCTRL(*this, "raw_rt_exe", wxTextCtrl);
    ctrl->SetValue(s);
    ctrl->AutoCompleteFileNames();
    // RT processing profile
    s = config->Read("/RawImportDialog/RTProcessingProfile", "");
    ctrl = XRCCTRL(*this, "raw_rt_processing_profile", wxTextCtrl);
    ctrl->SetValue(s);
    ctrl->AutoCompleteFileNames();
    // Darktable
    s = config->Read("/RawImportDialog/DarktableExe", DEFAULT_DARKTABLE_EXE);
    ctrl = XRCCTRL(*this, "raw_darktable_exe", wxTextCtrl);
    ctrl->SetValue(s);
    ctrl->AutoCompleteFileNames();
    XRCCTRL(*this, "raw_converter_notebook", wxNotebook)->SetSelection(config->Read("/RawImportDialog/Converter", 0l));
    m_pano=pano;
};

RawImportDialog::~RawImportDialog()
{
    wxConfigBase* config = wxConfig::Get();
    config->Write("/RawImportDialog/SplitterPos", XRCCTRL(*this, "raw_splitter_window", wxSplitterWindow)->GetSashPosition());
    StoreFramePosition(this, "RawImportDialog");
    config->Flush();
};

void RawImportDialog::OnOk(wxCommandEvent & e)
{
    if (m_rawImages.IsEmpty())
    {
        // no image in list
        wxMessageBox(_("Please add at least one raw image to list before you can import it."),
#ifdef _WIN32
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK | wxICON_INFORMATION, this);
        return;
    };
    std::shared_ptr<RawImport> rawConverter;
    switch (XRCCTRL(*this, "raw_converter_notebook", wxNotebook)->GetSelection())
    {
        case 0:
            rawConverter = std::make_shared<DCRawImport>();
            break;
        case 1:
            rawConverter = std::make_shared<RTRawImport>();
            break;
        case 2:
            rawConverter = std::make_shared<DarkTableRawImport>();
            break;
        default:
            // this should not happen
            wxBell();
            return;
    }
    // check if given program is available
    if (!rawConverter->CheckExe(this))
    {
        return;
    }
    // check additional parameters
    if (!rawConverter->ProcessAdditionalParameters(this))
    {
        return;
    };
    {
        // check if image files already exists
        m_images.clear();
        wxArrayString existingImages;
        for (auto& img : m_rawImages)
        {
            wxFileName newImage(img);
            newImage.SetExt(rawConverter->GetImageExtension());
            m_images.push_back(newImage.GetFullPath());
            if (newImage.FileExists())
            {
                existingImages.push_back(newImage.GetFullPath());
            };
        };
        if (!existingImages.IsEmpty())
        {
            wxDialog dlg;
            wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_warning_overwrite"));
            XRCCTRL(dlg, "dlg_overwrite_list", wxListBox)->Append(existingImages);
            dlg.Fit();
            dlg.CenterOnScreen();
            if (dlg.ShowModal() != wxID_OK)
            {
                return;
            };
        };
        if (!rawConverter->SupportsOverwrite())
        {
            // raw converter does not support overwritting
            // so delete the file explicit before
            for (auto& img : existingImages)
            {
                wxRemoveFile(img);
            };
        };
    };
    RawImportProgress dlg(this, rawConverter, m_rawImages, m_images, m_refImg);
    if (dlg.ShowModal() == wxID_OK)
    {
        // save settings for next call
        wxConfigBase* config = wxConfig::Get();
        config->Write("/RawImportDialog/Converter", XRCCTRL(*this, "raw_converter_notebook", wxNotebook)->GetSelection());
        config->Write("/RawImportDialog/dcrawExe", XRCCTRL(*this, "raw_dcraw_exe", wxTextCtrl)->GetValue());
        config->Write("/RawImportDialog/dcrawParameter", XRCCTRL(*this, "raw_dcraw_parameter", wxTextCtrl)->GetValue().Trim(true).Trim(false));
        config->Write("/RawImportDialog/RTExe", XRCCTRL(*this, "raw_rt_exe", wxTextCtrl)->GetValue());
        config->Write("/RawImportDialog/RTProcessingProfile", XRCCTRL(*this, "raw_rt_processing_profile", wxTextCtrl)->GetValue());
        config->Write("/RawImportDialog/DarktableExe", XRCCTRL(*this, "raw_darktable_exe", wxTextCtrl)->GetValue());
        config->Flush();
        // check if all files were generated
        std::vector<std::string> files;
        bool missingFiles = false;
        for (auto& img:m_images)
        {
            if (wxFileName::FileExists(img))
            {
                files.push_back(std::string(img.mb_str(HUGIN_CONV_FILENAME)));
            }
            else
            {
                missingFiles = true;
            };
        };
        if (missingFiles || files.empty())
        {
            wxMessageBox(_("At least one raw images was not successfully converted.\nThis image(s) will be skipped"),
#ifdef _WIN32
                _("Hugin"),
#else
                wxT(""),
#endif
                wxOK | wxICON_INFORMATION, this);
        };
        if (files.empty())
        {
            return;
        };
        // now build PanoCommand and add it the GlobalCmdHist
        std::vector<PanoCommand::PanoCommand*> cmds;
        cmds.push_back(new PanoCommand::wxAddImagesCmd(*m_pano, files));
        rawConverter->AddAdditionalPanoramaCommand(cmds, m_pano, m_pano->getNrOfImages(), files.size());
        PanoCommand::CombinedPanoCommand* combinedCmd = new PanoCommand::CombinedPanoCommand(*m_pano, cmds);
        combinedCmd->setName("import raw images");
        PanoCommand::GlobalCmdHist::getInstance().addCommand(combinedCmd);
        EndModal(wxID_OK);
    };
    EndModal(wxID_CANCEL);
}

void RawImportDialog::OnAddImages(wxCommandEvent & e)
{
    wxConfigBase* config = wxConfigBase::Get();
    wxString path = config->Read(wxT("/actualPath"), wxT(""));
    // build filter for raw files
    wxString rawFilter("*.DNG;*.CRW;*.CR2;*.CR3;*.RAW;*.ERF;*.RAF;*.MRW;*.NEF;*.ORF;*.RW2;*.PEF;*.SRW;*.ARW");
    if (wxFileName::IsCaseSensitive())
    {
        // if file system is case sensitive add also lowercase variant
        rawFilter.Append(";").Append(rawFilter.Lower());
    };
    wxFileDialog dlg(this, _("Import Raw Files"),
        path, wxT(""),
        wxString(_("Raw files")).Append("|").Append(rawFilter).Append("|").Append(_("All files (*)")).Append("|*"),
        wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST , wxDefaultPosition);
    dlg.SetDirectory(path);

    wxArrayString invalidFilenames;
    wxArrayString errorReadingFile;
    wxArrayString differentCam;
    // call the file dialog
    if (dlg.ShowModal() == wxID_OK)
    {
        // get the selections
        wxArrayString Pathnames;
        dlg.GetPaths(Pathnames);

        // remember path for later
#ifdef __WXGTK__
        //workaround a bug in GTK, see https://bugzilla.redhat.com/show_bug.cgi?id=849692 and http://trac.wxwidgets.org/ticket/14525
        config->Write(wxT("/actualPath"), wxPathOnly(Pathnames[0]));
#else
        config->Write(wxT("/actualPath"), dlg.GetDirectory());
#endif
        for (auto& file : Pathnames)
        {
            // check filenames for invalid characters
            if (containsInvalidCharacters(file))
            {
                invalidFilenames.push_back(file);
                continue;
            };
            // check that all images are from the same camera
            Exiv2::Image::AutoPtr image;
            try
            {
                image = Exiv2::ImageFactory::open(std::string(file.mb_str(HUGIN_CONV_FILENAME)));
            }
            catch (const Exiv2::Error& e)
            {
                std::cerr << "Exiv2: Error reading metadata (" << e.what() << ")" << std::endl;
                errorReadingFile.push_back(file);
                continue;
            }

            try
            {
                image->readMetadata();
            }
            catch (const Exiv2::Error& e)
            {
                std::cerr << "Caught Exiv2 exception '" << e.what() << "' for file " << file << std::endl;
                errorReadingFile.push_back(file);
            };
            Exiv2::ExifData &exifData = image->exifData();
            if (exifData.empty())
            {
                errorReadingFile.push_back(file);
                continue;
            };
            std::string cam;
            auto make = Exiv2::make(exifData);
            if (make != exifData.end() && make->count())
            {
                cam = make->toString();
            };
            auto model = Exiv2::model(exifData);
            if (model != exifData.end() && model->count())
            {
                cam += "|" + model->toString();
            };
            if (m_camera.empty())
            {
                m_camera = cam;
            }
            else
            {
                if (cam != m_camera)
                {
                    differentCam.push_back(file);
                    continue;
                };
            };
            // prevent duplicates
            if (m_rawImages.Index(file) == wxNOT_FOUND)
            {
                m_rawImages.Add(file);
            };
        };
        // update reference WB image if needed
        if (m_refImg == -1 && !m_rawImages.IsEmpty())
        {
            m_refImg = 0;
        };
        FillImageList();
        if (!invalidFilenames.IsEmpty())
        {
            ShowFilenameWarning(this, invalidFilenames);
        }
        if (!errorReadingFile.IsEmpty())
        {
            wxDialog dlg;
            wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_warning_filename"));
            dlg.SetLabel(_("Warning: Read error"));
            XRCCTRL(dlg, "dlg_warning_text", wxStaticText)->SetLabel(_("The following files will be skipped because the metadata of these files could not read."));
            XRCCTRL(dlg, "dlg_warning_list", wxListBox)->Append(errorReadingFile);
            dlg.Fit();
            dlg.CenterOnScreen();
            dlg.ShowModal();
        };
        if (!differentCam.IsEmpty())
        {
            wxDialog dlg;
            wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_warning_filename"));
            dlg.SetLabel(_("Warning: raw images from different cameras"));
            XRCCTRL(dlg, "dlg_warning_text", wxStaticText)->SetLabel(_("The following images were shot with different camera than the other on.\nThe raw import works only for images from the same cam."));
            XRCCTRL(dlg, "dlg_warning_list", wxListBox)->Append(differentCam);
            dlg.Fit();
            dlg.CenterOnScreen();
            dlg.ShowModal();
        };
    };
}

void RawImportDialog::OnRemoveImage(wxCommandEvent & e)
{
    const int selection = XRCCTRL(*this, "raw_images_list", wxListBox)->GetSelection();
    if (selection != wxNOT_FOUND)
    {
        m_rawImages.RemoveAt(selection);
        if (m_refImg >= m_rawImages.size())
        {
            m_refImg = 0;
        };
        if (m_rawImages.IsEmpty())
        {
            m_refImg = -1;
            m_camera.clear();
        };
        FillImageList();
    }
    else
    {
        wxBell();
    };
}

void RawImportDialog::OnSetWBReference(wxCommandEvent & e)
{
    if (m_rawImages.IsEmpty())
    {
        wxBell();
    }
    else
    {
        m_refImg = XRCCTRL(*this, "raw_images_list", wxListBox)->GetSelection();
        FillImageList();
    }
}

void RawImportDialog::OnInitDialog(wxInitDialogEvent & e)
{
    wxCommandEvent* event = new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, XRCID("raw_add_images"));
    GetEventHandler()->QueueEvent(event);
}

void RawImportDialog::OnSelectDCRAWExe(wxCommandEvent & e)
{
    OnSelectFile(e, 
        _("Select dcraw"), 
#ifdef __WXMSW__
        _("Executables (*.exe)|*.exe"),
#else
        wxT("*"),
#endif
        "raw_dcraw_exe");
}

void RawImportDialog::OnSelectRTExe(wxCommandEvent & e)
{
    OnSelectFile(e, 
        _("Select RawTherapee-cli"), 
#ifdef __WXMSW__
        _("Executables (*.exe)|*.exe"),
#else
        wxT("*"),
#endif
        "raw_rt_exe");
}

void RawImportDialog::OnSelectRTProcessingProfile(wxCommandEvent& e)
{
    OnSelectFile(e, 
        _("Select default RT processing profile"), 
        _("RT processing profile|*.pp3"),
        "raw_rt_processing_profile");
}

void RawImportDialog::OnSelectDarktableExe(wxCommandEvent & e)
{
    OnSelectFile(e,
        _("Select Darktable-cli"),
#ifdef __WXMSW__
        _("Executables (*.exe)|*.exe"),
#else
        wxT("*"),
#endif
        "raw_darktable_exe");
}

void RawImportDialog::OnSelectFile(wxCommandEvent& e, const wxString& caption, const wxString& filter, const char* id)
{
    wxTextCtrl* input = XRCCTRL(*this, id, wxTextCtrl);
    wxFileDialog dlg(this, caption, "", input->GetValue(), filter, wxFD_OPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        input->SetValue(dlg.GetPath());
    }
}

void RawImportDialog::FillImageList()
{
    wxListBox* listBox = XRCCTRL(*this, "raw_images_list", wxListBox);
    const int oldSelection = listBox->GetSelection();
    listBox->Clear();
    for (int i=0; i<m_rawImages.size();++i)
    {
        const wxFileName file(m_rawImages[i]);
        wxString string(file.GetFullName());
        if (i == m_refImg)
        {
            string.Append(" ").Append(_("(WB reference)"));
        };
        listBox->Append(string);
    };
    if (oldSelection>=0 && oldSelection < listBox->GetCount())
    {
        listBox->SetSelection(oldSelection);
    };
}

