// -*- c-basic-offset: 4 -*-
/** @file MaskEdEditWnd.cpp
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

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"

#include <vigra/basicimageview.hxx>
#include "vigra_ext/blend.h"
#include "PT/Stitcher.h"


#include "base_wx/ImageCache.h"
#include "hugin/PreviewFrame.h"
#include "hugin/MainFrame.h"
#include "hugin/CommandHistory.h"
#include "hugin/config_defaults.h"
#include "hugin/huginApp.h"
//#include "hugin/ImageProcessing.h"
#include <vigra_ext/ROIImage.h>
#include <math.h>
#include "ISegmentation.h"
#include "MaskEdEditWnd.h"

using namespace PT;
using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace hugin_utils;
using HuginBase::ImageCache;

typedef RGBValue<unsigned char> BRGBValue;

BEGIN_EVENT_TABLE(MaskEdEditWnd, wxScrolledWindow)
    EVT_LEFT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_LEFT_UP(MaskEdEditWnd::OnLeftMouseButtonUp)
    EVT_RIGHT_DOWN(MaskEdEditWnd::OnMouseButtonDown)
    EVT_RIGHT_UP(MaskEdEditWnd::OnRightMouseButtonUp)
    EVT_MOTION(MaskEdEditWnd::OnMotion)
    EVT_PAINT(MaskEdEditWnd::OnPaint)
END_EVENT_TABLE()

MaskEdEditWnd::MaskEdEditWnd(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
                     : wxScrolledWindow(parent, winid, pos, size, style, name), m_scale(1.0), m_active(0),
                     m_max_width(0), m_max_height(0), m_bShowOverlappedRect(false),
                     m_state_rendering(false), m_rerender(false), m_imgsDirty(true)
{
    MaskMgr::getInstance()->setSegmentationOption(0);
}

MaskEdEditWnd::~MaskEdEditWnd() 
{
    m_bimgs.clear();
}

void MaskEdEditWnd::init(PreviewFrame *parent, PT::Panorama * panorama )
{
    m_pano = panorama;
    m_parentWindow = parent;
    m_pano->addObserver(this);
}

void MaskEdEditWnd::init()
{
    m_bShowOverlappedRect = false;
    m_bimgs.clear();
    m_imgfiles.clear();
    m_selected.clear();
    m_brushstroke.clear();
    m_max_width = 0;
    m_max_height = 0;
    m_canvas_size.clear();
    m_pos.clear();
    m_state_rendering = false;
    m_rerender = false;
    m_imgsDirty = true;
}

void MaskEdEditWnd::setEditMode(MaskEdEditMode_t edmode)
{
    m_edmode = edmode;
}

void MaskEdEditWnd::loadImage(const wxString &filename)
{
    loadImage(string(filename.mb_str()));
}
void MaskEdEditWnd::loadImage(const string &filename)
{
    if(filename != "")
    {
        ImageCache::EntryPtr e = ImageCache::getInstance().getImage(filename);
        HuginBase::ImageCache::ImageCacheRGB8Ptr img = e->get8BitImage();
        vigra::ImageImportInfo info(filename.c_str());
        vigra::Size2D sz = info.getCanvasSize();
        vigra::Diff2D pos = info.getPosition();
        m_canvas_size.push_back(wxPoint(sz.width(), sz.height()));
        m_pos.push_back(wxPoint(pos.x, pos.y));
        m_selected.push_back(true);
        m_max_width = sz.width() > m_max_width ? sz.width() : m_max_width;
        m_max_height = sz.height() > m_max_height ? sz.height() : m_max_height;
        if (img) {
            m_max_width = img->width() > m_max_width ? img->width() : m_max_width;
            m_max_height = img->height() > m_max_height ? img->height() : m_max_height;

            wxImage img_temp(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true);
            wxBitmap *mask = MaskMgr::getInstance()->getSegmentation(filename)->getMaskBitmap();
            img_temp.SetMaskFromImage(mask->ConvertToImage(), 0, 0, 0);
            img_temp.Rescale(img->width()*m_scale, img->height()*m_scale);
            m_bimgs.push_back(new wxBitmap(img_temp));
        } 
        SetScrollbars( 1, 1, m_max_width, m_max_height);
    }
}

void MaskEdEditWnd::loadImages(const vector<string> &filesv)
{
    //remove_copy_if(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles), );
    copy(filesv.begin(), filesv.end(), back_insert_iterator<vector<string> >(m_imgfiles));
    //creates local cache of images
    for(vector<string>::const_iterator it = filesv.begin(); it != filesv.end(); it++)
    {
        loadImage(*it);
    }
}

void MaskEdEditWnd::findOverlappingRect(int i, int j, wxRect &rect)
{
    wxRect rectA, rectB;
    
    //rectA is the leftmost rectangle
    if(m_pos[i].x < m_pos[j].x) {
        rectA = wxRect(m_pos[i], wxSize(m_bimgs[i]->GetWidth(), m_bimgs[i]->GetHeight()));
        rectB = wxRect(m_pos[j], wxSize(m_bimgs[j]->GetWidth(), m_bimgs[j]->GetHeight()));
    } else {
        rectA = wxRect(m_pos[j], wxSize(m_bimgs[j]->GetWidth(), m_bimgs[j]->GetHeight()));
        rectB = wxRect(m_pos[i], wxSize(m_bimgs[i]->GetWidth(), m_bimgs[i]->GetHeight()));
    }
    
    //set upper-left corner
    rect.x = rectB.x;
    if(rectB.y < rectA.y)
        rect.y = rectA.y;
    else
        rect.y = rectB.y;

    //set bottom-right corner
    wxPoint brA = rectA.GetBottomRight();
    wxPoint brB = rectB.GetBottomRight();
    wxPoint br;
    if(brB.y < brA.y) {
        br.y = brB.y;
    } else {
        br.y = brA.y;
    }
    if(brB.x < brA.x)
        br.x = brB.x;
    else
        br.x = brA.x;
    rect.SetBottomRight(br);
}

void MaskEdEditWnd::updatePreview()
{
    init();
    HuginBase::SrcPanoImage panoImg;
    HuginBase::UIntSet activeImages = m_pano->getActiveImages();
    for(HuginBase::UIntSet::iterator it = activeImages.begin(); it != activeImages.end(); it++)
    {
        panoImg = m_pano->getSrcImage(*it);
        MaskMgr::getInstance()->loadImage(panoImg.getFilename());
        loadImage(panoImg.getFilename());
    }
}
//void MaskEdEditWnd::updatePreview(int imgIndex)
//{
//    DEBUG_TRACE("");
//
//    // we can accidentally end up here recursively, because wxWidgets
//    // allows user input during redraw of the progress in the bottom
//    if (m_state_rendering) {
//        DEBUG_DEBUG("m_state_rendering == true, aborting rendering");
//        m_rerender = true;
//        return;
//    }
//
//    DEBUG_DEBUG("m_state_rendering = true");
//    m_state_rendering = true;
//    m_rerender = false;
//
//    long nthreads = wxConfigBase::Get()->Read(wxT("/Nona/NumberOfThreads"), wxThread::GetCPUCount());
//    if (nthreads < 1) nthreads = 1;
//    vigra_ext::ThreadManager::get().setNThreads(nthreads);
//
//
//	{
//	  // Even though the frame is hidden, the panel is not
//	  // so check the parent instead
//	  if (m_parentWindow) {
//  		if (m_parentWindow->IsShown() && (! m_parentWindow->IsIconized())) {
//		  DEBUG_INFO("Parent window shown - updating");
//		} else {
//		  DEBUG_INFO("Parent window hidden - not updating");
//                  m_state_rendering = false;
//		  return;
//		}
//	  }
//	}
////    bool seaming = wxConfigBase::Get()->Read("/PreviewPanel/UseSeaming",0l) != 0;
//
//    // temporary bitmap for our remapped image
//    // calculate the image size from panel widht, height from vfov
//
////    long cor = wxConfigBase::Get()->Read("/PreviewPanel/correctDistortion",0l);
////    bool corrLens = cor != 0;
//
//    wxBusyCursor wait;
//    double finalWidth = m_pano->getOptions().getWidth();
//    double finalHeight = m_pano->getOptions().getHeight();
//
//    m_panoImgSize = Diff2D(GetClientSize().GetWidth(), GetClientSize().GetHeight());
//
//    double ratioPano = finalWidth / finalHeight;
//    double ratioPanel = (double)m_panoImgSize.x / (double)m_panoImgSize.y;
//
//    DEBUG_DEBUG("panorama ratio: " << ratioPano << "  panel ratio: " << ratioPanel);
//
//    if (ratioPano < ratioPanel) {
//        // panel is wider than pano
//        m_panoImgSize.x = ((int) (m_panoImgSize.y * ratioPano));
//        DEBUG_DEBUG("portrait: " << m_panoImgSize);
//    } else {
//        // panel is taller than pano
//        m_panoImgSize.y = ((int)(m_panoImgSize.x / ratioPano));
//        DEBUG_DEBUG("landscape: " << m_panoImgSize);
//    }
//
//    PanoramaOptions opts = m_pano->getOptions();
//    opts.setWidth(m_panoImgSize.x, false);
//    opts.setHeight(m_panoImgSize.y);
//    //m_panoImgSize.y = opts.getHeight();
//    // always use bilinear for preview.
//
//    // reset ROI. The preview needs to draw the parts outside the ROI, too!
//    opts.setROI(Rect2D(opts.getSize()));
//    opts.interpolator = vigra_ext::INTERP_BILINEAR;
//
//    // create images
//    wxImage panoImage(m_panoImgSize.x, m_panoImgSize.y);
//    try {
//        vigra::BasicImageView<RGBValue<unsigned char> > panoImg8((RGBValue<unsigned char> *)panoImage.GetData(), panoImage.GetWidth(), panoImage.GetHeight());
//        FRGBImage panoImg(m_panoImgSize);
//        BImage alpha(m_panoImgSize);
//        // the empty panorama roi
////        Rect2D panoROI;
//        DEBUG_DEBUG("about to stitch images, pano size: " << m_panoImgSize);
//        UIntSet displayedImages = m_pano->getActiveImages();
//        if (displayedImages.size() > 0) {
//            if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
//                DEBUG_DEBUG("HDR output merge");
//
//                ReduceToHDRFunctor<RGBValue<float> > hdrmerge;
//                ReduceStitcher<FRGBImage, BImage> stitcher(*m_pano, *m_parentWindow);
//                stitcher.stitch(opts, displayedImages,
//                                destImageRange(panoImg), destImage(alpha),
//                                m_remapCache,
//                                hdrmerge);
//                /*
//                std::vector<RemappedPanoImage<FRGBImage, BImage> *> remapped;
//                // get all remapped images
//                for (UIntSet::const_iterator it = displayedImages.begin();
//                     it != displayedImages.end(); ++it)
//                {
//                    remapped.push_back(m_remapCache.getRemapped(pano, opts, *it, *m_parentWindow));
//                }
//                reduceROIImages(remapped,
//                                destImageRange(panoImg), destImage(alpha),
//                                hdrmerge);
//                */
//#ifdef DEBUG_REMAP
//{
//    vigra::ImageExportInfo exi( DEBUG_FILE_PREFIX "hugin04_preview_HDR_Reduce.tif"); \
//            vigra::exportImage(vigra::srcImageRange(panoImg), exi); \
//}
//{
//    vigra::ImageExportInfo exi(DEBUG_FILE_PREFIX "hugin04_preview_HDR_Reduce_Alpha.tif"); \
//            vigra::exportImage(vigra::srcImageRange(alpha), exi); \
//}
//#endif
//
//                // find min and max
//                vigra::FindMinMax<float> minmax;   // init functor
//                vigra::inspectImageIf(srcImageRange(panoImg), srcImage(alpha),
//                                    minmax);
//                double min = std::max(minmax.min, 1e-6f);
//                double max = minmax.max;
//
//#if 0
//                for (int i=0; i<3; i++) {
//                    if (minmax.min[i]> 1e-6 && minmax.min[i] < min)
//                        min = minmax.min[i];
//                }
//                double max = DBL_MIN;
//                for (int i=0; i<3; i++) {
//                    if (minmax.max[i]> 1e-6 && minmax.max[i] > max)
//                        max = minmax.max[i];
//                }
//#endif
//
//                int mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/MappingFloat"), HUGIN_IMGCACHE_MAPPING_FLOAT);
//                applyMapping(srcImageRange(panoImg), destImage(panoImg8), min, max, mapping);
//
//            } else {
//                    // LDR output
//    //            FileRemapper<BRGBImage, BImage> m;
//                switch (m_blendMode) {
//                case BLEND_COPY:
//                {
//                    StackingBlender blender;
//    //                SimpleStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
//                    SimpleStitcher<FRGBImage, BImage> stitcher(*m_pano, *m_parentWindow);
//                    stitcher.stitch(opts, displayedImages,
//                                    destImageRange(panoImg), destImage(alpha),
//                                    m_remapCache,
//                                    blender);
//                    break;
//                }
//                case BLEND_DIFFERENCE:
//                {
//                    ReduceToDifferenceFunctor<RGBValue<float> > func;
//                    ReduceStitcher<FRGBImage, BImage> stitcher(*pano, *m_parentWindow);
//                    stitcher.stitch(opts, displayedImages,
//                                    destImageRange(panoImg), destImage(alpha),
//                                    m_remapCache,
//                                    func);
//                    break;
//    /*
//    
//                    WeightedStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
//                    stitcher.stitch(opts, m_displayedImages,
//                                    destImageRange(panoImg), destImage(alpha),
//                                    m_remapCache);
//                    break;
//    */
//                }
//    /*
//                case BLEND_DIFFERENCE:
//                {
//                    DifferenceBlender blender;
//                    SimpleStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
//                    stitcher.stitch(opts, m_displayedImages,
//                                    destImageRange(panoImg), destImage(alpha),
//                                    m_remapCache,
//                                    blender);
//                    break;
//                }
//    */
//                }
//
//                
//#ifdef DEBUG_REMAP
//{
//    vigra::ImageExportInfo exi( DEBUG_FILE_PREFIX "hugin04_preview_AfterRemap.tif"); \
//            vigra::exportImage(vigra::srcImageRange(panoImg), exi); \
//}
//{
//    vigra::ImageExportInfo exi(DEBUG_FILE_PREFIX "hugin04_preview_AfterRemapAlpha.tif"); \
//            vigra::exportImage(vigra::srcImageRange(alpha), exi); \
//}
//#endif
//
//                // apply default exposure and convert to 8 bit
//                SrcPanoImage src = pano->getSrcImage(0);
//
//                // apply the exposure
//                double scale = 1.0/pow(2.0,opts.outputExposureValue);
//
//                vigra::transformImage(srcImageRange(panoImg), destImage(panoImg),
//                                      vigra::functor::Arg1()*vigra::functor::Param(scale));
//
//                DEBUG_DEBUG("LDR output, with response: " << src.getResponseType());
//                if (src.getResponseType() == SrcPanoImage::RESPONSE_LINEAR) {
//                    vigra::copyImage(srcImageRange(panoImg), destImage(panoImg8));
////                    vigra::transformImage(srcImageRange(panoImg), destImage(panoImg8),
////                                          vigra::functor::Arg1()*vigra::functor::Param(255));
//                } else {
//                // create suitable lut for response
//                    typedef  std::vector<double> LUT;
//                    LUT lut;
//                    switch(src.getResponseType())
//                    {
//                        case SrcPanoImage::RESPONSE_EMOR:
//                            EMoR::createEMoRLUT(src.getEMoRParams(), lut);
//                            break;
//                        case SrcPanoImage::RESPONSE_GAMMA:
//                            lut.resize(256);
//                            createGammaLUT(1/src.getGamma(), lut);
//                            break;
//                        default:
//                            vigra_fail("Unknown or unsupported response function type");
//                            break;
//                    }
//                    // scale lut
//                    for (size_t i=0; i < lut.size(); i++) 
//                        lut[i] = lut[i]*255;
//                    typedef vigra::RGBValue<float> FRGB;
//                    LUTFunctor<FRGB, LUT> lutf(lut);
//
//                    vigra::transformImage(srcImageRange(panoImg), destImage(panoImg8),
//                                          lutf);
//                }
//            }
//        }
//
//#ifdef DEBUG_REMAP
//{
//    vigra::ImageExportInfo exi( DEBUG_FILE_PREFIX "hugin05_preview_final.tif"); \
//            vigra::exportImage(vigra::srcImageRange(panoImg8), exi); \
//}
//#endif
//
//
//    } catch (std::exception & e) {
//        m_state_rendering = false;
//        DEBUG_ERROR("error during stitching: " << e.what());
//        wxMessageBox(wxString(e.what(), wxConvLocal), _("Error during Stitching"));
//    }
//
//
//    // update the transform for pano -> erect coordinates
//    if (m_pano2erect) delete m_pano2erect;
//    SrcPanoImage src;
//    src.setProjection(SrcPanoImage::EQUIRECTANGULAR);
//    src.setHFOV(360);
//    src.setSize(Size2D(360,180));
//    m_pano2erect = new PTools::Transform;
//    m_pano2erect->createTransform(src, opts);
//
//    if (m_panoBitmap) {
//        delete m_panoBitmap;
//    }
//    m_panoBitmap = new wxBitmap(panoImage);
//
//
//    // always redraw
//    wxClientDC dc(this);
//    DrawPreview(dc);
//
//    m_state_rendering = false;
//    DEBUG_DEBUG("m_state_rendering = false");
//    m_rerender = false;
//}

