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
#include <QFontMetrics>
#include <QFileInfo>

#include <GLC_UserInput>
#include <GLC_Exception>
#include <GLC_Context>
#include <GLC_CuttingPlane>

#include <math.h>
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_glscene.h"
#include "fwe_schematics.h"
#include "fwe_schematics_renderer.h"

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
GLScene::GLScene(GLScene* in_parent_scene, Editor* in_editor, SchematicsEditor* in_schematics_editor, QWidget *parent) : QGraphicsScene(parent) {
	parent_scene = in_parent_scene; //FIXME: support for this
	editor = in_editor;
	schematics_editor = in_schematics_editor;

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
	//world->collection()->setVboUsage(true); FIXME
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
	if (schematics_editor) viewport->cameraHandle()->setTopView();

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

	if (!schematics_editor) { //Only in EVDS editor
		button_projection = new QPushButton(QIcon(":/icon/glview/projection_ortho.png"),"");
		connect(button_projection, SIGNAL(pressed()), this, SLOT(toggleProjection()));
		panel_control->layout()->addWidget(button_projection);

		button_shadow = new QPushButton(QIcon(":/icon/glview/render_shadow.png"),"");
		connect(button_shadow, SIGNAL(pressed()), this, SLOT(toggleShadow()));
		panel_control->layout()->addWidget(button_shadow);

		button_material_mode = new QPushButton(QIcon(":/icon/glview/render_shaded.png"),"");
		connect(button_material_mode, SIGNAL(pressed()), this, SLOT(toggleMaterialMode()));
		panel_control->layout()->addWidget(button_material_mode);
	}

	button_save_picture = new QPushButton(QIcon(":/icon/glview/render_screenshot.png"),"");
	connect(button_save_picture, SIGNAL(pressed()), this, SLOT(saveScreenshot()));
	panel_control->layout()->addWidget(button_save_picture);

	if (schematics_editor) {
		button_save_sheets = new QPushButton();//QIcon(":/icon/glview/render_screenshot.png"),"");
		connect(button_save_sheets, SIGNAL(pressed()), this, SLOT(saveSheets()));
		panel_control->layout()->addWidget(button_save_sheets);
	}
	

	//Fill view panel with buttons
	if (!schematics_editor) { //Only in schematics editor
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
	}
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

	editor->getWindow()->getMainWindow()->statusBar()->showMessage("Saved screenshot ("+fileName+")",3000);
}
void GLScene::saveSheets() {
	QString baseFilename = editor->getWindow()->getCurrentFile();
	QFileInfo baseInfo = QFileInfo(baseFilename);

	Object* old_sheet = schematics_editor->getCurrentSheet();
	int sheet_no = 1;
	for (int i = 0; i < schematics_editor->getRoot()->getChildrenCount(); i++) {
		Object* sheet = schematics_editor->getRoot()->getChild(i);
		if (sheet->getType() == "foxworks.schematics.sheet") {
			schematics_editor->setCurrentSheet(sheet);
			schematics_editor->getSchematicsRenderingManager()->updateInstances();

			QString code = sheet->getString("sheet.code");
			if (code == "") code = editor->getEditDocument()->getString("document.code");
			if (code == "") code = baseInfo.baseName();
			if (sheet->getVariable("sheet.number") > 0.0) sheet_no = (int)sheet->getVariable("sheet.number");

			saveCurrentSheet(tr("%1 (sheet %2).jpg")
				.arg(code)
				.arg(sheet_no));

			editor->getWindow()->getMainWindow()->statusBar()->showMessage(tr("Exported sheet %1..").arg(sheet_no),2000);
			sheet_no++;
		}
	}
	schematics_editor->setCurrentSheet(old_sheet);
	schematics_editor->getSchematicsRenderingManager()->updateInstances();
	editor->getWindow()->getMainWindow()->statusBar()->showMessage("Finished exporting sheets!",3000);
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
void GLScene::saveCurrentSheet(const QString& baseFilename) {
	if (!schematics_editor->getCurrentSheet()) return;

	QGLFramebufferObjectFormat format;
	format.setSamples(16);

	//Get number of pixels per cm
	float ppcm = schematics_editor->getCurrentSheet()->getVariable("paper.ppcm");
	if (ppcm <= 0.0) ppcm = 32.0;

	//Get paper size
	float paper_width,paper_height;
	schematics_editor->getCurrentSheet()->getSheetPaperSizeInCM(&paper_width,&paper_height);

	//Get picture size
	int width = paper_width*ppcm;
	int height = paper_height*ppcm;

	//Create final image buffer
	QImage finalImage(width,height,QImage::Format_RGB32);
	int fbo_width = 1024;
	int fbo_height = 1024;
	int x = 0;
	int y = 0;

	//Save camera
	GLC_Camera old_camera = GLC_Camera(*viewport->cameraHandle());
	
	while (x < width) {
		y = 0;
		while (y < height) {
			//Create FBO and draw into it
			QGLFramebufferObject renderFbo(fbo_width,fbo_height);
			QPainter fboPainter(&renderFbo);
			fboPainter.setRenderHint(QPainter::Antialiasing);
			fboPainter.setRenderHint(QPainter::HighQualityAntialiasing);

				//Offsets in meters
				float xstep = (fbo_width/ppcm)*0.01f;
				float ystep = (fbo_height/ppcm)*0.01f;

				//Calculate proper camera offset
				GLC_Vector3d targetVector(
					xstep*0.5f + xstep*(x/((float)fbo_width)),
					ystep*0.5f + ystep*((height-y)/((float)fbo_height) - 1),
					0);

				//if (width < fbo_width) targetVector.setX(0.0);
				//if (height < fbo_height) targetVector.setY(0.0);

				//Frame image correctly
				viewport->cameraHandle()->translate(targetVector-viewport->cameraHandle()->target());
				viewport->cameraHandle()->setDistEyeTarget(ystep*1.430f);

				//Render
				QRectF oldRect = sceneRect();
				setSceneRect(QRectF(0,0,fbo_width,fbo_height));
				panel_control->hide();
				panel_view->hide();
					makingScreenshot = true;
					render(&fboPainter);
					//fboPainter.drawText(0,50,tr("POSITION %1x%2").arg(x/2048).arg(0));
					makingScreenshot = false;
				setSceneRect(oldRect);
				panel_control->show();
				panel_view->show();

			fboPainter.end();

			QPainter painter(&finalImage);
			painter.drawImage(x, y, renderFbo.toImage());
			painter.end();

			y += fbo_height;
		}

		x += fbo_width;
	}

	//Restore camera
	viewport->cameraHandle()->setCam(old_camera);

	//Save image
	finalImage.save(baseFilename,0,95);
	//renderFbo.toImage().save(baseFilename,0,95);
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
/// @brief Draw schematics page layout
////////////////////////////////////////////////////////////////////////////////
QPointF GLScene::project(float x, float y, float z) {
	//Get OpenGL matrices
	double projectionMatrix[16];
	double viewMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
	glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);

	//Create result
	QPointF result;
	QVector3D screenPos = QVector3D(x,y,0)*QMatrix4x4(viewMatrix)*QMatrix4x4(projectionMatrix);
	result.setX(0.5*(screenPos.x()+1.0) * previousRect.width());
	result.setY(0.5*(1.0-screenPos.y()) * previousRect.height());
	return result;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Draw schematics page layout
////////////////////////////////////////////////////////////////////////////////
void GLScene::drawSchematicsElement(QPainter *painter, Object* element, QPointF offset) {
	for (int i = 0; i < element->getChildrenCount(); i++) {
		Object* child = element->getChild(i);
		if (child->getType() == "foxworks.schematics.element") {
			//Get state vector
			EVDS_STATE_VECTOR vector;
			EVDS_Object_GetStateVector(child->getEVDSObject(),&vector);

			//Draw if it's a label
			if (child->getString("reference") == "") {
				float font_size = 0.005f; //FIXME
				QString value = element->getName();
				if (child->getString("text") != "") value = child->getString("text");
				
				//Fake multi-line text
				QStringList lines = value.split("\n");
				for (int i = 0; i < lines.count(); i++) {
					painter->drawText(
						project(vector.position.x + offset.x(),vector.position.y + offset.y() - (font_size+0.001f)*i),
						lines[i]);
				}
			}

			//Draw children with offset
			drawSchematicsElement(painter,child,QPointF(vector.position.x,vector.position.y));
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Draw schematics page layout
////////////////////////////////////////////////////////////////////////////////
void GLScene::drawSchematicsPage(QPainter *painter) {
	Object* sheet = schematics_editor->getCurrentSheet();
	if (!sheet) return;

	//Define paper size
	float width,height;
	sheet->getSheetPaperSizeInCM(&width,&height);
	width *= 0.01f;
	height *= 0.01f;

	//Define other variables
	float margin = 0.006f; //6 mm
	float normal_font = 0.005f; //5 mm
	float normal_font_w = normal_font*0.75;
	float small_font = 0.0035f; //3.5 mm
	float thick_line = 0.0007f; //1.5 mm

	//Define line thickness
	int thick_line_px = project(0,0).y()-project(0,thick_line).y();
	if (thick_line_px < 1) thick_line_px = 1;

	//Define font thickness
	int normal_font_px = project(0,0).y()-project(0,normal_font).y();
	if (normal_font_px < 1) normal_font_px = 1;
	int small_font_px = project(0,0).y()-project(0,small_font).y();
	if (small_font_px < 1) small_font_px = 1;
	painter->setFont(QFont("GOST type B",normal_font_px));


	//Draw paper borders
	painter->setPen(QPen(QBrush(QColor(0,0,0)),thick_line_px));
	painter->drawRect(QRectF(project(0,0),project(width,height)));
	painter->drawRect(QRectF(project(margin,margin),project(width-margin,height-margin)));


	//Draw sections
	int sectionMulW = 1;
	int sectionMulH = 1;

	double wm = sheet->getVariable("paper.width_multiplier");
	double hm = sheet->getVariable("paper.height_multiplier");
	if (wm > 1.0) sectionMulW = (int)wm;
	if (hm > 1.0) sectionMulH = (int)hm;

	float sectionWidth = width / (sectionMulW*10.0f);
	float sectionHeight = height / (sectionMulH*10.0f);
	for (int x = 1; x < sectionMulW*10; x++) {
		painter->drawLine(project(sectionWidth*x,0.00f),project(sectionWidth*x,margin));
		painter->drawLine(project(sectionWidth*x,height-margin),project(sectionWidth*x,height));
		painter->drawText(project(sectionWidth*(x-0.5f),0.5*(margin-normal_font)),tr("%1").arg(x));
		painter->drawText(project(sectionWidth*(x-0.5f),height-margin+0.5*(margin-normal_font)),tr("%1").arg(x));
	}
	for (int y = 1; y < sectionMulH*10; y++) {
		painter->drawLine(project(0.00f,sectionHeight*y),project(margin,sectionHeight*y));
		painter->drawLine(project(width-margin,sectionHeight*y),project(width,sectionHeight*y));
		painter->drawText(
			project(
			0.5*(margin-normal_font_w),
			sectionHeight*(y-0.5f)-normal_font*0.5),tr("%1").arg((char)((y-1)+'A')));
		painter->drawText(
			project(
			width-margin+0.5*(margin-normal_font_w),
			sectionHeight*(y-0.5f)-normal_font*0.5),tr("%1").arg((char)((y-1)+'A')));
	}


	//Draw page information (GOST 2.104)
	if (1) { //First page
#define local(x,y) project(width-0.185-margin+x,margin+0.040-y)

		//General frame
		painter->drawRect(QRectF(local(0.000,0.000),local(0.185,0.040)));

		//Draw all other lines
		painter->drawLine(local(0.065,0.000),local(0.065,0.040));
		painter->drawLine(local(0.065,0.015),local(0.185,0.015));
		painter->drawLine(local(0.135,0.015),local(0.135,0.040));

		painter->drawLine(local(0.135,0.020),local(0.185,0.020));
		painter->drawLine(local(0.135,0.025),local(0.185,0.025));
		painter->drawLine(local(0.150,0.015),local(0.150,0.025));
		painter->drawLine(local(0.165,0.015),local(0.165,0.025));

		painter->drawLine(local(0.017,0.000),local(0.017,0.040));
		for (int line = 1; line <= 7; line++) {
			painter->drawLine(local(0.000,0.005*line),local(0.065,0.005*line));
		}

		painter->drawLine(local(0.017,0.000),local(0.017,0.040));

		//Draw texts
		painter->setFont(QFont("GOST type B",small_font_px));
			painter->drawText(local(0.001,0.014),"Created");
			painter->drawText(local(0.001,0.019),"Drawn");
			painter->drawText(local(0.001,0.024),"Verified");

			painter->drawText(local(0.137,0.019),"Sheet");
			painter->drawText(local(0.152,0.019),"Count");
			painter->drawText(local(0.170,0.019),"Scale");

			//Fill out fields
			painter->drawText(local(0.018,0.014),editor->getEditDocument()->getString("document.created_by"));
			painter->drawText(local(0.018,0.019),editor->getEditDocument()->getString("document.drawn_by"));
			painter->drawText(local(0.018,0.024),editor->getEditDocument()->getString("document.verified_by"));

			double scale = sheet->getVariable("sheet.scale");
			if (scale <= 0.0) scale = 1.0;
			painter->drawText(local(0.170,0.024),"1:" + tr("%1").arg(scale));
		painter->setFont(QFont("GOST type B",normal_font_px));

		//Fill out big fields
		QFontMetrics metric = painter->fontMetrics();
		QString value;


		if (sheet->getString("sheet.code") != "") {
			value = sheet->getString("sheet.code");
		} else {
			value = editor->getEditDocument()->getString("document.code");
		}
		painter->drawText(local(0.065 + 0.5*(0.185-0.065),0.000 + 0.5*(0.000-0.015) + 0.001)
			-QPointF(metric.width(value)/2,-metric.height()/2),value);


		if (sheet->getString("sheet.title") != "") {
			value = sheet->getString("sheet.title");
		} else {
			value = editor->getEditDocument()->getString("document.title");
		}
		painter->drawText(local(0.065 + 0.5*(0.135-0.065),0.015 + 0.5*(0.015-0.040) + 0.001)
			-QPointF(metric.width(value)/2,-metric.height()/2),value);


		value = editor->getEditDocument()->getString("document.company");
		painter->drawText(local(0.135 + 0.5*(0.185-0.135),0.025 + 0.5*(0.025-0.040) + 0.001)
			-QPointF(metric.width(value)/2,-metric.height()/2),value);


		//painter->drawText(QRectF(local(0.065,0.000),local(0.185,0.015)),
			//Qt::AlignCenter,editor->getEditDocument()->getString("document.code"));
		//painter->drawText(QRectF(local(0.065,0.015),local(0.135,0.015)),
			//Qt::AlignCenter,editor->getEditDocument()->getString("document.title"));

#undef local
	} else {

	}


	//Draw all labels
	drawSchematicsElement(painter,sheet,QPointF(0,0));

	//drawGOSTText(painter,0.05f,0.05f,normal_font,"1234567890");
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

void GLScene::drawBackground(QPainter *painter, const QRectF& rect) {
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
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	//Use orthographic view
	if (schematics_editor) {
		viewport->setToOrtho(true);
	} else {
		viewport->setToOrtho(sceneOrthographic);
	}
	bool inSelectionMode = GLC_State::isInSelectionMode();

	//Process selection from the editor
	world->collection()->unselectAll();
	if (editor->getSelected()) {
		recursiveSelect(editor->getSelected());
	}

	//Setup culling and LOD usage
	if (makingScreenshot) {
		viewport->setMinimumPixelCullingSize(0);
		world->collection()->setLodUsage(false,viewport);
	} else {
		viewport->setMinimumPixelCullingSize(fw_editor_settings->value("rendering.min_pixel_culling").toInt());
		world->collection()->setLodUsage(true,viewport);
	}


	//==========================================================================
	//Clear screen and buffers
	glClearColor(1.0f,1.0f,1.0f,0.0f);
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
			glClearColor(1.0f,1.0f,1.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fbo_fxaa->release();
	}


	//==========================================================================
	//Draw background
	if ((!inSelectionMode) && (!schematics_editor)) {
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
	if ((!inSelectionMode) && fbo_shadow && shader_shadow && sceneShadowed && (!schematics_editor)) {
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
		if (!sceneWireframe && (!schematics_editor)) {
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
			if (schematics_editor) {
				float thickness = fabs(project(0.0,0.0).y() - project(0.0,0.0007f).y())*0.5f;
				if (thickness < 0.5) thickness = 0.5;
				shader_outline->setUniformValue("f_outlineThickness",thickness);
			} else {
				shader_outline->setUniformValue("f_outlineThickness",
					(GLfloat)fw_editor_settings->value("rendering.outline_thickness").toDouble()
				);
			}
				glBindTexture(GL_TEXTURE_2D, fbo_outline->texture());
				drawScreenQuad();
				glBindTexture(GL_TEXTURE_2D, fbo_outline_selected->texture());
				drawScreenQuad();
			shader_outline->release();
		if (fbo_fxaa) fbo_fxaa->release();
	}

	//Draw schematics
	/*if (fbo_fxaa) fbo_fxaa->bind();
		if (schematics_editor) { //Draw schematics page in world
			viewport->useClipPlane(false);
				QPainter fbo_painter(fbo_fxaa);
				if (makingScreenshot) {
					drawSchematicsPage(painter);
				} else {
					drawSchematicsPage(&fbo_painter);
				}
			viewport->useClipPlane(true);
		}
	if (fbo_fxaa) fbo_fxaa->release();*/

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

	//Draw 2D schematics page
	//if (fbo_fxaa) fbo_fxaa->bind();
		if (schematics_editor) {
			viewport->useClipPlane(false);
				//QPainter fbo_painter(fbo_fxaa);
				//if (makingScreenshot) {
					drawSchematicsPage(painter);
				//} else {
					//drawSchematicsPage(&fbo_painter);
				//}
			viewport->useClipPlane(true);
		}
	//if (fbo_fxaa) fbo_fxaa->release();

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
			if (!schematics_editor) {
				controller.setActiveMover(GLC_MoverController::TrackBall, GLC_UserInput(x,y));
				update();
			}
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