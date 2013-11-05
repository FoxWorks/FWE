////////////////////////////////////////////////////////////////////////////////
/// @file
////////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2012-2013, Black Phoenix
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see http://www.gnu.org/licenses/.
////////////////////////////////////////////////////////////////////////////////
#ifndef FWE_EVDS_GLSCENE_H
#define FWE_EVDS_GLSCENE_H

#include <QProgressDialog>
#include <QGLWidget>
#include <QGLShader>
#include <QGLFramebufferObject>

#include <GLC_Factory>
#include <GLC_Light>
#include <GLC_Viewport>
#include <GLC_3DViewCollection>
#include <GLC_3DWidgetManager>
#include <GLC_MoverController>

namespace EVDS {
	class Object;
	class Editor;
	class SchematicsEditor;
	class GLScene : public QGraphicsScene
	{
		Q_OBJECT

	public:
		GLScene(GLScene* in_parent_scene, Editor* in_editor, SchematicsEditor* in_schematics_editor, QWidget *parent = 0);
		~GLScene();

		GLC_3DViewCollection* getCollection() { return world->collection(); }

		QGLShaderProgram* compileShader(const QString& name);
		void loadShaders();
		void drawScreenQuad();
		//void selectByCoordinates(int x, int y, bool multi, QMouseEvent* pMouseEvent);

		void setCutsectionPlane(int plane, bool active);

	protected:
		void geometryChanged(const QRectF &rect);
		void drawBackground(QPainter *painter, const QRectF &rect);
		void mousePressEvent(QGraphicsSceneMouseEvent *event);
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

	public slots:
		void doCenter();
		void toggleProjection();
		void toggleShadow();
		void toggleMaterialMode();
		void saveScreenshot();
		void saveSheets();
		void setIsoView();
		void setLeftView();
		void setRightView();
		void setFrontView();
		void setBackView();
		void setTopView();
		void setBottomView();

		void cutsectionUpdated();

	private:
		//Save a single snapshot of a sheet
		void saveCurrentSheet(const QString& baseFilename);
		//Create panels and interface
		void createInterface();
		//Recursively GLC-select object and its children
		void recursiveSelect(Object* object);
		//Draw schematics page layout
		void drawSchematicsPage(QPainter *painter);
		//Draw schematics page element
		void drawSchematicsElement(QPainter *painter, Object* element, QPointF offset);
		//Project coordinates
		QPointF project(float x, float y, float z = 0.0);

		//Parent scene from which GLC stuff is taken
		GLScene* parent_scene;
		Editor* editor;
		SchematicsEditor* schematics_editor;

		QProgressDialog* progressDialog;

		//UI elements
		QWidget* panel_control;
		QWidget* panel_view;
		QPushButton* button_center;
		QPushButton* button_projection;
		QPushButton* button_shadow;
		QPushButton* button_material_mode;
		QPushButton* button_save_picture;
		QPushButton* button_save_sheets;

		//GLC Specific
		GLC_Light* light[1];
		GLC_World* world;
		GLC_Viewport* viewport;
		GLC_MoverController controller;
		GLC_3DViewInstance* indicator_cm;
		GLC_3DWidgetManager* widget_manager;
		GLC_Plane* cutsectionPlane[3];
		int cutsectionPlaneWidget[3];

		//Is scene initialized OpenGL-wise
		bool sceneOrthographic;
		bool sceneShadowed;
		bool sceneWireframe;
		bool sceneInitialized;
		bool makingScreenshot;
		QRectF previousRect;

		//Shaders and framebuffers
		QGLFramebufferObject* fbo_outline;
		QGLFramebufferObject* fbo_outline_selected;
		QGLFramebufferObject* fbo_shadow;
		QGLFramebufferObject* fbo_fxaa;
		QGLShaderProgram* shader_background;
		QGLShaderProgram* shader_outline;
		QGLShaderProgram* shader_shadow;
		QGLShaderProgram* shader_fxaa;
	};

	class GLView : public QGraphicsView
	{
		Q_OBJECT

	public:
		QSize minimumSizeHint() const { return QSize(200, 100); }
		QSize sizeHint() const { return QSize(200, 200); }

		GLView(QWidget* parent) : QGraphicsView(parent) {
			QGLContext* context = new GLC_Context(QGLFormat(QGL::SampleBuffers));
			QGLWidget* opengl = new QGLWidget(context,this);

			setAcceptDrops(true);
			setViewport(opengl);
			setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
		}

	protected:
		void resizeEvent(QResizeEvent *event) {
			if (scene())
				scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
			QGraphicsView::resizeEvent(event);
		}
		void dropEvent(QDropEvent *event) {
		}
		void dragMoveEvent(QDragMoveEvent *event) {
		}
		void dragEnterEvent(QDragEnterEvent *event) {
		}
	};
}

#endif
