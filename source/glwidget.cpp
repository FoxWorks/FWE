/****************************************************************************

 This file is part of the GLC-lib library.
 Copyright (C) 2005-2008 Laurent Ribon (laumaya@users.sourceforge.net)
 http://glc-lib.sourceforge.net

 GLC-lib is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 GLC-lib is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GLC-lib; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*****************************************************************************/

#include <QtDebug>

#include <GLC_UserInput>
#include <GLC_Exception>
#include <GLC_Context>

#include "glwidget.h"

// For VSYNC problem under Mac OS X
#if defined(Q_OS_MAC)
#include <OpenGL.h>
#endif

GLWidget::GLWidget(QWidget *p_parent)
: QGLWidget(new GLC_Context(QGLFormat(QGL::SampleBuffers)),p_parent)
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

	// Set The Camera default Up Vector
	m_GlView.cameraHandle()->setDefaultUpVector(glc::Z_AXIS);
	m_GlView.cameraHandle()->setIsoView();

	// Create objects to display
	CreateScene();

}

GLWidget::~GLWidget()
{
}

void GLWidget::initializeGL()
{
	// For VSYNC problem under Mac OS X
	#if defined(Q_OS_MAC)
	const GLint swapInterval = 1;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);
	#endif

	m_GlView.initGl();
	m_GlView.reframe(m_Collection.boundingBox());
}

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
//////////////////////////End GLC specific/////////////////////////////////////

}

void GLWidget::resizeGL(int width, int height)
{
//////////////////////////// GLC specific///////////////////////////////////////
	m_GlView.setWinGLSize(width, height);	// Compute window aspect ratio
//////////////////////////End GLC specific/////////////////////////////////////

}

void createMeshAndWire(GLC_Mesh& mesh)
{
	const GLfloat lgX= static_cast<const GLfloat>(5.0 / 2.0);
	const GLfloat lgY= static_cast<const GLfloat>(5.0 / 2.0);
	const GLfloat lgZ= static_cast<const GLfloat>(5.0 / 2.0);

	GLfloatVector verticeVector;
	GLfloatVector normalsVector;
	GLfloatVector texelVector;

	// Face 1
	verticeVector << -lgX; verticeVector << -lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << 1.0f;
	texelVector << 0.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << -lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << 1.0f;
	texelVector << 1.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << 1.0f;
	texelVector << 1.0f; texelVector << 1.0f;

	verticeVector << -lgX; verticeVector << lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << 1.0f;
	texelVector << 0.0f; texelVector << 1.0f;

	// Face 2
	verticeVector << lgX; verticeVector << -lgY; verticeVector << lgZ;
	normalsVector << 1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << -lgY; verticeVector << -lgZ;
	normalsVector << 1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << lgY; verticeVector << -lgZ;
	normalsVector << 1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 1.0f;

	verticeVector << lgX; verticeVector << lgY; verticeVector << lgZ;
	normalsVector << 1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 1.0f;

	// Face 3
	verticeVector << -lgX; verticeVector << -lgY; verticeVector << -lgZ;
	normalsVector << -1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 0.0f;

	verticeVector << -lgX; verticeVector << -lgY; verticeVector << lgZ;
	normalsVector << -1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 0.0f;

	verticeVector << -lgX; verticeVector << lgY; verticeVector << lgZ;
	normalsVector << -1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 1.0f;

	verticeVector << -lgX; verticeVector << lgY; verticeVector << -lgZ;
	normalsVector << -1.0f; normalsVector << 0.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 1.0f;

	// Face 4
	verticeVector << lgX; verticeVector << -lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << -1.0f;
	texelVector << 0.0f; texelVector << 0.0f;

	verticeVector << -lgX; verticeVector << -lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << -1.0f;
	texelVector << 1.0f; texelVector << 0.0f;

	verticeVector << -lgX; verticeVector << lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << -1.0f;
	texelVector << 1.0f; texelVector << 1.0f;

	verticeVector << lgX; verticeVector << lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << 0.0f; normalsVector << -1.0f;
	texelVector << 0.0f; texelVector << 1.0f;

	// Face 5
	verticeVector << -lgX; verticeVector << lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << 1.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << 1.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << 1.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 1.0f;

	verticeVector << -lgX; verticeVector << lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << 1.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 1.0f;

	// Face 6
	verticeVector << -lgX; verticeVector << -lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << -1.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << -lgY; verticeVector << -lgZ;
	normalsVector << 0.0f; normalsVector << -1.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 0.0f;

	verticeVector << lgX; verticeVector << -lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << -1.0f; normalsVector << 0.0f;
	texelVector << 1.0f; texelVector << 1.0f;

	verticeVector << -lgX; verticeVector << -lgY; verticeVector << lgZ;
	normalsVector << 0.0f; normalsVector << -1.0f; normalsVector << 0.0f;
	texelVector << 0.0f; texelVector << 1.0f;

	// Add bulk data in to the mesh
	mesh.addVertice(verticeVector);
	mesh.addNormals(normalsVector);
	mesh.addTexels(texelVector);

	// Set the material to use
	GLC_Material* pMaterial= new GLC_Material();

	IndexList index;
	// Face 1
	index << 0 << 1 << 3 << 2;
	mesh.addTrianglesStrip(pMaterial, index, 0, 1.0);
	index.clear();
	// Face 2
	index << 4 << 5 << 7 << 6;
	mesh.addTrianglesStrip(pMaterial, index, 0, 1.0);
	index.clear();
	// Face 3
	index << 8 << 9 << 11 << 10;
	mesh.addTrianglesStrip(pMaterial, index, 1, 1.0);
	index.clear();
	// Face 4
	index << 12 << 13 << 15 << 14;
	mesh.addTrianglesStrip(pMaterial, index, 1, 5.0);
	index.clear();
	// Face 5
	index << 16 << 17 << 19 << 18;
	mesh.addTrianglesStrip(pMaterial, index, 2, 5.0);
	index.clear();
	// Face 6
	index << 20 << 21 << 23 << 22;
	mesh.addTrianglesStrip(pMaterial, index, 2, 5.0);
	index.clear();

	mesh.finish();
}

