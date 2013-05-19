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
#ifndef FWE_EVDS_GLSCENE_H
#define FWE_EVDS_GLSCENE_H

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
	class GLScene : public QGraphicsScene
	{
		Q_OBJECT

	public:
		GLScene(GLScene* in_parent_scene, QWidget *parent = 0);
		~GLScene();

		QSize minimumSizeHint() const { return QSize(50, 50); }
		QSize sizeHint() const { return QSize(400, 400); }
		GLC_3DViewCollection* getCollection() { return world->collection(); }

		QGLShaderProgram* compileShader(const QString& name);
		void loadShaders();
		void drawScreenQuad();
		//void selectByCoordinates(int x, int y, bool multi, QMouseEvent* pMouseEvent);

	protected:
		void geometryChanged(const QRectF &rect);
		void drawBackground(QPainter *painter, const QRectF &rect);
		void mousePressEvent(QGraphicsSceneMouseEvent *event);
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

	private slots:
		void doCenter();
		void toggleProjection();
		void setIsoView();
		void setLeftView();
		void setRightView();
		void setFrontView();
		void setBackView();
		void setTopView();
		void setBottomView();

	private:
		//Parent scene from which GLC stuff is taken
		GLScene* parent_scene;

		//UI elements
		QWidget* panel_control;
		QWidget* panel_view;
		QPushButton* button_center;
		QPushButton* button_projection;
		QPushButton* button_shadow;
		QPushButton* button_material_mode;

		//GLC Specific
		GLC_Light* light[1];
		GLC_World* world;
		GLC_Viewport* viewport;
		GLC_MoverController controller;

		//Is scene initialized OpenGL-wise
		bool sceneOrthographic;
		bool sceneInitialized;
		int previousWidth,previousHeight;

		//Shaders and framebuffers
		QGLFramebufferObject* fbo_outline;
		QGLFramebufferObject* fbo_fxaa;
		QGLShaderProgram* shader_background;
		QGLShaderProgram* shader_outline;
		QGLShaderProgram* shader_fxaa;
	};

	class GLView : public QGraphicsView
	{
		Q_OBJECT

	public:
		GLView(QWidget* parent) : QGraphicsView(parent) {
			QGLContext* context = new GLC_Context(QGLFormat(QGL::SampleBuffers));
			QGLWidget* opengl = new QGLWidget(context,this);

			setViewport(opengl);
			setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
		}

	protected:
		void resizeEvent(QResizeEvent *event) {
			if (scene())
				scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
			QGraphicsView::resizeEvent(event);
		}
	};
}

#endif
