// -*- c-basic-offset: 4 -*-
/** @file PreviewIdentifyTool.cpp
 *
 *  @author James Legg
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

#include "panoinc_WX.h"
#include "panoinc.h"

#include "PreviewIdentifyTool.h"
#include <config.h>

#include "base_wx/platform.h"
#include "GLPreviewFrame.h"
#include "MainFrame.h"

#include <wx/platform.h>

//multitexture feature requires glew on some systems
#include <GL/glew.h>

#ifdef __WXMAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <algorithm>
#include <vector>

// the size of the rectangular texture. Must be a power of two, and at least 8.
#define rect_ts 64
// the number of times larger the circular texture is, must be a power of 2, and
// at least 1. Making better improves the appearance of the circle.
#define circle_ts_multiple 4
#define circle_ts (rect_ts * circle_ts_multiple)
#define circle_middle ((float) (circle_ts - 1) / 2.0)
#define circle_border_outer (circle_middle - 0.5 * (float) circle_ts_multiple)
#define circle_border_inner (circle_middle - 2.5 * (float) circle_ts_multiple)
#define circle_border_peak (circle_middle - 1.5 * (float) circle_ts_multiple)

bool PreviewIdentifyTool::texture_created = false;
unsigned int PreviewIdentifyTool::circle_border_tex;
unsigned int PreviewIdentifyTool::rectangle_border_tex;
unsigned int PreviewIdentifyTool::font_tex;
unsigned int PreviewIdentifyTool::font_list;
std::vector<int> PreviewIdentifyTool::m_glyphWidth;

#define FONT_TEXTURE_HEIGHT 75
wxBitmap GenerateFontTexture(const int textureHeight, int& textureWidth, std::vector<int>& glyphWidth)
{
    wxBitmap bitmap(10 * textureHeight, textureHeight);
    wxMemoryDC dc(bitmap);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
#if wxCHECK_VERSION(3,0,0)
    wxFont font(wxSize(0, textureHeight), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
#else
    wxFont* font=wxFont::New(wxSize(0, textureHeight), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT(""));
    dc.SetFont(*font);
#endif    
    dc.SetTextForeground(*wxWHITE);
    dc.DrawText(wxT("0123456789"), 0, 0);
    textureWidth = 0;
    glyphWidth.resize(10, 0);
    for (int i = 0; i < 10; ++i)
    {
        wxString number;
        number << i;
        wxSize textSize = dc.GetTextExtent(number);
        textureWidth += textSize.GetWidth();
        glyphWidth[i] = textSize.GetWidth();
    };
#if !wxCHECK_VERSION(3,0,0)
    delete font;
#endif  
    dc.SelectObject(wxNullBitmap);
    return bitmap;
}

PreviewIdentifyTool::PreviewIdentifyTool(ToolHelper *helper,
                                         GLPreviewFrame *owner)
    : Tool(helper)
{
    holdControl = false;
    constantOn = false;
    holdLeft = false;
    stopUpdating = true;
    preview_frame = owner;
    if (!texture_created) {
        // make the textures. We have a circle border and a square one.
        // the textures are white with a the alpha chanel forming a border.
        glGenTextures(1, (GLuint*) &rectangle_border_tex);
        glGenTextures(1, (GLuint*) &circle_border_tex);
        // we only want to specify alpha, but using just alpha in opengl attaches 0
        // for the luminosity. I tried biasing the red green and blue values to get
        // them to 1.0, but it didn't work under OS X for some reason. Instead we
        // use a luminance alpha pair, and use 1.0 for luminance all the time.
        {
            // In the rectangle texture, the middle is 1/4 opaque, the outer pixels
            // are completely transparent, and one pixel in from the edges is
            // a completly opaque line.
            GLubyte rect_tex_data[rect_ts][rect_ts][2];
            // make everything white
            for (unsigned int x = 0; x < rect_ts; x++)
            {
                for (unsigned int y = 0; y < rect_ts; y++)
                {
                    rect_tex_data[x][y][0] = 255;
                }
            }
            // now set the middle of the mask semi transparent
            for (unsigned int x = 2; x < rect_ts - 2; x++)
            {
                for (unsigned int y = 2; y < rect_ts - 2; y++)
                {
                    rect_tex_data[x][y][1] = 63;
                }
            }
            // make an opaque border
            for (unsigned int d = 1; d < rect_ts - 1; d++)
            {
                rect_tex_data[d][1][1] = 255;
                rect_tex_data[d][rect_ts - 2][1] = 255;
                rect_tex_data[1][d][1] = 255;
                rect_tex_data[rect_ts - 2][d][1] = 255;
            }
            // make a transparent border around that
            for (unsigned int d = 0; d < rect_ts; d++)
            {
                rect_tex_data[d][0][1] = 0;
                rect_tex_data[d][rect_ts - 1][1] = 0;
                rect_tex_data[0][d][1] = 0;
                rect_tex_data[rect_ts - 1][d][1] = 0;
            }
            glBindTexture(GL_TEXTURE_2D, rectangle_border_tex);
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, rect_ts, rect_ts,
                              GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, rect_tex_data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            // clamp texture so it won't wrap over the border.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }
        {
            // the circular one should look similar to the rectangle one, but we
            // enlarge it so that the circle apears less blocky. We don't want to
            // make it equally sharper however, so we make it a bit fuzzier by
            // blending.
            GLubyte circle_tex_data[circle_ts][circle_ts][2];
            for (unsigned int x = 0; x < circle_ts; x++)
            {
                for (unsigned int y = 0; y < circle_ts; y++)
                {
                    float x_offs = (float) x - circle_middle,
                          y_offs = (float) y - circle_middle,
                          radius = sqrt(x_offs * x_offs + y_offs * y_offs),
                          intensity;
                    if (radius < circle_border_inner)
                    {
                        intensity = 63.0;
                    } else if (radius < circle_border_peak)
                    {
                        intensity = (radius - circle_border_inner) /
                              (float) circle_ts_multiple * 255.0 * 3.0 / 4.0 + 64.0;
                    } else if (radius < circle_border_outer)
                    {
                        intensity = (radius - circle_border_peak) /
                                        (float) circle_ts_multiple * -255.0 + 256.0;
                    } else
                    {
                        intensity = 0.0;
                    }
                    circle_tex_data[x][y][0] = 255;
                    circle_tex_data[x][y][1] = (unsigned char) intensity;
                }
            }
            glBindTexture(GL_TEXTURE_2D, circle_border_tex);
            gluBuild2DMipmaps(GL_TEXTURE_2D,
                              GL_LUMINANCE_ALPHA, circle_ts, circle_ts,
                              GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, circle_tex_data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            // clamp texture so it won't wrap over the border.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }
        {
            int textureWidth;
            wxBitmap fontTexture(GenerateFontTexture(FONT_TEXTURE_HEIGHT, textureWidth, m_glyphWidth));
            wxImage image = fontTexture.ConvertToImage();
            glGenTextures(1, (GLuint*)&font_tex);
            glBindTexture(GL_TEXTURE_2D, font_tex);
            GLubyte* font_tex_data = new GLubyte[FONT_TEXTURE_HEIGHT * textureWidth * 2];
            for (size_t x = 0; x < textureWidth; ++x)
            {
                for (size_t y = 0; y < FONT_TEXTURE_HEIGHT; ++y)
                {
                    const unsigned char value = image.GetRed(x, y);
                    font_tex_data[2 * y * textureWidth + 2 * x] = value;
                    font_tex_data[2 * y * textureWidth + 2 * x + 1] = value;
                };
            };
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, textureWidth, FONT_TEXTURE_HEIGHT,
                GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, font_tex_data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_LINEAR_MIPMAP_LINEAR);
            delete[] font_tex_data;

            font_list = glGenLists(10);
            float posX = 0;
            for (int loop = 0; loop<10; loop++)
            {
                glNewList(font_list + loop, GL_COMPILE);
                glBegin(GL_QUADS);
                glTexCoord2f(posX, 0);
                glVertex2i(0, 0);
                glTexCoord2f(posX + m_glyphWidth[loop] / static_cast<float>(textureWidth), 0);
                glVertex2i(m_glyphWidth[loop], 0);
                glTexCoord2f(posX + m_glyphWidth[loop] / static_cast<float>(textureWidth), 1);
                glVertex2i(m_glyphWidth[loop], FONT_TEXTURE_HEIGHT);
                glTexCoord2f(posX, 1);
                glVertex2i(0, FONT_TEXTURE_HEIGHT);
                glEnd();
                glTranslatef(m_glyphWidth[loop], 0, 0);
                glEndList();
                posX += m_glyphWidth[loop] / static_cast<float>(textureWidth);
            }
        }
        texture_created = true;
    }
}
PreviewIdentifyTool::~PreviewIdentifyTool()
{
    // free the textures
    glDeleteTextures(1, (GLuint*) &rectangle_border_tex);
    glDeleteTextures(1, (GLuint*) &circle_border_tex);
    glDeleteLists(font_list, 10);
    glDeleteTextures(1, (GLuint*) &font_tex);
}

void PreviewIdentifyTool::Activate()
{
    // register notifications
    helper->NotifyMe(PreviewToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(PreviewToolHelper::DRAW_OVER_IMAGES, this);
    helper->NotifyMe(PreviewToolHelper::IMAGES_UNDER_MOUSE_CHANGE, this);
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    
    // clear up
    // Assume that there are no images under the mouse when the tool is
    // activated. This should be fine if the user clicks the button to activate
    // the tool.
    image_set.clear();
    mouse_is_over_button = false;
    /* TODO if it becomes possible to activate the tool by a keyboard shortcut
     * or something, call ImagesUnderMouseChangedEvent() to make sure we display
     * indicators for images currently under the cursor. */
    ImagesUnderMouseChangedEvent();

    helper->SetStatusMessage(_("Move the mouse over the images or image buttons to identify them."));
}

