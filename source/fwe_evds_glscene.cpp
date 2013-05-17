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
	//fbo_window = 0;

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

	GLC_Plane* m_pClipPlane = new GLC_Plane(GLC_Vector3d(0,1,0), GLC_Point3d(0,0,0));
	viewport->addClipPlane(GL_CLIP_PLANE0, m_pClipPlane);

	//Enable LOD
	collection->setLodUsage(true,viewport);
	viewport->setMinimumPixelCullingSize(fw_editor_settings->value("render.min_pixel_culling",4).toInt());
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
/*QGLShaderProgram* GLScene::compileShader(const QString& name) {
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
/*void GLScene::reloadShaders() {
	shader_object = compileShader("shader_object");
	shader_outline_object = compileShader("shader_outline_object");
	shader_outline_fbo = compileShader("shader_outline_fbo");
	shader_background = compileShader("shader_background");
	shader_fxaa = compileShader("shader_fxaa");
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
/*void GLScene::drawScreenQuad() {
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
		viewport->reframe(collection->boundingBox());
	}

	//Setup native rendering and viewport size
	painter->beginNativePainting();
	viewport->setWinGLSize(rect.width(), rect.height());

	//Always use orthographic view
	viewport->setToOrtho(fw_editor_settings->value("render.use_ortho_projection",true).toBool());

	//Clear buffer
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	collection->render(0, glc::ShadingFlag);

	//Depth buffer must be cleared and clip plane removed
	glClear(GL_DEPTH_BUFFER_BIT);
	viewport->useClipPlane(false);
	controller.drawActiveMoverRep(); //FIXME: is there need to force 2D after this call

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