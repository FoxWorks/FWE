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
#include <GLC_CuttingPlane>

#include <math.h>
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_renderer.h"
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
GLScene::GLScene(GLScene* in_parent_scene, Editor* in_editor, QWidget *parent) : QGraphicsScene(parent) {
	parent_scene = in_parent_scene; //FIXME: support for this
	editor = in_editor;

	//Have everything be initialized later
	sceneInitialized = false;
	fbo_outline = 0;
	fbo_outline_selected = 0;
	fbo_shadow = 0;
	fbo_fxaa = 0;

	cutsectionPlaneWidget[0] = 0;
	cutsectionPlaneWidget[1] = 0;
	cutsectionPlaneWidget[2] = 0;

	//Create GLC objects
	viewport = new GLC_Viewport();
	controller = GLC_Factory::instance()->createDefaultMoverController(QColor(255,30,30), viewport);
	world = new GLC_World();
	widget_manager = new GLC_3DWidgetManager(viewport);

	//GLC scene cannot be empty, either it crashes. Also ensure minimum size of bounding box
	GLC_3DViewInstance instance1(GLC_Factory::instance()->createCircle(0.0));
	instance1.translate(-1,-1,-1);
	world->collection()->add(instance1);
	GLC_3DViewInstance instance2(GLC_Factory::instance()->createCircle(0.0));
	instance2.translate(1,1,1);
	world->collection()->add(instance2);

	//Add center of mass indicator
	GLC_PointSprite* sprite = new GLC_PointSprite(16.0f,new GLC_Material(new GLC_Texture(":/icon/glview/cm.png")));
	indicator_cm = new GLC_3DViewInstance(sprite);
	indicator_cm->translate(3.0,0,0);

	//Create lights
	light[0] = new GLC_Light();
	light[0]->setPosition(20.0,20.0,20.0);
	light[0]->setTwoSided(true);

	//Setup signals
	connect(viewport, SIGNAL(updateOpenGL()), this, SLOT(update())); //FIXME: use render() instead
	connect(&controller, SIGNAL(repaintNeeded()), this, SLOT(update()));

	//Enable LOD, setup default camera
	viewport->cameraHandle()->setDefaultUpVector(glc::Z_AXIS);
	viewport->cameraHandle()->setIsoView();
	world->collection()->setLodUsage(true,viewport);
	world->collection()->setVboUsage(true);
	viewport->setMinimumPixelCullingSize(fw_editor_settings->value("rendering.min_pixel_culling").toInt());
	GLC_SelectionMaterial::setUseSelectionMaterial(false);

	//Add clipping plane
	//GLC_Plane* m_pClipPlane = new GLC_Plane(GLC_Vector3d(0,1,0), GLC_Point3d(0,0,0));
	//viewport->addClipPlane(GL_CLIP_PLANE0, m_pClipPlane);

	//Default modes
	sceneOrthographic = true;
	sceneShadowed = false;
	sceneWireframe = false;
	makingScreenshot = false;

	//Create interface and enable drag and drop
	createInterface();
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
void GLScene::createInterface() {
	//Create panels
	panel_control = new QWidget();
	panel_control->setWindowOpacity(0.7);
	panel_control->setLayout(new QHBoxLayout);
	panel_control->layout()->setSpacing(0);
	panel_control->layout()->setMargin(0);
	panel_control->setStyleSheet("#glview_ui_panel { background: transparent } QPushButton { min-width: 24; min-height: 24; }");
	panel_control->setObjectName("glview_ui_panel");
	addWidget(panel_control);

	panel_view = new QWidget();
	panel_view->setWindowOpacity(0.7);
	panel_view->setLayout(new QHBoxLayout);
	panel_view->layout()->setSpacing(0);
	panel_view->layout()->setMargin(0);
	panel_view->setStyleSheet("#glview_ui_panel { background: transparent } QPushButton { min-width: 24; min-height: 24; }");
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
	connect(button_shadow, SIGNAL(pressed()), this, SLOT(toggleShadow()));
	panel_control->layout()->addWidget(button_shadow);

	button_material_mode = new QPushButton(QIcon(":/icon/glview/render_shaded.png"),"");
	connect(button_material_mode, SIGNAL(pressed()), this, SLOT(toggleMaterialMode()));
	panel_control->layout()->addWidget(button_material_mode);

	button_save_picture = new QPushButton(QIcon(":/icon/glview/render_screenshot.png"),"");
	connect(button_save_picture, SIGNAL(pressed()), this, SLOT(saveScreenshot()));
	panel_control->layout()->addWidget(button_save_picture);
	

	//Fill view panel with buttons
	QPushButton* button = new QPushButton(QIcon(":/icon/glview/view_iso.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setIsoView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_front.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setFrontView()));
	panel_view->layout()->addWidget(button);

	button = new QPushButton(QIcon(":/icon/glview/view_back.png"),"");
	button->setIconSize(QSize(20,20));
	connect(button, SIGNAL(pressed()), this, SLOT(setBackView()));
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

	/*QMenu* view_menu = new QMenu(button_view);
	button_view->setMenu(view_menu);
	QAction* action = new QAction(QIcon(":/icon/glview/view_iso.png"), tr("Isometric"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(setIsoView()));
	view_menu->addAction(action);
	action = new QAction(QIcon(":/icon/glview/view_left.png"), tr("Left"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(setLeftView()));
	view_menu->addAction(action);
	view_menu->setMinimumHeight(24);*/
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::doCenter() {
	viewport->reframe(world->collection()->boundingBox(),1.6);
}
void GLScene::toggleProjection() {
	sceneOrthographic = !sceneOrthographic;
	if (sceneOrthographic) {
		button_projection->setIcon(QIcon(":/icon/glview/projection_ortho.png"));
	} else {
		button_projection->setIcon(QIcon(":/icon/glview/projection_perspective.png"));
	}
}
void GLScene::toggleShadow() {
	sceneShadowed = !sceneShadowed;
}
void GLScene::toggleMaterialMode() {
	sceneWireframe = !sceneWireframe;
	if (sceneWireframe) {
		button_material_mode->setIcon(QIcon(":/icon/glview/render_wireframe.png"));
	} else {
		button_material_mode->setIcon(QIcon(":/icon/glview/render_shaded.png"));
	}
}
void GLScene::saveScreenshot() {
	QGLFramebufferObjectFormat format;
	format.setSamples(16);

	//Determine best maximum size
	float aspectRatio = previousRect.height() / previousRect.width();
	int width = fw_editor_settings->value("screenshot.width").toInt();
	int height = width*aspectRatio;

	if (fw_editor_settings->value("screenshot.height").toInt() > 0) {
		height = fw_editor_settings->value("screenshot.height").toInt();
	} else {
		if (height > fw_editor_settings->value("screenshot.width").toInt()) {
			height = fw_editor_settings->value("screenshot.width").toInt();
			width = height/aspectRatio;
		}
	}
	
	//Create FBO and draw into it
	QGLFramebufferObject renderFbo(width,height);
	QPainter fboPainter(&renderFbo);
	fboPainter.setRenderHint(QPainter::Antialiasing);
	fboPainter.setRenderHint(QPainter::HighQualityAntialiasing);

		QRectF oldRect = sceneRect();
		setSceneRect(QRectF(0,0,width,height));
		panel_control->hide();
		panel_view->hide();
			makingScreenshot = true;
			render(&fboPainter);
			makingScreenshot = false;
		setSceneRect(oldRect);
		panel_control->show();
		panel_view->show();

	fboPainter.end();

	//Get image and save it
	QString fileName = QFileDialog::getSaveFileName(static_cast<QWidget*>(this->parent()), "Save Screenshot", "",
		"JPEG/PNG image (*.jpg;*.png);;"
		"All files (*.*)");

	if (!fileName.isEmpty()) {
		renderFbo.toImage().save(fileName,0,95);
	}
}
void GLScene::setIsoView() {
	viewport->cameraHandle()->setIsoView();
}
void GLScene::setLeftView() {
	viewport->cameraHandle()->setFrontView();
}
void GLScene::setRightView() {
	viewport->cameraHandle()->setRearView();
}
void GLScene::setFrontView() {
	viewport->cameraHandle()->setLeftView();
}
void GLScene::setBackView() {
	viewport->cameraHandle()->setRightView();
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
void GLScene::cutsectionUpdated() {
	/*GLC_CuttingPlane* widget = dynamic_cast<GLC_CuttingPlane*>(sender());
	int plane = widget->property("plane").toInt();
	
	if (NULL != widget) {
		cutsectionPlane[plane]->setPlane(widget->normal(), widget->center());
	}*/
}

void GLScene::setCutsectionPlane(int plane, bool active) {
	if (active) {
		GLC_Point3d center = world->collection()->boundingBox().center();
		GLC_Vector3d normal(0,1,0);
		//const double d1 = 1.00 * world->collection()->boundingBox().xLength();
		//const double d2 = 1.00 * world->collection()->boundingBox().yLength();
		switch (plane) {
			case 0: normal = GLC_Vector3d(0,1,0); break;
			case 1: normal = GLC_Vector3d(1,0,0); break;
			case 2: normal = GLC_Vector3d(0,0,-1); break;
		};

		//GLC_CuttingPlane* widget = new GLC_CuttingPlane(center, normal, d1, d2);
		//connect(widget, SIGNAL(asChanged()), this, SLOT(cutsectionUpdated()));
		//cutsectionPlaneWidget[plane] = widget->id();

		//widget->setOpacity(0.0);
		//widget->setProperty("plane",plane);
	
		//widget_manager->add3DWidget(widget);
		cutsectionPlaneWidget[plane] = 1;
		cutsectionPlane[plane] = new GLC_Plane(normal, center);
		viewport->addClipPlane(GL_CLIP_PLANE0 + plane, cutsectionPlane[plane]);
	} else if (cutsectionPlaneWidget[plane] != 0) {
		//widget_manager->remove3DWidget(cutsectionPlaneWidget[plane]);
		cutsectionPlaneWidget[plane] = 0;
		viewport->removeClipPlane(GL_CLIP_PLANE0 + plane);
	}
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

	//if (!fragment.compileSourceFile("../resources/shader/" + name + ".frag")) {
		//return 0;
	//}
	//if (!vertex.compileSourceFile("../resources/shader/" + name + ".vert")) {
		//return 0;
	//}
	if (!fragment.compileSourceFile(":/shader/" + name + ".frag")) {
		return 0;
	}
	if (!vertex.compileSourceFile(":/shader/" + name + ".vert")) {
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
	shader_background = compileShader("background");
	shader_outline = compileShader("outline");
	shader_shadow = compileShader("shadow");
	shader_fxaa = compileShader("fxaa");
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::drawScreenQuad() {
	//Setup correct projection-view matrix (all views)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, 1.0f, 500.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glTranslatef(0.0, 0.0, -1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	//Draw screen quad
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

	//Restore state
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::recursiveSelect(Object* object) {
	world->collection()->select(object->getRenderer()->getInstance()->id());
	for (int i = 0; i < object->getChildrenCount(); i++) {
		recursiveSelect(object->getChild(i));
	}
}

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
		previousRect = QRectF(0,0,0,0);
		doCenter();

		//Load shaders
		loadShaders();
	}

	//Setup native rendering and viewport size
	painter->beginNativePainting();
	viewport->setWinGLSize(rect.width(), rect.height());
	geometryChanged(rect);
	if (rect != previousRect) {
		previousRect = rect;

		if (fbo_outline) delete fbo_outline;
		if (fbo_outline_selected) delete fbo_outline_selected;
		if (fbo_shadow) delete fbo_shadow;
		if (fbo_fxaa) delete fbo_fxaa;
		fbo_outline = new QGLFramebufferObject((int)rect.width(),(int)rect.height(),QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		fbo_outline_selected = new QGLFramebufferObject((int)rect.width(),(int)rect.height(),QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		fbo_shadow = new QGLFramebufferObject((int)rect.width(),(int)rect.height(),QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		if (fw_editor_settings->value("rendering.use_fxaa") == true) {
			fbo_fxaa = new QGLFramebufferObject((int)rect.width(),(int)rect.height(),QGLFramebufferObject::Depth,GL_TEXTURE_2D,GL_RGBA8);
		} else {
			fbo_fxaa = 0;
		}
	}

	//Always use orthographic view
	viewport->setToOrtho(sceneOrthographic);
	bool inSelectionMode = GLC_State::isInSelectionMode();

	//Process selection from the editor
	world->collection()->unselectAll();
	if (editor->getSelected()) {
		recursiveSelect(editor->getSelected());
	}


	//==========================================================================
	//Clear screen and buffers
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (fbo_outline) {
		fbo_outline->bind();
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fbo_outline->release();
	}
	if (fbo_outline_selected) {
		fbo_outline_selected->bind();
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fbo_outline_selected->release();
	}
	if (fbo_shadow) {
		fbo_shadow->bind();
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fbo_shadow->release();
	}
	if (fbo_fxaa) {
		fbo_fxaa->bind();
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fbo_fxaa->release();
	}


	//==========================================================================
	//Draw background
	if (!inSelectionMode) {
		if (fbo_fxaa) fbo_fxaa->bind();
			if (shader_background) {
				shader_background->bind();
				shader_background->setUniformValue("v_baseColor",190.0f,190.0f,230.0f);
				drawScreenQuad();
				shader_background->release();
			}
		if (fbo_fxaa) fbo_fxaa->release();
	}


	//==========================================================================
	//Prepare scene rendering
    GLC_Context::current()->glcLoadIdentity();
	viewport->setDistMinAndMax(world->collection()->boundingBox()); //Clipping planes defined by bounding box
	viewport->glExecuteCam(); //Camera
	viewport->useClipPlane(true); //Enable section plane
	light[0]->setPosition(viewport->cameraHandle()->eye() - viewport->cameraHandle()->forward() * 1000.0); //Parallel lighting
	light[0]->glExecute(); //Scene light #1

	//Draw into outline buffer
	if ((!inSelectionMode) && fbo_outline) {
		fbo_outline->bind();
			world->render(0, glc::OutlineSilhouetteRenderFlag);
			world->render(1, glc::OutlineSilhouetteRenderFlag);
		fbo_outline->release();
	}
	if ((!inSelectionMode) && fbo_outline_selected) {
		fbo_outline_selected->bind();
			world->render(1, glc::OutlineSilhouetteRenderFlag);
		fbo_outline_selected->release();
	}

	//Draw into shadows buffer
	if ((!inSelectionMode) && fbo_shadow && shader_shadow && sceneShadowed) {
		fbo_shadow->bind();
			GLC_Context::current()->glcPushMatrix();
			GLC_Context::current()->glcTranslated(0,0,1.2*world->collection()->boundingBox().lowerCorner().z());
			GLC_Context::current()->glcScaled(1,1,0);
				//viewport->setWinGLSize(rect.width()/2, rect.height()/2);
				world->collection()->setLodUsage(false,viewport);

				world->render(0, glc::ShadingFlag);
				world->render(1, glc::ShadingFlag);

				world->collection()->setLodUsage(true,viewport);
				//viewport->setWinGLSize(rect.width(), rect.height());
			GLC_Context::current()->glcPopMatrix();
		fbo_shadow->release();

		//Make shadows even more blurry
		for (int i = 0; i < 8; i++) {
			fbo_shadow->bind();
				viewport->useClipPlane(false);
				glBindTexture(GL_TEXTURE_2D, fbo_shadow->texture());
				shader_shadow->bind();
				shader_shadow->setUniformValue("s_Data",0);
				shader_shadow->setUniformValue("v_invScreenSize",1.0f/rect.width(),1.0f/rect.height());
				drawScreenQuad();
				shader_shadow->release();
				viewport->useClipPlane(true);
			fbo_shadow->release();
		}

		if (fbo_fxaa) fbo_fxaa->bind();
			viewport->useClipPlane(false);
			glBindTexture(GL_TEXTURE_2D, fbo_shadow->texture());
			shader_shadow->bind();
			shader_shadow->setUniformValue("s_Data",0);
			shader_shadow->setUniformValue("v_invScreenSize",1.0f/rect.width(),1.0f/rect.height());
			drawScreenQuad();
			shader_shadow->release();
			viewport->useClipPlane(true);
		if (fbo_fxaa) fbo_fxaa->release();
	}

	//Render scene into world
	if ((!inSelectionMode) && fbo_fxaa) fbo_fxaa->bind();
		if (!sceneWireframe) {
			world->render(0, glc::ShadingFlag);
			//glClear(GL_DEPTH_BUFFER_BIT);
			world->render(1, glc::ShadingFlag);
		}
		if (!makingScreenshot) {
			viewport->useClipPlane(false);
			widget_manager->render();
			viewport->useClipPlane(true);
		}
	if ((!inSelectionMode) && fbo_fxaa) fbo_fxaa->release();


	//==========================================================================
	//Draw the rest of UI related stuff/outlines without clipping planes
	viewport->useClipPlane(false);

	//Draw object outlines
	if ((!inSelectionMode) && fbo_outline && shader_outline) {
		if (fbo_fxaa) fbo_fxaa->bind();
			shader_outline->bind();
			shader_outline->setUniformValue("s_Data",0);
			shader_outline->setUniformValue("v_invScreenSize",1.0f/rect.width(),1.0f/rect.height());
			shader_outline->setUniformValue("f_outlineThickness",
				(GLfloat)fw_editor_settings->value("rendering.outline_thickness").toDouble()
			);
				glBindTexture(GL_TEXTURE_2D, fbo_outline->texture());
				drawScreenQuad();
				glBindTexture(GL_TEXTURE_2D, fbo_outline_selected->texture());
				drawScreenQuad();
			shader_outline->release();
		if (fbo_fxaa) fbo_fxaa->release();
	}

	//Draw controller UI
	if (!inSelectionMode) {
		if (fbo_fxaa) fbo_fxaa->bind();
			//Draw CM indicator
			glClear(GL_DEPTH_BUFFER_BIT);
			if (editor->getSelected()) {
				bool cm1 = editor->getSelected()->isInformationDefined("total_cm");
				bool cm2 = editor->getSelected()->isInformationDefined("cm");
				if (cm1 || cm2) {
					QVector3D position = QVector3D();
					if (cm1) {
						position = editor->getSelected()->getInformationVector("total_cm");
					} else {
						position = editor->getSelected()->getInformationVector("cm");
					}

					indicator_cm->resetMatrix();
					indicator_cm->translate(position.x(),position.y(),position.z());
					indicator_cm->multMatrix(editor->getSelected()->getRenderer()->getInstance()->matrix());
					indicator_cm->render();
				}
			}

			controller.drawActiveMoverRep();
		if (fbo_fxaa) fbo_fxaa->release();
	}
	

	//==========================================================================
	//End FXAA and display it on screen
	if ((!inSelectionMode) && fbo_fxaa) {
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
	QMouseEvent mouseEvent(QEvent::MouseButtonPress,
				QPoint(e->scenePos().x(),e->scenePos().y()),
				e->button(),e->buttons(),e->modifiers());

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
			if (widget_manager->mousePressEvent(&mouseEvent) == glc::BlockedEvent) {
				update();
				break;
			}

			controller.setActiveMover(GLC_MoverController::Pan, GLC_UserInput(x,y));
			//{ GLC_uint selectedID = viewport->renderAndSelect(x,y);
			//qDebug("Id: %d\n",selectedID);}
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
/*void GLScene::selectByCoordinates(int x, int y, bool multi, QMouseEvent* pMouseEvent) {
	const bool spacePartitioningIsUsed= m_World.collection()->spacePartitioningIsUsed();
	// Selection frustum
	if (spacePartitioningIsUsed) {
		GLC_Frustum selectionFrustum(m_GlView.selectionFrustum(x, y));
		m_World.collection()->updateInstanceViewableState(selectionFrustum);
		m_World.collection()->setSpacePartitionningUsage(false);
	}

	m_SelectionMode= true;
	setAutoBufferSwap(false);

	m_World.collection()->setLodUsage(true, &m_GlView);
	GLC_uint SelectionID= m_GlView.renderAndSelect(x, y);
	m_World.collection()->setLodUsage(false, &m_GlView);

	if (spacePartitioningIsUsed) {
		m_World.collection()->updateInstanceViewableState(m_GlView.frustum());
		m_World.collection()->setSpacePartitionningUsage(true);
	}

	// 3DWidget manager test
	glc::WidgetEventFlag eventFlag = m_3DWidgetManager.mousePressEvent(pMouseEvent);

	m_SelectionMode= false;
	setAutoBufferSwap(true);

	if (eventFlag == glc::BlockedEvent) {
		updateGL();
		return;
	} else if (m_World.containsOccurence(SelectionID)) {
		if ((!m_World.isSelected(SelectionID)) && (m_World.selectionSize() > 0) && (!multiSelection)) {
			m_World.unselectAll();
			emit unselectAll();
		}
		if (!m_World.isSelected(SelectionID)) {
			m_World.select(SelectionID);
			updateGL();
			emit updateSelection(m_World.collection()->selection());
		} else if (m_World.isSelected(SelectionID) && multiSelection) {
			m_World.unselect(SelectionID);
			updateGL();
			emit updateSelection(m_World.collection()->selection());
		} else {
			m_World.unselectAll();
			m_World.select(SelectionID);
			updateGL();
			emit updateSelection(m_World.collection()->selection());
		}
	} else if ((m_World.selectionSize() > 0) && (!multiSelection)) {
		// if a geometry is selected, unselect it
		m_World.unselectAll();
		updateGL();
		emit unselectAll();
	} else if (eventFlag == glc::AcceptEvent) {
		updateGL();
	}
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
	QMouseEvent mouseEvent(QEvent::MouseButtonPress,
				QPoint(e->scenePos().x(),e->scenePos().y()),
				e->button(),e->buttons(),e->modifiers());

	QGraphicsScene::mouseMoveEvent(e);
	if (e->isAccepted()) return;

	//Process 3D widgets
	if (widget_manager->mouseMoveEvent(&mouseEvent) == glc::BlockedEvent) {
		update();
		return;
	}

	//Keep moving the view
	int x = e->scenePos().x();
	int y = e->scenePos().y();
	if (controller.hasActiveMover()) {
		controller.move(GLC_UserInput(x,y));
		//viewport->setDistMinAndMax(world->collection()->boundingBox());
		update();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void GLScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
	QMouseEvent mouseEvent(QEvent::MouseButtonPress,
				QPoint(e->scenePos().x(),e->scenePos().y()),
				e->button(),e->buttons(),e->modifiers());

	QGraphicsScene::mouseReleaseEvent(e);
	if (e->isAccepted()) return;

	//Process 3D widgets
	if (widget_manager->mouseReleaseEvent(&mouseEvent) == glc::BlockedEvent) {
		update();
		return;
	}

	//Stop moving the view
	if (controller.hasActiveMover()) {
		controller.setNoMover();
		update();
	}
}