void PreviewIdentifyTool::StopUpdating() {
    if (!image_set.empty()) {
        std::set<unsigned int>::iterator iterator;
        for (iterator = image_set.begin(); iterator != image_set.end(); ++iterator)
        {
            DEBUG_ASSERT(*iterator < helper->GetPanoramaPtr()->getNrOfImages());
            // reset this button to its default system colour.
            preview_frame->SetImageButtonColour(*iterator, 0, 0, 0);
            // remove the notification
            helper->DoNotNotifyMeBeforeDrawing(*iterator, this);
        }
    }
    image_set.clear();
    preview_frame->UpdateIdentifyTools(image_set);
    stopUpdating = true;
    ForceRedraw();
}

void PreviewIdentifyTool::ContinueUpdating() {
    stopUpdating = false;
    ImagesUnderMouseChangedEvent();
}

void PreviewIdentifyTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{

    bool stop = false;
    bool start = false;

    if (constantOn) {
        if (e.Dragging() && !(holdLeft)) {
            stop = true;
            stopUpdating = true;
        }

        if (stopUpdating && !e.LeftIsDown() && !e.MiddleIsDown() && !e.RightIsDown()) {
            start = true;
        }
    }

    if (holdControl && !e.m_controlDown) {
        holdControl = false;
        if (!constantOn) {
            stop = true;
        }
    }
    
    if (!holdControl && e.m_controlDown) {
        holdControl = true;
        stop = false;
        if (stopUpdating) {
            start = true;
        }
    }
    
    if (stop) {
        this->StopUpdating();
    } else if(start) {
        this->ContinueUpdating();
    }


}

