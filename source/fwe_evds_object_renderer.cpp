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
#include <evds.h>
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_glwidget.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::ObjectRenderer(Object* in_object) {
	object = in_object;
	visibleLod = 0;

	for (int i = 0; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
		//Create meshes for LOD levels
		glcMesh[i] = new GLC_Mesh();
		glcInstance[i] = new GLC_3DViewInstance(glcMesh[i]);
		glcInstance[i]->setVisibility(false);

		//Create mesh generators
		lodMeshGenerators[i] = new ObjectMeshGenerator(object,getLODResolution(i),i);
		connect(lodMeshGenerators[i], SIGNAL(signalMeshReady(int)),
			this, SLOT(lodMeshGenerated(int)), Qt::QueuedConnection);
		lodMeshGenerators[i]->start();
	}

	//Create initial meshes
	meshChanged();
	positionChanged();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::~ObjectRenderer() {
	for (int i = 0; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
		delete glcInstance[i];
		lodMeshGenerators[i]->stopWork();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
float ObjectRenderer::getLODResolution(int lod) {
	return 32.0f*((float)(1+5*lod));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::lodMeshGenerated(int lod) {
	printf("LOD arrived %d\n",lod);
	
	lodMeshGenerators[lod]->readingLock.lock();
	addLODMesh(lodMeshGenerators[lod]->getMesh(),lod);
	lodMeshGenerators[lod]->readingLock.unlock();

	//positionChanged();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::positionChanged() {
	//Offset GLC instance relative to objects parent
	Object* parent = object->getParent();
	if (parent) {
		for (int i = 0; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) { //FIXME
			if (parent->getRenderer()) {
				glcInstance[i]->setMatrix(parent->getRenderer()->getInstance(0)->matrix());
			} else {
				glcInstance[i]->resetMatrix();
			}

			EVDS_OBJECT* obj = object->getEVDSObject();
			EVDS_STATE_VECTOR vector;
			EVDS_Object_GetStateVector(obj,&vector);
			glcInstance[i]->translate(vector.position.x,vector.position.y,vector.position.z);

			//Add/replace in GL widget to update position
			GLWidget* glview = object->getEVDSEditor()->getGLWidget();
			if (glview->getCollection()->contains(glcInstance[i]->id())) {
				glview->getCollection()->remove(glcInstance[i]->id());			
			}
			glview->getCollection()->add(*(glcInstance[i]));
			//object->getEVDSEditor()->getGLWidget()->getCollection()->
		}
	}

	//Update position of all children
	for (int i = 0; i < object->getChildrenCount(); i++) {
		object->getChild(i)->getRenderer()->positionChanged();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::meshChanged() {
	EVDS_MESH* coarse_mesh;

	//Create temporary object and generate coarse mesh from it
	EVDS_OBJECT* temp_object;
	EVDS_Object_CopySingle(object->getEVDSObject(),0,&temp_object);
	EVDS_Object_Initialize(temp_object,1);
	EVDS_Mesh_Generate(temp_object,&coarse_mesh,16.0f,EVDS_MESH_USE_DIVISIONS);
	EVDS_Object_Destroy(temp_object);

	visibleLod = -1; //Coarse mesh always visible
	for (int i = 0; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
		lodPresent[i] = 0;
		lodFinished[i] = 0;
		glcInstance[i]->setVisibility(false);
	}
	addLODMesh(coarse_mesh,0);


	//Create temporary object and generate coarse mesh from it
	/*EVDS_Object_CopySingle(object->getEVDSObject(),0,&temp_object);
	EVDS_Object_Initialize(temp_object,1);
	EVDS_Mesh_Generate(temp_object,&coarse_mesh,128.0f,EVDS_MESH_USE_DIVISIONS);
	EVDS_Object_Destroy(temp_object);
	addLODMesh(coarse_mesh,1);*/

	//Ask for higher quality mesh from all other generators
	for (int i = 1; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
		lodMeshGenerators[i]->updateMesh();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::addLODMesh(EVDS_MESH* mesh, int lod) {
	for (int i = 0; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
		if (lod == 0) { //Special case: coarse mesh is always first, clears all others
			glcMesh[i]->clear();
		}
	}
	if (!mesh) return;

	//Add empty mesh?
	if (mesh->num_triangles == 0) {
		GLfloatVector verticesVector;
		GLfloatVector normalsVector;
		IndexList indicesList;

		if (lod == 0) { //Add only coarse mesh
			verticesVector << 0 << 0 << 0;
			normalsVector << 0 << 0 << 0;
			indicesList << 0 << 0 << 0;

			for (int i = lod; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
				glcMesh[i]->addVertice(verticesVector);
				glcMesh[i]->addNormals(normalsVector);
				glcMesh[i]->addTriangles(new GLC_Material(), indicesList, 0);
			}
		}
	} else {
		GLfloatVector verticesVector;
		GLfloatVector normalsVector;
		GLC_Material* glcMaterial = new GLC_Material();

		if (object->getType() == "fuel_tank") {
			if (object->isOxidizerTank()) {
				glcMaterial->setDiffuseColor(QColor(0,0,255));
			} else {
				glcMaterial->setDiffuseColor(QColor(255,255,0));
			}
		}

		//Add all data
		for (int i = 0; i < mesh->num_vertices; i++) {
			verticesVector << mesh->vertices[i].x;
			verticesVector << mesh->vertices[i].y;
			verticesVector << mesh->vertices[i].z;
			normalsVector << mesh->normals[i].x;
			normalsVector << mesh->normals[i].y;
			normalsVector << mesh->normals[i].z;
		}

		for (int i = lod; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
			int firstVertexIndex = glcMesh[i]->VertexCount();

			IndexList indicesList; //Generate custom indices per each level
			for (int j = 0; j < mesh->num_triangles; j++) {
				if ((mesh->triangles[j].vertex[0].y > 0.0) &&
					(mesh->triangles[j].vertex[1].y > 0.0) &&
					(mesh->triangles[j].vertex[2].y > 0.0)) {
					indicesList << mesh->triangles[j].indices[0] + firstVertexIndex;
					indicesList << mesh->triangles[j].indices[1] + firstVertexIndex;
					indicesList << mesh->triangles[j].indices[2] + firstVertexIndex;
				}
			}

			glcMesh[i]->addVertice(verticesVector);
			glcMesh[i]->addNormals(normalsVector);
			glcMesh[i]->addTriangles(glcMaterial, indicesList, lod); //coarse_mesh->resolution);
		}
		//glcMesh->reverseNormals();
	}

	//Make the current LOD
	lodPresent[lod] = 1;

	//Finish meshes which have complete data
	for (int i = 0; i < EVDS_OBJECT_RENDERER_MAX_LODS; i++) {
		int all_present = 1;
		for (int j = 0; j <= i; j++) {
			if (!lodPresent[j]) all_present = 0;
		}

		if (all_present && (!lodFinished[i])) {
			printf("Finished mesh LOD %d\n",i);
			glcMesh[i]->finish();
			lodFinished[i] = true;
			if (i > visibleLod) {
				//FIXME: update visibility
				visibleLod = i;

				for (int j = 0; j < EVDS_OBJECT_RENDERER_MAX_LODS; j++) {
					if (j != visibleLod) {
						glcInstance[i]->setVisibility(false);
					} else {
						glcInstance[i]->setVisibility(true);

						//Add/replace in GL widget to update position
						GLWidget* glview = object->getEVDSEditor()->getGLWidget();
						if (glview->getCollection()->contains(glcInstance[i]->id())) {
							glview->getCollection()->remove(glcInstance[i]->id());			
						}
						//delete glcInstance[i] = new GLC_3DViewInstance(glcMesh[i]);
						//glcInstance[i]->setVisibility(false);
						//glview->getCollection()->add(*(glcInstance[i]));
					}
				}
			}
		}
	}

	//glcMesh->finish();
	//glcMesh->finish();
	//EVDS_Mesh_Destroy(mesh);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
/*EVDS_MESH* ObjectRenderer::getMesh() {	
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
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectMeshGenerator::ObjectMeshGenerator(Object* in_object, float in_resolution, int in_lod) {
	object = in_object; 
	resolution = in_resolution;
	lod = in_lod;

	//Initialize temporary object
	object_copy = 0;
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
EVDS_MESH* ObjectMeshGenerator::getMesh() { 
	if ((!needMesh) && mesh && meshCompleted && this->isRunning()) { 
		return mesh;
	} else {
		return 0;
	} 
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Get a temporary copy of the rendered object
////////////////////////////////////////////////////////////////////////////////
void ObjectMeshGenerator::updateMesh() {
	if (this->isRunning()) {
		printf("  Copy %p for level %d\n",object,lod);
		readingLock.lock();
		EVDS_Object_CopySingle(object->getEVDSObject(),0,&object_copy);
		readingLock.unlock();
	}
	needMesh = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectMeshGenerator::run() {
	while (!doStopWork) {
		if (needMesh) {
			readingLock.lock();
			printf("Generating mesh %p for level %d\n",object,lod);

			needMesh = false;
			meshCompleted = false;
			EVDS_OBJECT* work_object = object_copy; //Fetch the pointer
			EVDS_Object_TransferInitialization(work_object); //Get rights to work with variables
			
			//Remove old mesh
			if (mesh) {
				EVDS_Mesh_Destroy(mesh);
				mesh = 0;
			}

			//Create new mesh from initialized object copy
			if (work_object) { //EVDS_MESH_SKIP_EDGES | EVDS_MESH_SKIP_VERTICES);
				EVDS_Object_Initialize(work_object,1);
				EVDS_Mesh_Generate(work_object,&mesh,resolution,EVDS_MESH_USE_DIVISIONS);
				EVDS_Object_Destroy(work_object); //Release the object that was worked on
			}

			printf("Done mesh %p %p for level %d\n",object,mesh,lod);
			meshCompleted = true;
			readingLock.unlock();

			if (!needMesh) { //If new mesh is needed, do not return generated one - return actually needed one instead
				emit signalMeshReady(lod);
			}
		}
		msleep(100);
	}

	//Finish thread work and destroy HQ mesh
	if (mesh) EVDS_Mesh_Destroy(mesh);
}