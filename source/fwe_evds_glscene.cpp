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

#include <QPushButton>
#include <QMenu>
#include <QHBoxLayout>

#include <GLC_UserInput>
#include <GLC_Exception>
#include <GLC_Context>

#include <math.h>
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_glscene.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
///
/// Must be created under the QGraphicsScene which has correct context as viewport:
///
///		QGraphicsView* view = new QGraphicsView(scene,this);
///		QGLWidget* glwidget = new QGLWidget(new GLC_Context(QGLFormat(QGL::SampleBuffers)),view);
///		view->setViewport(glwidget);
///
////////////////////////////////////////////////////////////////////////////////
GLScene::GLScene(GLScene* in_parent_scene, QWidget *parent) : QGraphicsScene(parent) {
	sceneInitialized = false;
	parent_scene = in_parent_scene; //FIXME: support for this
	//fbo_outline = 0;
	fbo_fxaa = 0;

	//Create objects
	viewport = new GLC_Viewport();
	collection = new GLC_3DViewCollection();
	controller = GLC_Factory::instance()->createDefaultMoverController(QColor(255,30,30), viewport);

	//Create lights
	light[0] = new GLC_Light();
	light[0]->setPosition(20.0,20.0,20.0);

	//Setup signals
	connect(viewport, SIGNAL(updateOpenGL()), this, SLOT(update()));
	connect(&controller, SIGNAL(repaintNeeded()), this, SLOT(update()));

	//Default camera setup
	viewport->cameraHandle()->setDefaultUpVector(glc::Z_AXIS);
	viewport->cameraHandle()->setIsoView();

	//Make sure scene is not empty (otherwise GLC crashes)
	GLC_3DViewInstance instance(GLC_Factory::instance()->createCircle(0.0));
	collection->add(instance);

	//GLC_Plane* m_pClipPlane = new GLC_Plane(GLC_Vector3d(0,1,0), GLC_Point3d(0,0,0));
	//viewport->addClipPlane(GL_CLIP_PLANE0, m_pClipPlane);

	//Enable LOD
	collection->setLodUsage(true,viewport);
	viewport->setMinimumPixelCullingSize(fw_editor_settings->value("render.min_pixel_culling",4).toInt());

	//Create panels
	panel_control = new QWidget();
	panel_control->setWindowOpacity(0.7);
	panel_control->setLayout(new QHBoxLayout);
	panel_control->layout()->setSpacing(0);
	panel_control->layout()->setMargin(0);
	panel_control->setStyleSheet("#glview_ui_panel { background: transparent } QPushButton { min-width: 24; min-height: 24; border-width: 2; }");
	panel_control->setObjectName("glview_ui_panel");
	addWidget(panel_control);

	panel_view = new QWidget();
	panel_view->setWindowOpacity(0.7);
	panel_view->setLayout(new QHBoxLayout);
	panel_view->layout()->setSpacing(0);
	panel_view->layout()->setMargin(0);
	panel_view->setStyleSheet("#glview_ui_panel { background: transparent } QPushButton { min-width: 24; min-height: 24; border-width: 2; }");
	panel_view->setObjectName("glview_ui_panel");
	addWidget(panel_view);


	//Fill control panel with buttons
	button_center = new QPushButton(QIcon(":/icon/glview/center.png"),"");
	connect(button_center, SIGNAL(pressed()), this, SLOT(doCenter()));
	panel_control->layout()->addWidget(button_center);

	button_projection = new QPushButton(QIcon(":/icon/glview/projection_ortho.png"),"");
	connect(button_projection, SIGNAL(pressed()), this, SLOT(toggleProjection()));
	panel_control->layout()->addWidget(button_projection);

	button_shadow = new QPushButton(QIcon(":/icon/glview/render_shadow.png"),"");
	panel_control->layout()->addWidget(button_shadow);

	button_material_mode = new QPushButton(QIcon(":/icon/glview/render_wireframe.png"),"");
	panel_control->layout()->addWidget(button_material_mode);

	//Fill view panel with buttons
	QPushButton* button = new QPushButton(QIcon(":/icon/glview/view_iso.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setIsoView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_left.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setLeftView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_right.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setRightView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_top.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setTopView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_bottom.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setBottomView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_front.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setFrontView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_back.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setBackView()));
	panel_view->layout()->addWidget(button);

	/*QMenu* view_menu = new QMenu(button_view);
	button_view->setMenu(view_menu);
	QAction* action = new QAction(QIcon(":/icon/glview/view_iso.png"), tr("Isometric"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(setIsoView()));
	view_menu->addAction(action);
	action = new QAction(QIcon(":/icon/glview/view_left.png"), tr("Left"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(setLeftView()));
	view_menu->addAction(action);
	view_menu->setMinimumHeight(24);*/

	//Ortho by default
	sceneOrthographic = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
GLScene::~GLScene()
{
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::doCenter() {
	viewport->reframe(collection->boundingBox(),1.6);
}
void GLScene::toggleProjection() {
	sceneOrthographic = !sceneOrthographic;
	if (sceneOrthographic) {
		button_projection->setIcon(QIcon(":/icon/glview/projection_ortho.png"));
	} else {
		button_projection->setIcon(QIcon(":/icon/glview/projection_perspective.png"));
	}
}
void GLScene::setIsoView() {
	viewport->cameraHandle()->setIsoView();
}
void GLScene::setLeftView() {
	viewport->cameraHandle()->setLeftView();
}
void GLScene::setRightView() {
	viewport->cameraHandle()->setRightView();
}
void GLScene::setFrontView() {
	viewport->cameraHandle()->setFrontView();
}
void GLScene::setBackView() {
	viewport->cameraHandle()->setRearView();
}
void GLScene::setTopView() {
	viewport->cameraHandle()->setTopView();
}
void GLScene::setBottomView() {
	viewport->cameraHandle()->setBottomView();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::geometryChanged(const QRectF &rect) {
	QSize minSize = panel_control->layout()->minimumSize();
	panel_control->setGeometry(rect.width()/2 - minSize.width()/2,rect.y()+4,
		minSize.width(),minSize.height());

	minSize = panel_view->layout()->minimumSize();
	panel_view->setGeometry(rect.width()/2 - minSize.width()/2,rect.y()+rect.height()-minSize.height()-4,
		minSize.width(),minSize.height());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QGLShaderProgram* GLScene::compileShader(const QString& name) {
	//if (!QGLShaderProgram::hasOpenGLShaderPrograms(context())) {
		//return 0;
	//}

	QGLShader fragment(QGLShader::Fragment);
	QGLShader vertex(QGLShader::Vertex);

	//if (!fragment.compileSourceFile("../resources/" + name + ".frag")) {
	if (!fragment.compileSourceFile(":/shader/" + name + ".frag")) {
		QMessageBox::warning(0, tr("EVDS Editor"),tr("Shader error in [%1.frag]:\n%2.").arg(name).arg(fragment.log()));
		return 0;
	}
	//if (!vertex.compileSourceFile("../resources/" + name + ".vert")) {
	if (!vertex.compileSourceFile(":/shader/" + name + ".vert")) {
		QMessageBox::warning(0, tr("EVDS Editor"),tr("Shader error in [%1.vert]:\n%2.").arg(name).arg(fragment.log()));
		return 0;
	}

	QGLShaderProgram* shader = new QGLShaderProgram();
	shader->addShader(&fragment);
	shader->addShader(&vertex);
	shader->link();
	return shader;	
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::loadShaders() {
	//shader_object = compileShader("shader_object");
	//shader_outline_object = compileShader("shader_outline_object");
	//shader_outline_fbo = compileShader("shader_outline_fbo");
	shader_background = compileShader("background");
	shader_fxaa = compileShader("fxaa");
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::drawScreenQuad() {
	//Setup correct projection-view matrix (all views)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, 1.0f, 500.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTranslatef(0.0, 0.0, -1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
		glTexCoord2f( 0.0f, 1.0f);
		glVertex2f(-1, 1);
		glTexCoord2f( 1.0f, 1.0f);
		glVertex2f( 1, 1);
		glTexCoord2f( 1.0f, 0.0f);
		glVertex2f( 1,-1);
		glTexCoord2f( 0.0f, 0.0f);
		glVertex2f(-1,-1);
	glEnd();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::drawBackground(QPainter *painter, const QRectF& rect)
{
	if ((painter->paintEngine()->type() != QPaintEngine::OpenGL) &&
		(painter->paintEngine()->type() != QPaintEngine::OpenGL2)) {
		qWarning("GLScene: requires valid OpenGL context");
		return;
	}

	//Initialize scene
	if (!sceneInitialized) {
		sceneInitialized = true;

		//GLC author says this fixes "VSYNC problem under MacOS X". I don't care about macs
		#if defined(Q_OS_MAC)
		const GLint swapInterval = 1;
		CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);
		#endif

		viewport->initGl();
		previousWidth = 0;
		previousHeight = 0;
		doCenter();

		//Load shaders
		loadShaders();
	}

	//Setup native rendering and viewport size
	painter->beginNativePainting();
	viewport->setWinGLSize(rect.width(), rect.height());
	if ((rect.width() != previousWidth) || (rect.height() != previousHeight)) {
		geometryChanged(rect);
		previousWidth = rect.width();
		previousHeight = rect.height();

		if (fbo_fxaa) delete fbo_fxaa;
		//fbo_outline = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		//fbo_selected_outline = new QGLFramebufferObject(width,height,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		if (fw_editor_settings->value("rendering.use_fxaa",true) == true) {
			fbo_fxaa = new QGLFramebufferObject(previousWidth,previousHeight,QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		}
	}

	//Always use orthographic view
	viewport->setToOrtho(sceneOrthographic);

	//Start FXAA
	if (fbo_fxaa) fbo_fxaa->bind();

	//Clear screen and draw background if possible
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (shader_background) {
		shader_background->bind();
		shader_background->setUniformValue("v_baseColor",190.0f,190.0f,230.0f);
		drawScreenQuad();
		shader_background->release();
		
	}

	//Load identity matrix
    GLC_Context::current()->glcLoadIdentity();

	//Prepare rendering
	viewport->setDistMinAndMax(collection->boundingBox()); //Clipping planes defined by bounding box
	light[0]->glExecute(); //Scene light #1
	viewport->glExecuteCam(); //Camera
	viewport->useClipPlane(true); //Enable section plane

	//Render scene
	if (fw_editor_settings->value("render.use_wireframe",false).toBool()) {
		collection->setPolygonModeForAll(GL_FRONT_AND_BACK, GL_LINE);
	}
	//collection->setVboUsage(false);
	collection->render(0, glc::ShadingFlag);

	//Depth buffer must be cleared and clip plane removed
	glClear(GL_DEPTH_BUFFER_BIT);
	viewport->useClipPlane(false);

	//Draw controller UI
	controller.drawActiveMoverRep(); //FIXME: is there need to force 2D after this call
	
	//End FXAA and display it on screen
	if (fbo_fxaa) {
		fbo_fxaa->release();

		glBindTexture(GL_TEXTURE_2D, fbo_fxaa->texture());
		shader_fxaa->bind();
		shader_fxaa->setUniformValue("textureSampler",0);
		shader_fxaa->setUniformValue("texcoordOffset",1.0f/((float)rect.width()),1.0f/((float)rect.height()));
		drawScreenQuad();
		shader_fxaa->release();
	}

	//Finish native rendering
	painter->endNativePainting();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::mousePressEvent(QGraphicsSceneMouseEvent* e) {
	QGraphicsScene::mousePressEvent(e);
	if (e->isAccepted()) return;
	if (controller.hasActiveMover()) return;

	//Move the view
	int x = e->scenePos().x();
	int y = e->scenePos().y();
	switch (e->button()) {
		case (Qt::RightButton):
			controller.setActiveMover(GLC_MoverController::TrackBall, GLC_UserInput(x,y));
			update();
			break;
		case (Qt::LeftButton):
			controller.setActiveMover(GLC_MoverController::Pan, GLC_UserInput(x,y));
			update();
			break;
		case (Qt::MidButton):
			controller.setActiveMover(GLC_MoverController::Zoom, GLC_UserInput(x,y));
			update();
			break;
		default:
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
	QGraphicsScene::mouseMoveEvent(e);
	if (e->isAccepted()) return;

	//Keep moving the view
	int x = e->scenePos().x();
	int y = e->scenePos().y();
	if (controller.hasActiveMover()) {
		controller.move(GLC_UserInput(x,y));
		//viewport->setDistMinAndMax(collection->boundingBox());
		update();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
	QGraphicsScene::mouseReleaseEvent(e);
	if (e->isAccepted()) return;

	//Stop moving the view
	if (controller.hasActiveMover()) {
		controller.setNoMover();
		update();
	}
}