void PreviewIdentifyTool::KeypressEvent(int keycode, int modifierss, int pressed) {

    if (keycode == WXK_CONTROL) {
        if (pressed) {
            holdControl = true;
            ContinueUpdating();
        } else {
            if(holdControl) {
                StopUpdating();
            }
            holdControl = false;
        }
    }
}

void PreviewIdentifyTool::setConstantOn(bool constant_on_in) {
    constantOn = constant_on_in;
    if (constant_on_in) {
        stopUpdating = false;
        ContinueUpdating();
    } else {
        stopUpdating = true;
        StopUpdating();
    }

}


void PreviewIdentifyTool::ImagesUnderMouseChangedEvent()
{
    if (stopUpdating) {
        return;
    }
    
    std::set<unsigned int> new_image_set = helper->GetImageNumbersUnderMouse();
    
    //UpdateIdentifyTools() will unrequest notification for the old indicators,
    //reset the button colors, request notification for the new ones, swap in
    //image_set with new_image_set, and force a redraw for all three
    //PreviewIdentifyTool objects in GLPreviewFrame. This has the effect of 
    //displaying the indicators in both the preview and overview when you move
    //your mouse over either when the Identify button is toggled on. 
    preview_frame->UpdateIdentifyTools(new_image_set);
    
    // if there is exactly two images, tell the user they can click to edit CPs.
    if (image_set.size() == 2)
    {
         helper->SetStatusMessage(_("Click to create or edit control points here."));
    } else {
         helper->SetStatusMessage(_("Move the mouse over the images or image buttons to identify them."));
    }
}

