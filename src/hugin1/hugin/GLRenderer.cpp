// -*- c-basic-offset: 4 -*-
/** @file GLRenderer.cpp
 *
 *  @author James Legg
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

#include <wx/wx.h>
#include <wx/platform.h>

#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <config.h>

#include "panoinc.h"

#include "TextureManager.h"
#include "MeshManager.h"
#include "ViewState.h"
#include "GLRenderer.h"
#include "ToolHelper.h"
#include <panodata/PanoramaOptions.h>

GLPreviewRenderer::GLPreviewRenderer(PT::Panorama *pano, TextureManager *tex_man,
                       MeshManager *mesh_man, VisualizationState *visualization_state,
                       PreviewToolHelper *tool_helper)
{
    m_pano = pano;
    m_tex_man = tex_man;
    m_mesh_man = mesh_man;
    m_visualization_state = visualization_state;
    m_tool_helper = tool_helper;
}

GLOverviewRenderer::GLOverviewRenderer(PT::Panorama *pano, TextureManager *tex_man,
                       MeshManager *mesh_man, PanosphereOverviewVisualizationState *visualization_state,
                       OverviewToolHelper *tool_helper)
{
    #ifdef __WXGTK__
    int dum = 0;
    glutInit(&dum, NULL);
    #endif

    m_pano = pano;
    m_tex_man = tex_man;
    m_mesh_man = mesh_man;
    m_visualization_state = visualization_state;
    m_tool_helper = tool_helper;
}


GLRenderer::~GLRenderer()
{
}

vigra::Diff2D GLPreviewRenderer::Resize(int in_width, int in_height)
{
  width = in_width;
  height = in_height;
  glViewport(0, 0, width, height);
  // we use the view_state rather than the panorama to allow interactivity.
  HuginBase::PanoramaOptions *options = m_visualization_state->getViewState()->GetOptions();
  width_o = options->getWidth();
  height_o = options->getHeight();
  double aspect_screen = double(width) / double (height),
        aspect_pano = width_o / height_o;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  
  double scale;
  if (aspect_screen < aspect_pano)
  {
      // the panorama is wider than the screen
      scale = width_o / width;
  } else {
      // the screen is wider than the panorama
      scale = height_o / height;
  }
  double x_offs = (scale * double(width) - width_o) / 2.0,
         y_offs = (scale * double(height) - height_o) / 2.0;
  // set up the projection, so we can use panorama coordinates.
  glOrtho(-x_offs, width * scale - x_offs,
          height * scale - y_offs, -y_offs,
          -1.0, 1.0);
  // scissor to the panorama.
  glScissor(x_offs / scale, y_offs / scale,
            width_o / scale, height_o / scale);
  glMatrixMode(GL_MODELVIEW);
  // tell the view state the region we are displaying.
  // TODO add support for zooming and panning.
  m_visualization_state->SetVisibleArea(vigra::Rect2D(0, 0, options->getWidth(),
                                             options->getHeight()));
  m_visualization_state->SetScale(1.0 / scale);
  // return the offset from the top left corner of the viewpoer to the top left
  // corner of the panorama.
  return vigra::Diff2D(int (x_offs / scale), int (y_offs / scale));
}

void GLRenderer::SetBackground(unsigned char red, unsigned char green, unsigned char blue)
{
    glClearColor((float) red / 255.0, (float) green / 255.0, (float) blue / 255.0, 1.0);
}

void GLPreviewRenderer::Redraw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    // draw the box around the panorma in black, with some background colour.
    m_tex_man->DisableTexture();
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_QUADS);
        glVertex2f(0.0, 0.0);
        glVertex2f(width_o, 0.0);
        glVertex2f(width_o, height_o);
        glVertex2f(0.0, height_o);
    glEnd();
    glColor3f(1.0, 1.0, 1.0);
    // draw things under the preview images
    m_tool_helper->BeforeDrawImages();
    // draw each active image.
    int imgs = m_pano->getNrOfImages();
    // offset by a half a pixel
    glPushMatrix();
    glTranslatef(0.5, 0.5, 0.0);
    glEnable(GL_TEXTURE_2D);
    m_tex_man->Begin();
    // The old preview shows the lowest numbered image on top, so do the same:
    for (int img = imgs - 1; img != -1; img--)
    {
        // only draw active images
        if (m_pano->getImage(img).getOptions().active)
        {
            // the tools can cancel drawing of images.
            if (m_tool_helper->BeforeDrawImageNumber(img))
            {
                // the texture manager may need to call the display list
                // multiple times with blending, so we pass it the display list
                // rather than switching to the texture and then calling the
                // list ourselves.
                m_tex_man->DrawImage(img, m_mesh_man->GetDisplayList(img));
                m_tool_helper->AfterDrawImageNumber(img);
            }
        }
    }
    m_tex_man->End();
    // drawn things after the active image.
    m_tool_helper->AfterDrawImages();
    m_tex_man->DisableTexture();
    glPopMatrix();
    // darken the cropped out range
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0.0, 0.0, 0.0, 0.5);
    // construct a strip of quads, with each pair being one of the corners.
    const vigra::Rect2D roi = m_visualization_state->getViewState()->GetOptions()->getROI();
    glBegin(GL_QUAD_STRIP);
        glVertex2f(0.0,     0.0);      glVertex2i(roi.left(),  roi.top());
        glVertex2f(width_o, 0.0);      glVertex2i(roi.right(), roi.top());
        glVertex2f(width_o, height_o); glVertex2i(roi.right(), roi.bottom());
        glVertex2f(0.0,     height_o); glVertex2i(roi.left(),  roi.bottom());
        glVertex2f(0.0,     0.0);      glVertex2i(roi.left(),  roi.top());
    glEnd();
    // draw lines around cropped area.
    // we want to invert the color to make it stand out.
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
        glVertex2i(roi.left(),  roi.top());
        glVertex2i(roi.right(), roi.top());
        glVertex2i(roi.right(), roi.bottom());
        glVertex2i(roi.left(),  roi.bottom());
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    
    glDisable(GL_SCISSOR_TEST);
}

void GLOverviewRenderer::Redraw()
{

    
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

    double R = m_visualization_state->getR();
    double angx = m_visualization_state->getAngX();
    double angy = m_visualization_state->getAngY();
	
	gluLookAt(R * cos(angy) * cos(angx), R * sin(angy), R * cos(angy) * sin(angx), 0, 0, 0, 0, 1, 0);

    // draw things under the preview images
    m_tool_helper->BeforeDrawImages();
    // draw each active image.
    int imgs = m_pano->getNrOfImages();
    // offset by a half a pixel
    glPushMatrix();
    glTranslatef(0.5, 0.5, 0.0);

    glColor3f(0.5,0.5,0.5);

    double side = 5;
    glBegin(GL_LINE_LOOP);

        glVertex3f(-side,side,0);
        glVertex3f(side,side,0);
        glVertex3f(side,-side,0);
        glVertex3f(-side,-side,0);

    glEnd();

    double axis = 200;
    glBegin(GL_LINES);

        glColor3f(1,0,0);
        glVertex3f(0,0,0);
        glVertex3f(axis,0,0);

        glColor3f(0,1,0);
        glVertex3f(0,0,0);
        glVertex3f(0,axis,0);

        glColor3f(0,0,1);
        glVertex3f(0,0,0);
        glVertex3f(0,0,axis);

    glEnd();


    glEnable(GL_TEXTURE_2D);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    m_tex_man->Begin();
    // The old preview shows the lowest numbered image on top, so do the same:
    for (int img = imgs - 1; img != -1; img--)
    {
        // only draw active images
        if (m_pano->getImage(img).getOptions().active)
        {
            // the tools can cancel drawing of images.
            if (m_tool_helper->BeforeDrawImageNumber(img))
            {
                // the texture manager may need to call the display list
                // multiple times with blending, so we pass it the display list
                // rather than switching to the texture and then calling the
                // list ourselves.
                m_tex_man->DrawImage(img, m_mesh_man->GetDisplayList(img));
                m_tool_helper->AfterDrawImageNumber(img);
            }
        }
    }

    #ifdef __WXGTK__
    glCullFace(GL_BACK);
    glPushMatrix();
    glRotated(90,1,0,0);
    if (imgs > 0) {
//        glEnable( GL_TEXTURE_2D );
        glEnable(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.5,0.5,0.5,0.5);
        GLUquadric* grid = gluNewQuadric();
        gluQuadricTexture(grid, GL_TRUE);
        m_tex_man->BindTexture(0);
        gluSphere(grid, 101,40,20);
        glDisable(GL_BLEND);
    } else {
        glutWireSphere(101,40,20);
    }
    glPopMatrix();
    #endif


    glCullFace(GL_BACK);
    // The old preview shows the lowest numbered image on top, so do the same:
    for (int img = imgs - 1; img != -1; img--)
    {
        // only draw active images
        if (m_pano->getImage(img).getOptions().active)
        {
            // the tools can cancel drawing of images.
            if (m_tool_helper->BeforeDrawImageNumber(img))
            {
                // the texture manager may need to call the display list
                // multiple times with blending, so we pass it the display list
                // rather than switching to the texture and then calling the
                // list ourselves.
                m_tex_man->DrawImage(img, m_mesh_man->GetDisplayList(img));
                m_tool_helper->AfterDrawImageNumber(img);
            }
        }
    }

    m_tex_man->End();
    glDisable(GL_CULL_FACE);
    // drawn things after the active image.
    m_tool_helper->AfterDrawImages();
    m_tex_man->DisableTexture();

    glPopMatrix();
}

vigra::Diff2D GLOverviewRenderer::Resize(int w, int h)
{

    width = w;
    height = h;
    glViewport(0, 0, width, height);
    // we use the view_state rather than the panorama to allow interactivity.
    HuginBase::PanoramaOptions *options = m_visualization_state->getViewState()->GetOptions();
    width_o = options->getWidth();
    height_o = options->getHeight();
    double aspect_screen = double(width) / double (height),
        aspect_pano = width_o / height_o;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();  
    double scale;
    if (aspect_screen < aspect_pano)
    {
      // the panorama is wider than the screen
      scale = width_o / width;
    } else {
      // the screen is wider than the panorama
      scale = height_o / height;
    }


	float ratio = 1.0* w / h;
//	aspect = ratio;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
	gluPerspective(m_visualization_state->getFOVY(),ratio,1,10000);

}