void MaskEdEditWnd::ForceUpdate()
{
    updatePreview();
}


void MaskEdEditWnd::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    //DoPrepareDC(dc);
    int x,y;
    x = GetScrollPos(wxSB_HORIZONTAL);
    y = GetScrollPos(wxSB_VERTICAL);
    int i = m_bimgs.size() - 1;
    int lastDrawn = -1;
    //dc.SetDeviceOrigin(-x, -y);
    dc.SetUserScale(m_scale, m_scale);
    for(vector<wxBitmap*>::reverse_iterator it = m_bimgs.rbegin(); it != m_bimgs.rend(); it++, i--)
    {
        if(!m_selected[i]) continue;
        dc.DrawBitmap(**it, -x + m_pos[i].x, -y + m_pos[i].y, true); 
        if(m_bShowOverlappedRect && lastDrawn > -1) {
            wxRect rect;
            findOverlappingRect(lastDrawn, i, rect);
            wxPen pen = dc.GetPen();
            wxBrush brush = dc.GetBrush();
            dc.SetPen(*wxRED);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(rect);
            dc.SetPen(pen);
            dc.SetBrush(brush);
        }
        lastDrawn = i;
        //ignore this case for the timebeing
        ////drawing is not really needed here but just in case the window is refershed
        ////while the brushstroke is drawn
        //if(m_brushstroke.pt.size() > 1)
        //{
        //    std::vector<wxPoint>::iterator it = m_brushstroke.pt.begin();
        //    wxPoint pt = *it + wxPoint(-x,-y);
        //    wxPen pen(*wxRED, 1);
        //    dc.SetPen(pen);
        //    for(it++; it != m_brushstroke.pt.end(); it++)
        //    {
        //        dc.DrawLine(pt, *it + wxPoint(-x,-y));
        //        pt = *it + wxPoint(-x,-y);
        //    }
        //    dc.SetPen(wxNullPen);
        //}
    }
    if(m_poly.pt.size() > 0) {
        wxPoint *pts = new wxPoint[m_poly.pt.size()];
        dc.SetPen(*wxRED);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        //copy(m_poly.pt.begin(), m_poly.pt.end(), pts);
        int i = 0;
        for(vector<wxPoint>::iterator it = m_poly.pt.begin(); it != m_poly.pt.end(); it++,i++)
        {
            pts[i].x = it->x * m_scale - x + m_pos[m_active].x;
            pts[i].y = it->y * m_scale - y + m_pos[m_active].y;
        }
        dc.DrawPolygon(m_poly.pt.size(), pts);
    }
}