void PreviewIdentifyTool::AfterDrawImagesEvent()
{
    // we draw the partly transparent identification boxes over the top of the
    // entire stack of images in image_set so that the extents of images in the
    // background are clearly marked.
    unsigned int num_images = image_set.size();
    // draw the actual images
    // the preview draws them in reverse order, so the lowest numbered appears
    // on top. We will folow this convention to avoid confusion.
    glMatrixMode(GL_MODELVIEW);
    std::set<unsigned int>::reverse_iterator it;
    for (it = image_set.rbegin(); it != image_set.rend(); ++it)
    {
        DEBUG_ASSERT(*it < helper->GetPanoramaPtr()->getNrOfImages());
        helper->GetViewStatePtr()->GetTextureManager()->
                DrawImage(*it,
                         helper->GetVisualizationStatePtr()->GetMeshDisplayList(*it));
    }
    glMatrixMode(GL_TEXTURE);
    
    // now draw the identification boxes
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int image_counter = 0;
    const unsigned int canvasWidth = helper->GetViewStatePtr()->GetOptions()->getWidth();
    const unsigned int canvasHeight = helper->GetViewStatePtr()->GetOptions()->getHeight();
    for (it = image_set.rbegin(); it != image_set.rend(); ++it)
    {
        glMatrixMode(GL_TEXTURE);
        // Use the mask to alter the shape of the identification boxes, but
        // replace the normal image texture with the identification box itself.
        if (helper->GetViewStatePtr()->GetSupportMultiTexture())
        {
            helper->GetViewStatePtr()->GetTextureManager()->BindTexture(*it);
            glActiveTexture(GL_TEXTURE0);
        }
        // we want to shift the texture so it lines up with the cropped region.
        glPushMatrix();
        HuginBase::SrcPanoImage *src = helper->GetViewStatePtr()->
                                                               GetSrcImage(*it);
        int width = src->getSize().width(), height = src->getSize().height();
        vigra::Rect2D crop_region = src->getCropRect();
        // pick a texture depending on crop mode and move it to the cropped area
        switch (src->getCropMode())
        {
            case HuginBase::SrcPanoImage::CROP_CIRCLE:
                glBindTexture(GL_TEXTURE_2D, circle_border_tex);
                // change the crop region to a square around the circle.
                if (crop_region.width() < crop_region.height())
                {
                    // too tall, move top and bottom.
                    int diff = (crop_region.width() - crop_region.height()) / 2;
                    // diff is negative, so we will shrink the border in the y
                    // direction.
                    crop_region.addBorder(0, diff);
                } else if (crop_region.width() > crop_region.height())
                {
                    // too wide, move left and right
                    int diff = (crop_region.height() - crop_region.width()) / 2;
                    crop_region.addBorder(diff, 0);
                }
                {
                    float diameter = (float) crop_region.width();
                    glScalef((float) width / diameter,
                             (float) height / diameter, 1.0);
                    glTranslatef(-(float) crop_region.left() / (float) width,
                                 -(float) crop_region.top() / (float) height,
                                 0.0);
                }
                break;
            case HuginBase::SrcPanoImage::CROP_RECTANGLE:
                // get the biggest rectangle contained by both the image 
                // and the cropped area.
                crop_region &= vigra::Rect2D(src->getSize());
                glBindTexture(GL_TEXTURE_2D, rectangle_border_tex);
                glScalef((float) width / (float) crop_region.width(),
                         (float) height / (float) crop_region.height(),
                         1.0);
                glTranslatef(-(float) crop_region.left() / (float) width,
                             -(float) crop_region.top() / (float) height,
                             0.0);
                break;
            case HuginBase::SrcPanoImage::NO_CROP:
                glBindTexture(GL_TEXTURE_2D, rectangle_border_tex);
                break;
        }
        // draw the image in this texture
        glMatrixMode(GL_MODELVIEW);
        unsigned char r,g,b;
        HighlightColour(image_counter, num_images, r, g, b);
        image_counter++;
        glColor3ub(r,g,b);
        glCallList(helper->GetVisualizationStatePtr()->GetMeshDisplayList(*it));
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
        // tell the preview frame to update the button to show the same colour.
        preview_frame->SetImageButtonColour(*it, r, g, b);
        // draw number
        HuginBase::PanoramaOptions* opts = helper->GetViewStatePtr()->GetOptions();
        HuginBase::SrcPanoImage* img = helper->GetViewStatePtr()->GetSrcImage(*it);
        HuginBase::PTools::Transform transform;
        transform.createInvTransform(*img, *opts);
        hugin_utils::FDiff2D imageCenter(crop_region.upperLeft() + crop_region.size() / 2);
        hugin_utils::FDiff2D imageCenterPano;
        if (transform.transformImgCoord(imageCenterPano, imageCenter) && !wxGetKeyState(WXK_ALT))
        {
            glBindTexture(GL_TEXTURE_2D, font_tex);
            if (helper->GetViewStatePtr()->GetSupportMultiTexture())
            {
                glActiveTexture(GL_TEXTURE1);
                glDisable(GL_TEXTURE_2D);
                glActiveTexture(GL_TEXTURE0);
            };
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            wxString number;
            number << *it;
            int textWidth = 0;
            unsigned char* listIndex = new unsigned char[number.Length()];
            for (size_t i = 0; i < number.Length(); ++i)
            {
#if wxCHECK_VERSION(3,0,0)
                textWidth += m_glyphWidth[number[i].GetValue() - 48];
                listIndex[i] = number[i].GetValue() - 48;
#else
                textWidth += m_glyphWidth[number[i] - 48];
                listIndex[i] = number[i] - 48;
#endif
            }
            float scaleFactor = std::min(canvasWidth, canvasHeight) / static_cast<float>(FONT_TEXTURE_HEIGHT) / 10;
            imageCenterPano.x = std::min<double>(std::max<double>(imageCenterPano.x, textWidth*scaleFactor / 2.0), canvasWidth - textWidth * scaleFactor / 2.0);
            imageCenterPano.y = std::min<double>(std::max<double>(imageCenterPano.y, FONT_TEXTURE_HEIGHT*scaleFactor / 2.0), canvasHeight - FONT_TEXTURE_HEIGHT * scaleFactor / 2.0);
            glTranslatef(imageCenterPano.x, imageCenterPano.y, 0);
            glScalef(scaleFactor, scaleFactor, 1.0);
            glTranslatef(-textWidth / 2, -FONT_TEXTURE_HEIGHT / 2, 0);
            glListBase(font_list);
            glCallLists(number.Length(), GL_UNSIGNED_BYTE, listIndex);
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            delete[] listIndex;
        };
    }
    // set stuff back how we found it.
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND);
    glColor3ub(255, 255, 255);
}