// Create GLC_Object to display
void GLWidget::CreateScene()
{
	QColor matBlue;
	matBlue.setRgbF(0.5, 0.8, 1.0, 1.0);

	// Create a new cylinder
	//GLC_3DViewInstance instance(GLC_Factory::instance()->createCylinder(1.0, 2.0));
	// Assign material to the cylinder
    //instance.geomAt(0)->replaceMasterMaterial(new GLC_Material(matBlue));
	// Add the cylinder to the collection
	//m_Collection.add(instance);

	// Add mesh
	//GLC_3DViewInstance instance(GLC_Factory::instance()->createM(1.0, 2.0));
	GLC_Mesh* mesh = new GLC_Mesh();
	createMeshAndWire(*mesh);
	GLC_3DViewInstance instance(mesh);
	m_Collection.add(instance);

	// Create 20 circles
	for (int i= 0; i < 20; ++i)
	{
		//GLC_3DViewInstance instance(GLC_Factory::instance()->createCircle(1.2 + static_cast<double>(i) / 10.0));
		//instance.geomAt(0)->setWireColor(Qt::white);
		//m_Collection.add(instance);
	}
	m_Collection.setLodUsage(true,&m_GlView);
}

void GLWidget::mousePressEvent(QMouseEvent *e)
{
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

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
	if (m_MoverController.hasActiveMover())
	{
		m_MoverController.move(GLC_UserInput(e->x(), e->y()));
		m_GlView.setDistMinAndMax(m_Collection.boundingBox());
		updateGL();
	}
}

void GLWidget::mouseReleaseEvent(QMouseEvent *)
{
	if (m_MoverController.hasActiveMover())
	{
		m_MoverController.setNoMover();
		updateGL();
	}
}