void MaskEdEditWnd::OnMouseButtonDown(wxMouseEvent &event)
{
    m_brushstroke.clear();
    event.Skip();
}

void MaskEdEditWnd::OnLeftMouseButtonUp(wxMouseEvent &event)
{
    if(m_bimgs.empty()) return;
    if(m_edmode == ME_BSTROKE)
    {
        m_brushstroke.label = ISegmentation::FGND;
        m_MaskEdCmdHist.addCommand(new BrushStrokeCmd(MaskMgr::getInstance(), m_brushstroke, m_active));
        reloadImages();
        m_brushstroke.clear();
    }
    else if(m_edmode == ME_POLY) {
        int x, y;
        x = GetScrollPos(wxSB_HORIZONTAL);
        y = GetScrollPos(wxSB_VERTICAL);
        wxPoint pos = event.GetPosition()+wxPoint(x,y)-m_pos[m_active];
        m_poly.add(wxPoint(pos.x/m_scale, pos.y/m_scale));
    }
    event.Skip();
    Refresh();
}

void MaskEdEditWnd::OnRightMouseButtonUp(wxMouseEvent &event)
{
    if(m_bimgs.empty()) return;
    if(m_edmode == ME_BSTROKE)
    {
        m_brushstroke.label = ISegmentation::BKGND;
        m_MaskEdCmdHist.addCommand(new BrushStrokeCmd(MaskMgr::getInstance(), m_brushstroke, m_active));
        reloadImages();
        m_brushstroke.clear();
    }
    else if(m_edmode == ME_POLY) {
        //end of creating polygon
        m_poly.label = ISegmentation::FGND;
        m_MaskEdCmdHist.addCommand(new PolygonCmd(MaskMgr::getInstance(), m_poly, m_active));
        reloadImages();
        m_poly.clear();
    }
    event.Skip();
    Refresh();
}
void MaskEdEditWnd::OnMotion(wxMouseEvent &event)
{
    if(event.Dragging() && !event.MiddleIsDown() && m_bimgs.size() > 0 && m_edmode == ME_BSTROKE)
    {
        int x, y;
        x = GetScrollPos(wxSB_HORIZONTAL);
        y = GetScrollPos(wxSB_VERTICAL);
        if(m_brushstroke.pt.size() > 0)
        {
            wxClientDC dc(this);
            //DoPrepareDC(dc);
            wxPen oldpen = dc.GetPen();
            wxPen *pen;

            if(event.LeftIsDown())
                pen = new wxPen(*wxRED, 1);
            else 
                pen = new wxPen(*wxBLUE, 1);
            dc.SetPen(*pen);
            dc.DrawLine(m_brushstroke.pt.back() - wxPoint(x, y) + m_pos[m_active], event.GetPosition());
            dc.SetPen(oldpen);
        }
        m_brushstroke.pt.push_back(event.GetPosition() + wxPoint(x, y) - m_pos[m_active]);
    }
}

void MaskEdEditWnd::reloadImages()
{
    m_bimgs.clear();
    for(vector<string>::iterator it = m_imgfiles.begin(); it != m_imgfiles.end(); it++)
        loadImage(*it);
}   

void MaskEdEditWnd::zoom(float scale, wxRect region)
{
    m_scale = scale;
    //reloadImages();
    //SetScrollbars( 1, 1, m_bimgs[0]->GetWidth()*m_scale, m_bimgs[0]->GetHeight()*m_scale);
    Refresh();
}

float MaskEdEditWnd::getZoomLevel() const
{
    return m_scale;
}

void MaskEdEditWnd::setDisplayImages(const std::map<int, std::pair<std::string, bool> > &selection)
{
    
}

void MaskEdEditWnd::setSelectedImage(int index, bool state)
{
    m_selected[index] = state;
    this->Refresh();
}

void MaskEdEditWnd::toggleShowOverlappedRect()
{
    m_bShowOverlappedRect = m_bShowOverlappedRect ? false : true;
    Refresh();
}