bool PreviewIdentifyTool::BeforeDrawImageEvent(unsigned int image)
{
    // Delay drawing of images, so we can show them on top of the others.
    DEBUG_ASSERT(image < helper->GetPanoramaPtr()->getNrOfImages());
    if (image_set.count(image)) return false;
    return true;
}

void PreviewIdentifyTool::ShowImageNumber(unsigned int image)
{
    DEBUG_ASSERT(image < helper->GetPanoramaPtr()->getNrOfImages());
    // Add this image to the set of images drawn.
    if (!image_set.count(image))
    {
        // it is not already in the set. Add it now
        image_set.insert(image);
        // we want notification of when it is drawn so we can delay drawing.
        helper->NotifyMeBeforeDrawing(image, this);
        //  now we want a redraw.
        ForceRedraw();
    }
    mouse_over_image = image;
    mouse_is_over_button = true;
}

void PreviewIdentifyTool::StopShowingImages()
{
    if (mouse_is_over_button)
    {
        // set the colour to the image the user just moused off to the default.
        preview_frame->SetImageButtonColour(mouse_over_image, 0, 0, 0);
        helper->DoNotNotifyMeBeforeDrawing(mouse_over_image, this);
        image_set.erase(mouse_over_image);
        // now redraw without the indicator.
        ForceRedraw();
        mouse_is_over_button = false;
    }    
}

