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
#ifndef FWE_EVDS_GLWIDGET_H
#define FWE_EVDS_GLWIDGET_H

#include <QGLWidget>
#include <QGLShader>
#include <QGLFramebufferObject>

#include <GLC_Factory>
#include <GLC_Light>
#include <GLC_Viewport>
#include <GLC_3DViewCollection>
#include <GLC_MoverController>

namespace EVDS {
	class Object;
	class GLWidget : public QGLWidget
	{
		Q_OBJECT

	public:
		GLWidget(Object* in_root, QWidget *parent = 0);
		~GLWidget();

		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void setAntialiasing(bool enable) { antialiasingEnabled = enable; update(); }
		GLC_3DViewCollection* getCollection() { return &m_Collection; }

	protected:
		//void drawScreenQuad();
		//QGLShaderProgram* compileShader(const QString& name);

		void initializeGL();
		void paintGL();
		void resizeGL(int width, int height);
		
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);

	private:
		Object* root;

		//GLC Specific
		GLC_Light m_Light;
		GLC_3DViewCollection m_Collection;
		GLC_Viewport m_GlView;
		GLC_MoverController m_MoverController;

		//Shaders and framebuffers
		bool antialiasingEnabled;
		//QGLFramebufferObject* fbo_outline;
		//QGLFramebufferObject* fbo_selected_outline;
		//QGLFramebufferObject* fbo_window;
		//QGLShaderProgram* shader_outline_object;
		//QGLShaderProgram* shader_outline_fbo;
		//QGLShaderProgram* shader_background;
		//QGLShaderProgram* shader_fxaa;
	};
}

#endif
