////////////////////////////////////////////////////////////////////////////////
/// @file
////////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2012-2013, Black Phoenix
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions are met:
///   - Redistributions of source code must retain the above copyright
///     notice, this list of conditions and the following disclaimer.
///   - Redistributions in binary form must reproduce the above copyright
///     notice, this list of conditions and the following disclaimer in the
///     documentation and/or other materials provided with the distribution.
///   - Neither the name of the author nor the names of the contributors may
///     be used to endorse or promote products derived from this software without
///     specific prior written permission.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
/// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
/// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
/// DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS BE LIABLE FOR ANY
/// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
/// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
/// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
/// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
/// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////
#include <QWidget>
#include <QtOpenGL>
#include <QMessageBox>
#include <math.h>

#include <GLC_UserInput>
#include <GLC_Exception>
#include <GLC_Context>

#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_glwidget.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
GLWidget::GLWidget(Object* in_root, QWidget *parent)
	//: QGLWidget(parent)
	: QGLWidget(new GLC_Context(QGLFormat(QGL::SampleBuffers)), parent)
, m_Light()
, m_Collection()
, m_GlView()
, m_MoverController()
{
	connect(&m_GlView, SIGNAL(updateOpenGL()), this, SLOT(updateGL()));
	m_Light.setPosition(1.0, 1.0, 1.0);
	QColor repColor;
	repColor.setRgbF(1.0, 0.11372, 0.11372, 1.0);
	m_MoverController= GLC_Factory::instance()->createDefaultMoverController(repColor, &m_GlView);
	m_GlView.cameraHandle()->setDefaultUpVector(glc::Z_AXIS);
	m_GlView.cameraHandle()->setIsoView();
	
	//Setup scene
	//for (int i= 0; i < 10; ++i)
	//{
		GLC_3DViewInstance instance(GLC_Factory::instance()->createCircle(0.0));
		instance.geomAt(0)->setWireColor(Qt::white);
		m_Collection.add(instance);
	//}

	//m_GlView.addClipPlane(GL_CLIP_PLANE0,new GLC_Plane(0,1,0,0));
	//m_GlView.useClipPlane(true);
	//m_Collection.setLodUsage(true,&m_GlView);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
GLWidget::~GLWidget()
{
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QSize GLWidget::minimumSizeHint() const
{
	return QSize(50, 50);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QSize GLWidget::sizeHint() const
{
	return QSize(400, 400);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
/*QGLShaderProgram* GLWidget::compileShader(const QString& name) {
	if (!QGLShaderProgram::hasOpenGLShaderPrograms(context())) {
		return 0;
	}

	QGLShader fragment(QGLShader::Fragment);
	QGLShader vertex(QGLShader::Vertex);

	//if (!fragment.compileSourceFile("../source/foxworks_editor/resources/" + name + ".frag")) {
	if (!fragment.compileSourceFile(":/" + name + ".frag")) {
		QMessageBox::warning(0, tr("EVDS Editor"),tr("Shader error in [%1.frag]:\n%2.").arg(name).arg(fragment.log()));
		return 0;
	}
	//if (!vertex.compileSourceFile("../source/foxworks_editor/resources/" + name + ".vert")) {
	if (!vertex.compileSourceFile(":/" + name + ".vert")) {
		QMessageBox::warning(0, tr("EVDS Editor"),tr("Shader error in [%1.vert]:\n%2.").arg(name).arg(fragment.log()));
		return 0;
	}

	QGLShaderProgram* shader = new QGLShaderProgram(context());
	shader->addShader(&fragment);
	shader->addShader(&vertex);
	shader->link();
	return shader;	
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::initializeGL() {
	//Load shaders
	//reloadShaders();
	
	//Create framebuffer
	//fbo_outline = 0;
	//fbo_selected_outline = 0;
	//fbo_window = 0;

	// For VSYNC problem under Mac OS X
	#if defined(Q_OS_MAC)
	const GLint swapInterval = 1;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);
	#endif

	m_GlView.initGl();
	m_GlView.reframe(m_Collection.boundingBox());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
/*void GLWidget::reloadShaders() {
	shader_object = compileShader("shader_object");
	shader_outline_object = compileShader("shader_outline_object");
	shader_outline_fbo = compileShader("shader_outline_fbo");
	shader_background = compileShader("shader_background");
	shader_fxaa = compileShader("shader_fxaa");
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::resizeGL(int width, int height)
{
	m_GlView.setWinGLSize(width, height);
	//if (fbo_outline) delete fbo_outline;
	//if (fbo_selected_outline) delete fbo_selected_outline;
	//if (fbo_window) delete fbo_window;
	//fbo_outline = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
	//fbo_selected_outline = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
	//fbo_window = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
/*void GLWidget::drawScreenQuad() {
	//Setup correct projection-view matrix (all views)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1, 1.0f, 500.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTranslatef(0.0, 0.0, -1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	float aspect = ((float)width())/((float)height());
	glBegin(GL_QUADS);
		glTexCoord2f( 0.0f, 1.0f);
		glVertex2f(-1,1);
		glTexCoord2f( 1.0f, 1.0f);
		glVertex2f(1,1);
		glTexCoord2f( 1.0f, 0.0f);
		glVertex2f(1,-1);
		glTexCoord2f( 0.0f, 0.0f);
		glVertex2f(-1,-1);
	glEnd();
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::paintGL()
{
	// Clear screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Load identity matrix
    GLC_Context::current()->glcLoadIdentity();

	try
	{
		// Set Opengl clipping planes
		m_GlView.setDistMinAndMax(m_Collection.boundingBox());

		// define the light
		m_Light.glExecute();

		// define view matrix
		m_GlView.glExecuteCam();

		// Display the collection of GLC_Object
		m_Collection.render(0, glc::ShadingFlag);

		// Display UI Info (orbit circle)
		m_MoverController.drawActiveMoverRep();
	}
	catch (GLC_Exception &e)
	{
		qDebug() << e.what();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::mousePressEvent(QMouseEvent* e) {
	if (m_MoverController.hasActiveMover()) return;
	switch (e->button())
	{
	case (Qt::RightButton):
		m_MoverController.setActiveMover(GLC_MoverController::TrackBall, GLC_UserInput(e->x(), e->y()));
		updateGL();
		break;
	case (Qt::LeftButton):
		m_MoverController.setActiveMover(GLC_MoverController::Pan, GLC_UserInput(e->x(), e->y()));
		updateGL();
		break;
	case (Qt::MidButton):
		m_MoverController.setActiveMover(GLC_MoverController::Zoom, GLC_UserInput(e->x(), e->y()));
		updateGL();
		break;

	default:
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::mouseMoveEvent(QMouseEvent* e) {
	if (m_MoverController.hasActiveMover())
	{
		m_MoverController.move(GLC_UserInput(e->x(), e->y()));
		m_GlView.setDistMinAndMax(m_Collection.boundingBox());
		updateGL();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::mouseReleaseEvent(QMouseEvent* e) {
	if (m_MoverController.hasActiveMover())
	{
		m_MoverController.setNoMover();
		updateGL();
	}
}