// generate a colour given how many colours we need and an index.
void PreviewIdentifyTool::HighlightColour(unsigned int index,
                                          unsigned int count,
                                          unsigned char &red,
                                          unsigned char &green,
                                          unsigned char &blue)
{
    DEBUG_ASSERT(index < count && index >= 0);
    // the first one is red, the rest are evenly spaced throughout the spectrum 
    float hue = ((float) index / (float) count) * 6.0;
    if (hue < 1.0)
    {
        // red to yellow
        red = 255;
        green = (unsigned char) (hue * 255.0);
        blue = 0;
    } else if (hue < 2.0) {
        // yellow to green
        red = (unsigned char) ((-hue + 2.0) * 255.0);
        green = 255;
        blue = 0;
    } else if (hue < 3.0) {
        // green to cyan
        red = 0;
        green = 255;
        blue = (unsigned char) ((hue - 2.0) * 255.0);
    } else if (hue < 4.0) {
        // cyan to blue
        red = 0;
        green = (unsigned char) ((-hue + 4.0) * 255.0);
        blue = 255;
    } else if (hue < 5.0) {
        // blue to magenta
        red = (unsigned char) ((hue - 4.0) * 255.0);
        green = 0;
        blue = 255;
    } else {
        // magenta to red
        red = 255;
        green = 0;
        blue = (unsigned char) ((-hue + 6.0) * 255.0);
    }
}

void PreviewIdentifyTool::MouseButtonEvent(wxMouseEvent & e)
{


    if ( e.LeftDown() && helper->IsMouseOverPano())
    {   
        holdLeft = true;
    } 

    if (holdLeft && e.LeftUp() && (image_set.size()==1 || image_set.size() == 2)) 
    {
        holdLeft = false;
        if (constantOn || e.CmdDown())
        {
            // when there are only two images with indicators shown, show the
            // control point editor with those images when left clicked.
            if(image_set.size()==2)
            {
                MainFrame::Get()->ShowCtrlPointEditor(*(image_set.begin()),
                                                        *(++image_set.begin()));
            }
            else
            {
                MainFrame::Get()->ShowMaskEditor(*image_set.begin());
            };
            MainFrame::Get()->Raise();
        }
    }

    if (holdLeft && !(e.LeftIsDown())) {
        holdLeft = false;
    }

    if (constantOn) {
        if (e.ButtonUp() && !e.MiddleIsDown() && !e.RightIsDown()) {
            stopUpdating = false;
            ImagesUnderMouseChangedEvent();
        }
    }

}

void PreviewIdentifyTool::UpdateWithNewImageSet(std::set<unsigned int> new_image_set)
{
    // If we are currently showing indicators for some of the images, we want
    // to work out which ones are not in the new set, so we can set their
    // buttons back to the system colour.
    if (!image_set.empty())
    {
        HuginBase::UIntSet difference;
        std::set_difference (image_set.begin(), image_set.end(),
                             new_image_set.begin(), new_image_set.end(),
                             std::inserter(difference,difference.end()));
        if (!difference.empty())
        {
            for (HuginBase::UIntSet::iterator iterator = difference.begin(); iterator != difference.end(); ++iterator)
            {
                DEBUG_ASSERT(*iterator < helper->GetPanoramaPtr()->getNrOfImages());
                // reset this button to its default system colour.
                preview_frame->SetImageButtonColour(*iterator, 0, 0, 0);
                // remove the notification
                helper->DoNotNotifyMeBeforeDrawing(*iterator, this);
            }
        }
    }

    // now request to be notified when drawing the new ones.
    if (!new_image_set.empty())
    {
        HuginBase::UIntSet difference;
        std::set_difference(new_image_set.begin(), new_image_set.end(),
            image_set.begin(), image_set.end(),
            std::inserter(difference, difference.end()));
        if (!difference.empty())
        {
            for (HuginBase::UIntSet::iterator iterator = difference.begin(); iterator != difference.end(); ++iterator)
            {
                DEBUG_ASSERT(*iterator < helper->GetPanoramaPtr()->getNrOfImages());
                // get notification of when this is about to be drawn.
                helper->NotifyMeBeforeDrawing(*iterator, this);
            }
        }
    };
    // remember the new set.
    image_set.swap(new_image_set);

    // Redraw with new indicators. Since the indicators aren't part of the
    // panorama and none of it is likely to change, we have to persuade the
    // viewstate that a redraw is required.
    ForceRedraw();
}

void PreviewIdentifyTool::ForceRedraw()
{
    helper->GetVisualizationStatePtr()->ForceRequireRedraw();
    helper->GetVisualizationStatePtr()->Redraw();
}
