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

#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_glwidget.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
GLWidget::GLWidget(Object* in_root, const QGLFormat& format, QWidget *parent)
	: QGLWidget(format, parent)
{
	root = in_root;
	zZoom = 3.0f;
	antialiasingEnabled = true;
	clip_plane = QVector4D(0,0,0,0);

	//Initialize camera
	EVDS_QUATERNION delta_quaternion;
	EVDS_Quaternion_SetEuler(&current_quaternion,0,EVDS_RAD(0),EVDS_RAD(0),EVDS_RAD(0));
	EVDS_Quaternion_SetEuler(&delta_quaternion,0,0,-EVDS_RAD(45),0);
	EVDS_Quaternion_Multiply(&current_quaternion,&current_quaternion,&delta_quaternion);
	EVDS_Quaternion_SetEuler(&delta_quaternion,0,-EVDS_RAD(32.264),0,0);
	EVDS_Quaternion_Multiply(&current_quaternion,&current_quaternion,&delta_quaternion);

	//Reset camera position
	EVDS_Vector_Set(&current_offset,0,0,0,0,0);
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
QGLShaderProgram* GLWidget::compileShader(const QString& name) {
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
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::initializeGL() {
	//Load shaders
	reloadShaders();
	
	//Create framebuffer
	//fbo_outline = new QGLFramebufferObject(width(),height(),QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
	//fbo_window = new QGLFramebufferObject(width(),height(),QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
	fbo_outline = 0;
	fbo_selected_outline = 0;
	fbo_window = 0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::reloadShaders() {
	shader_object = compileShader("shader_object");
	shader_outline_object = compileShader("shader_outline_object");
	shader_outline_fbo = compileShader("shader_outline_fbo");
	shader_background = compileShader("shader_background");
	shader_fxaa = compileShader("shader_fxaa");
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::drawObject(Object* object, bool drawOutline, bool onlySelected) {
	glPushMatrix();

	//Transform to current vessels coordinates
	EVDS_STATE_VECTOR vector;
	EVDS_Object_GetStateVector(object->getEVDSObject(),&vector);
	glTranslatef(vector.position.x,vector.position.y,vector.position.z);

	//Get and fix quaternion
	float opengl_matrix[16];
	EVDS_MATRIX Qmatrix;
	EVDS_Quaternion_ToMatrix(&vector.orientation,Qmatrix);
	for (int i=0; i<4; i++) { //Transpose matrix for OpenGL
		for (int j=0; j<4; j++)	{
			opengl_matrix[i*4+j] = Qmatrix[j*4+i];
		}
	}
	glMultMatrixf(opengl_matrix);

	//Check if object is selected
	EVDS::Editor* editor = static_cast<EVDS::Editor*>(parentWidget());
	bool isSelected = editor->getSelected() == object;

	//Check if child is selected
	//bool isChildSelected = false;
	//for (int i = 0; i < object->getChildrenCount(); i++) {
		//if (editor->getSelected() == object->getChild(i)) isChildSelected = true;
	//}

	//Avoid drawing root object
	if (object != root) {
		if ((!onlySelected) || (onlySelected && isSelected)) {
		//if ((onlySelected && isSelected) ||
			//((!onlySelected) && (!isSelected)) ||
			//(!drawOutline)) {
			object->draw(isSelected);
		}
	}

	//Draw children
	for (int i = 0; i < object->getChildrenCount(); i++) {
		drawObject(object->getChild(i),drawOutline,onlySelected);
	}
	glPopMatrix();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::resizeGL(int width, int height)
{
	if (fbo_outline) delete fbo_outline;
	if (fbo_selected_outline) delete fbo_selected_outline;
	if (fbo_window) delete fbo_window;
	fbo_outline = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
	fbo_selected_outline = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
	fbo_window = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::enterGLState(DrawPhase phase) {
	float aspect = ((float)width())/((float)height());

	//Setup correct projection-view matrix (all views)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-zZoom*aspect, zZoom*aspect, -zZoom,zZoom, 1.0f, 500.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	switch (phase) {
		case GLWidget::Background:
			glClearColor(0.95f,0.95f,0.95f,1.00f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glTranslatef(0.0, 0.0, -1.0);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
		break;
		case GLWidget::DrawOutline:
			glTranslatef(0.0, 0.0, -1.0);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_TEXTURE_2D);
		break;
		case GLWidget::DrawFXAA:
			glTranslatef(0.0, 0.0, -1.0);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_TEXTURE_2D);
		break;
		case GLWidget::RenderOutline:
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
		break;
		case GLWidget::Scene:
			glClear(GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
		break;
	}

	//For 3D rendering also set up correct camera
	if (phase >= GLWidget::RenderOutline) {
		//Offset from screen for orhogonal rendering
		glTranslatef(0.0, 0.0, -100.0);
	
		//Setup camera orientation
		EVDS_MATRIX Qmatrix,GLmatrix;
		EVDS_Quaternion_ToMatrix(&current_quaternion,Qmatrix);
		EVDS_Matrix_Transpose(GLmatrix,Qmatrix); //Convert to OpenGL format
		glMultMatrixd(GLmatrix);

		//Setup camera translation
		glTranslatef(current_offset.x,current_offset.y,current_offset.z);

		//Store current matrix
		glGetDoublev(GL_MODELVIEW_MATRIX,current_matrix);
		glLoadIdentity();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::drawScreenQuad() {
	float aspect = ((float)width())/((float)height());
	glBegin(GL_QUADS);
		glTexCoord2f( 0.0f, 1.0f);
		glVertex2f(-zZoom*aspect,zZoom);
		glTexCoord2f( 1.0f, 1.0f);
		glVertex2f(zZoom*aspect,zZoom);
		glTexCoord2f( 1.0f, 0.0f);
		glVertex2f(zZoom*aspect,-zZoom);
		glTexCoord2f( 0.0f, 0.0f);
		glVertex2f(-zZoom*aspect,-zZoom);
	glEnd();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::paintGL()
{
	//Get clip plane
	//QVector4D clip_plane = QVector4D(0,0,-1,0); //0,1,0,1e7
	//QVector4D clip_plane = QVector4D(0,1,0,1e7);

	//Setup viewport
	glViewport(0,0,width(),height());

	//Draw background
	enterGLState(GLWidget::Background);
	if (shader_background) {
		if (fbo_window && antialiasingEnabled) fbo_window->bind();
		shader_background->bind();
		shader_background->setUniformValue("v_baseColor",190.0f,190.0f,230.0f);
		drawScreenQuad();
		shader_background->release();
		if (fbo_window && antialiasingEnabled) fbo_window->release();
	}

	//Draw outline for normal objects
	if (shader_outline_object && fbo_outline) {
		fbo_outline->bind();
			enterGLState(GLWidget::RenderOutline);
			shader_outline_object->bind();
			shader_object->setUniformValue("m_View",QMatrix4x4(current_matrix).transposed());
			shader_object->setUniformValue("v_ClipPlane",clip_plane);
			drawObject(root,true,false);

			//glClear(GL_DEPTH_BUFFER_BIT);
			//drawObject(root,true);
			shader_outline_object->release();
		fbo_outline->release();
	}

	//Draw outline for selected objects
	if (shader_outline_object && fbo_selected_outline) {
		fbo_selected_outline->bind();
			enterGLState(GLWidget::RenderOutline);
			shader_outline_object->bind();
			shader_object->setUniformValue("m_View",QMatrix4x4(current_matrix).transposed());
			shader_object->setUniformValue("v_ClipPlane",clip_plane);
			drawObject(root,true,true);			
			shader_outline_object->release();
		fbo_selected_outline->release();
	}

	//Start drawing into window
	if (fbo_window && antialiasingEnabled) fbo_window->bind();

	//Draw scene
	enterGLState(GLWidget::Scene);
	if (shader_object) {
		shader_object->bind();
		shader_object->setUniformValue("m_View",QMatrix4x4(current_matrix).transposed());
		shader_object->setUniformValue("v_ClipPlane",clip_plane);
		drawObject(root,false,false);
		shader_object->release();
	}

	//Draw scene
	if (shader_outline_fbo && fbo_outline) {
		enterGLState(GLWidget::DrawOutline);

		//Draw outlines of other objects
		glBindTexture(GL_TEXTURE_2D, fbo_outline->texture());
		shader_outline_fbo->bind();
		shader_outline_fbo->setUniformValue("s_Data",0);
		shader_outline_fbo->setUniformValue("v_invScreenSize",1.0f/((float)width()),1.0f/((float)height()));
		shader_outline_fbo->setUniformValue("f_outlineThickness",1.0f);
		drawScreenQuad();
		shader_outline_fbo->release();

		//Draw outlines of selected objects
		glBindTexture(GL_TEXTURE_2D, fbo_selected_outline->texture());
		shader_outline_fbo->bind();
		shader_outline_fbo->setUniformValue("s_Data",0);
		shader_outline_fbo->setUniformValue("v_invScreenSize",1.0f/((float)width()),1.0f/((float)height()));
		shader_outline_fbo->setUniformValue("f_outlineThickness",1.0f);
		drawScreenQuad();
		shader_outline_fbo->release();
	}

	//End drawing into window
	if (fbo_window && antialiasingEnabled) {
		fbo_window->release();

		enterGLState(GLWidget::DrawFXAA);
		glBindTexture(GL_TEXTURE_2D, fbo_window->texture());
		shader_fxaa->bind();
		shader_fxaa->setUniformValue("textureSampler",0);
		shader_fxaa->setUniformValue("texcoordOffset",1.0f/((float)width()),1.0f/((float)height()));
		drawScreenQuad();
		shader_outline_fbo->release();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton) {
		if (event->modifiers() & Qt::ControlModifier) {
			EVDS_QUATERNION delta_quaternion;
			EVDS_Quaternion_SetEuler(&delta_quaternion,0,0,0,EVDS_RAD(0.2f*(dx+dy)));
			EVDS_Quaternion_Multiply(&current_quaternion,&current_quaternion,&delta_quaternion);
		} else {
			EVDS_QUATERNION delta_quaternion;
			EVDS_Quaternion_SetEuler(&delta_quaternion,0,0,-EVDS_RAD(0.2f*dx),0);
			EVDS_Quaternion_Multiply(&current_quaternion,&current_quaternion,&delta_quaternion);
			EVDS_Quaternion_SetEuler(&delta_quaternion,0,EVDS_RAD(0.2f*dy),0,0);
			EVDS_Quaternion_Multiply(&current_quaternion,&current_quaternion,&delta_quaternion);
		}

		updateGL();
	}
	if (event->buttons() & Qt::RightButton) {
		float scale = 2.0 / ((float)height());

		EVDS_VECTOR delta_vector;
		EVDS_Vector_Set(&delta_vector,0,0,dx*scale*zZoom,-dy*scale*zZoom,0);
		EVDS_Vector_Rotate(&delta_vector,&delta_vector,&current_quaternion);
		EVDS_Vector_Add(&current_offset,&current_offset,&delta_vector);

		updateGL();
	}
	lastPos = event->pos();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLWidget::wheelEvent(QWheelEvent *event) {
    float step = (event->modifiers() & Qt::ControlModifier) ? 5.0f : 1.0f;
	zZoom += step * event->delta() / (150*4);
	if (zZoom < 0.1f) zZoom = 0.1f;

	updateGL();
}
