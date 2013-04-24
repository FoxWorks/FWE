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
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_renderer.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::ObjectRenderer(Object* in_object) {
	object = in_object;
	mesh = NULL;
	objectModified = false;
	initializeGLFunctions();

	thread = new HQRendererThread(object,0.08f);
	connect(thread, SIGNAL(signalHQMeshReady()),this, SLOT(slotHQMeshReady()), Qt::QueuedConnection);
	thread->start();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::~ObjectRenderer() {
	if (mesh) EVDS_Mesh_Destroy(mesh);
	thread->stopWork();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
HQRendererThread::HQRendererThread(Object* in_object, float in_resolution) {
	object = in_object; 
	resolution = in_resolution;

	//Initialize temporary object
	temp_object = 0;
	mesh = 0;

	//Delete thread when work is finished
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));	
	doStopWork = false;
	needMesh = false; 
	meshCompleted = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
EVDS_MESH* ObjectRenderer::getMesh() {	
	if ((!mesh) || (objectModified)) {
		if (mesh) EVDS_Mesh_Destroy(mesh); //EVDS_MESH_SKIP_EDGES | EVDS_MESH_SKIP_VERTICES

		EVDS_OBJECT* temp_object;
		EVDS_Object_CopySingle(object->getEVDSObject(),0,&temp_object);
		EVDS_Object_Initialize(temp_object,1);
		EVDS_Mesh_Generate(temp_object,&mesh,0.2f,0);
		EVDS_Object_Destroy(temp_object);

		objectModified = false;
		thread->updateMesh();
	}

	if (isHQMeshReady()) {
		return thread->getMesh();
	} else {
		return mesh;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::meshChanged() {
	objectModified = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ObjectRenderer::isHQMeshReady() {
	return thread->getMesh() != 0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::slotHQMeshReady() {
	thread->readingLock.lock();
	emit signalHQMeshReady();
	thread->readingLock.unlock();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
EVDS_MESH* HQRendererThread::getMesh() { 
	if ((!needMesh) && mesh && meshCompleted && this->isRunning()) { 
		return mesh;
	} else {
		return 0;
	} 
};


////////////////////////////////////////////////////////////////////////////////
/// @brief Get a temporary copy of the rendered object
////////////////////////////////////////////////////////////////////////////////
void HQRendererThread::updateMesh() {
	if (this->isRunning()) {
		readingLock.lock();
		EVDS_Object_CopySingle(object->getEVDSObject(),0,&temp_object);
		readingLock.unlock();
	}
	needMesh = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void HQRendererThread::run() {
	while (!doStopWork) {
		if (needMesh) {
			readingLock.lock();
			//printf("Generating mesh %p\n",object);

			needMesh = false;
			meshCompleted = false;
			EVDS_OBJECT* work_object = temp_object; //Fetch the pointer
			EVDS_Object_TransferInitialization(work_object); //Get rights to work with variables
			
			if (mesh) EVDS_Mesh_Destroy(mesh);
			if (work_object) {
				EVDS_Object_Initialize(work_object,1);
				EVDS_Mesh_Generate(work_object,&mesh,resolution,0);//EVDS_MESH_SKIP_EDGES | EVDS_MESH_SKIP_VERTICES);
			}
			EVDS_Object_Initialize(work_object,1); //Must be initialized before being destroyed
			EVDS_Object_Destroy(work_object); //Release the object that was worked on

			//printf("Done mesh %p %p\n",object,mesh);
			meshCompleted = true;
			readingLock.unlock();

			emit signalHQMeshReady();
		}
		msleep(100);
	}

	//Finish thread work and destroy HQ mesh
	if (mesh) EVDS_Mesh_Destroy(mesh);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
//#include <rdrs.h>
//#include <math.h>
void ObjectRenderer::drawMesh(bool objectSelected) {
	//Make sure mesh exists
	EVDS_MESH* render_mesh = getMesh();
	if (!render_mesh) return;

	//Get selected cross-section index
	int selected_index = object->getSelectedCrossSection();

	//Determine if object is a fuel tank
	float special_color = 7.0;
	if (object->getType() == "fuel_tank") {
		if (object->isOxidizerTank()) {
			special_color = 4.0;
		} else {
			special_color = 3.0;
		}				
	}

	//Draw mesh
	float* color_attribs = (float*)malloc(sizeof(float)*render_mesh->num_vertices*4);
	for (int i = 0; i < render_mesh->num_vertices; i++) {
		color_attribs[i*4+0] = (float)objectSelected;
		color_attribs[i*4+1] = render_mesh->vertex_info[i].cross_section + ((int)(render_mesh) & 0xFF);
		color_attribs[i*4+2] = (float)(render_mesh->vertex_info[i].cross_section == selected_index);
		color_attribs[i*4+3] = special_color;
	}
	float* color_attribs2 = (float*)malloc(sizeof(float)*render_mesh->num_vertices*4);
	for (int i = 0; i < render_mesh->num_vertices; i++) {
		color_attribs2[i*4+0] = (float)objectSelected;
		color_attribs2[i*4+1] = color_attribs[i*4+1]+32;
		color_attribs2[i*4+2] = (float)(render_mesh->vertex_info[i].cross_section == selected_index);
		color_attribs2[i*4+3] = special_color;
	}

	glVertexPointer(3, GL_FLOAT, 0, render_mesh->vertices);
	glNormalPointer(GL_FLOAT, 0, render_mesh->normals);
	glColorPointer(4, GL_FLOAT, 0, color_attribs);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glCullFace(GL_BACK);
	glDrawElements(GL_TRIANGLES, render_mesh->num_indices, GL_UNSIGNED_INT, render_mesh->indices);

	glCullFace(GL_FRONT);
	glPolygonOffset(-2,-2);
	glColorPointer(4, GL_FLOAT, 0, color_attribs2);
	glDrawElements(GL_TRIANGLES, render_mesh->num_indices, GL_UNSIGNED_INT, render_mesh->indices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	free(color_attribs);
	free(color_attribs2);

	glPolygonOffset(0,0);
	glCullFace(GL_BACK);

	//Draw radio antenna directionality
	/*if (object->getType() == "antenna") {
		if (object->getEditor()->getSelected() == object) {
			Object* iobject = object->getInformationObject();

			glPointSize(2.0f);
			glDisable(GL_DEPTH_TEST);
			glBegin(GL_POINTS);
			if (iobject) {
				EVDS_OBJECT* antenna_object = iobject->getEVDSObject();
				RDRS_ANTENNA* antenna;
				EVDS_Object_GetSolverdata(antenna_object,(void**)&antenna);

				for (double theta = 0; theta < 2*EVDS_PI; theta += EVDS_PI/128.0) {
					for (double phi = 0; phi < EVDS_PI; phi += EVDS_PI/128.0) {
						double D = 0.0;
						double x,y,z;

						RDRS_Physics_GetDirectionalGain(antenna,theta,phi,&D);

						D *= 2;
						x = D*cos(theta);
						y = D*sin(theta)*cos(phi);
						z = D*sin(theta)*sin(phi);

						glColor4f(0.0f,63.0f,0.0f,1.0f);
						glVertex3f(x,y,z);
					}
				}
			}
			glEnd();
			glEnable(GL_DEPTH_TEST);
		}
	}*/

	//Draw center of mass
	/*if (object->getEditor()->getSelected() == object) {
		Object* iobject = object->getInformationObject();

		glDisable(GL_DEPTH_TEST);
		glBegin(GL_LINES);
		if (iobject) {
			QVector3D cm = iobject->getVector("cm");
			glColor4f(0.0f,63.0f,0.0f,1.0f);
			glVertex3f(cm.x()-2,cm.y()  ,cm.z()  );
			glVertex3f(cm.x()+2,cm.y()  ,cm.z()  );
			glVertex3f(cm.x()  ,cm.y()-2,cm.z()  );
			glVertex3f(cm.x()  ,cm.y()+2,cm.z()  );
			glVertex3f(cm.x()  ,cm.y()  ,cm.z()-2);
			glVertex3f(cm.x()  ,cm.y()  ,cm.z()+2);
		}
		glEnd();
		glEnable(GL_DEPTH_TEST);

	}*/

	/*glBegin(GL_LINES);
	for (int i = 0; i < render_mesh->num_triangles; i++) {
		EVDS_MESH_TRIANGLE* triangle = &render_mesh->triangles[i];
		if (triangle->thickness == 0.0) continue;
		for (int v = 0; v < 3; v++) {
			if (triangle->edge[v]) {
				glColor3f(1.0f,0.0f,0.0f);
			} else {
				glColor3f(0.0f,0.0f,1.0f);
			}
			glVertex3f(triangle->vertex[v].x,triangle->vertex[v].y,triangle->vertex[v].z);
			glVertex3f(triangle->vertex[(v+1)%3].x,triangle->vertex[(v+1)%3].y,triangle->vertex[(v+1)%3].z);
		}
	}
	glEnd();*/
}