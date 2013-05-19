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
#include "fwe_evds_glscene.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::ObjectRenderer(Object* in_object) {
	object = in_object;

	//Create meshes
	glcMesh = new GLC_Mesh();
	glcInstance = new GLC_3DViewInstance(glcMesh);

	//Read LOD count and make sure it's sane
	int lod_count = fw_editor_settings->value("rendering.lod_count",6).toInt();
	if (lod_count < 1) lod_count = 1;
	if (lod_count > 20) lod_count = 20;

	//Create mesh generators
	lodMeshGenerator = new ObjectLODGenerator(object,lod_count);
	connect(lodMeshGenerator, SIGNAL(signalLODsReady()), this, SLOT(lodMeshesGenerated()), Qt::QueuedConnection);
	if (fw_editor_settings->value("rendering.no_lods",false) == false) {
		lodMeshGenerator->start();
	}

	//Create initial data
	meshChanged();
	positionChanged();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::~ObjectRenderer() {
	delete glcInstance;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::positionChanged() {
	//Offset GLC instance relative to objects parent
	Object* parent = object->getParent();
	if (parent) {
		if (parent->getRenderer()) {
			glcInstance->setMatrix(parent->getRenderer()->getInstance()->matrix());
		} else {
			glcInstance->resetMatrix();
		}

		EVDS_OBJECT* obj = object->getEVDSObject();
		EVDS_STATE_VECTOR vector;
		EVDS_Object_GetStateVector(obj,&vector);
		glcInstance->translate(vector.position.x,vector.position.y,vector.position.z);

		//Add/replace in GL widget to update position
		GLScene* glview = object->getEVDSEditor()->getGLScene();
		if (glview->getCollection()->contains(glcInstance->id())) {
			glview->getCollection()->remove(glcInstance->id());			
		}
		glview->getCollection()->add(*(glcInstance));
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
	EVDS_MESH* mesh;

	//Create temporary object
	EVDS_OBJECT* temp_object;
	EVDS_Object_CopySingle(object->getEVDSObject(),0,&temp_object);
	EVDS_Object_Initialize(temp_object,1);

	//Ask dear generator LOD thing to generate LODs
	lodMeshGenerator->updateMesh();

	//Do the quick hack job anyway
	glcMesh->clear();
	//for (int i = 0; i < 4; i++) {
		//EVDS_Mesh_Generate(temp_object,&mesh,getLODResolution(3-i),EVDS_MESH_USE_DIVISIONS);
		//addLODMesh(mesh,i);
		//EVDS_Mesh_Destroy(mesh);
	//}

	EVDS_Mesh_Generate(temp_object,&mesh,32.0f,EVDS_MESH_USE_DIVISIONS);
	addLODMesh(mesh,0);
	EVDS_Mesh_Destroy(mesh);
	//EVDS_Mesh_Generate(temp_object,&mesh,24.0f,EVDS_MESH_USE_DIVISIONS);
	//addLODMesh(mesh,1);
	//EVDS_Mesh_Destroy(mesh);

	//glcMesh->reverseNormals();
	glcMesh->finish();
	EVDS_Object_Destroy(temp_object);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::lodMeshesGenerated() {
	qDebug("ObjectRenderer: LOD ready %p",this);
	
	glcMesh->clear();
	for (int i = 0; i < lodMeshGenerator->getNumLODs(); i++) {
		addLODMesh(lodMeshGenerator->getMesh(i),i);
	}
	//glcMesh->reverseNormals();
	glcMesh->finish();
	object->getEVDSEditor()->updateObject(NULL); //Force into repaint
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::addLODMesh(EVDS_MESH* mesh, int lod) {
	if (!mesh) return;

	//Add empty mesh?
	if (mesh->num_triangles == 0) {
		GLfloatVector verticesVector;
		GLfloatVector normalsVector;
		IndexList indicesList;

		verticesVector << 0 << 0 << 0;
		normalsVector << 0 << 0 << 0;
		indicesList << 0 << 0 << 0;

		glcMesh->addVertice(verticesVector);
		glcMesh->addNormals(normalsVector);
		glcMesh->addTriangles(new GLC_Material(), indicesList, lod);
	} else {
		GLfloatVector verticesVector;
		GLfloatVector normalsVector;
		QList<GLC_Material*> glcGroupMaterials;
		QList<IndexList> indicesLists;

		//Must have at least one smoothing group
		if (mesh->num_smoothing_groups == 0) mesh->num_smoothing_groups = 1;

		//Prepare indices and materials lists for every group
		for (int i = 0; i < mesh->num_smoothing_groups; i++) {
			GLC_Material* glcMaterial = new GLC_Material();
			glcGroupMaterials.append(glcMaterial);
			indicesLists.append(IndexList());
		}

		if (object->getType() == "fuel_tank") {
			if (object->isOxidizerTank()) {
				for (int i = 0; i < mesh->num_smoothing_groups; i++) {
					glcGroupMaterials[i]->setDiffuseColor(QColor(0,0,255));
				}
			} else {
				for (int i = 0; i < mesh->num_smoothing_groups; i++) {
					glcGroupMaterials[i]->setDiffuseColor(QColor(255,255,0));
				}
			}
		}

		//Add all data
		int firstVertexIndex = glcMesh->VertexCount();	
		for (int i = 0; i < mesh->num_vertices; i++) {
			verticesVector << mesh->vertices[i].x;
			verticesVector << mesh->vertices[i].y;
			verticesVector << mesh->vertices[i].z;
			normalsVector << mesh->normals[i].x;
			normalsVector << mesh->normals[i].y;
			normalsVector << mesh->normals[i].z;
		}
		for (int i = 0; i < mesh->num_triangles; i++) {
			indicesLists[mesh->triangles[i].smoothing_group] << mesh->triangles[i].indices[0] + firstVertexIndex;
			indicesLists[mesh->triangles[i].smoothing_group] << mesh->triangles[i].indices[1] + firstVertexIndex;
			indicesLists[mesh->triangles[i].smoothing_group] << mesh->triangles[i].indices[2] + firstVertexIndex;
		}
		indicesLists[0] << 0 << 0 << 0; //Prevent empty lists

		glcMesh->addVertice(verticesVector);
		glcMesh->addNormals(normalsVector);
		for (int i = 0; i < mesh->num_smoothing_groups; i++) {
			if (!indicesLists[i].isEmpty()) {
				glcMesh->addTriangles(glcGroupMaterials[i], indicesLists[i], lod); //coarse_mesh->resolution);
			}
		}

		/*for (int i = 0; i < mesh->num_triangles; i++) {
			GLfloatVector floatVector;
			floatVector << mesh->triangles[i].center.x;
			floatVector << mesh->triangles[i].center.y;
			floatVector << mesh->triangles[i].center.z;
			floatVector << mesh->triangles[i].center.x + mesh->triangles[i].triangle_normal.x*0.2;
			floatVector << mesh->triangles[i].center.y + mesh->triangles[i].triangle_normal.y*0.2;
			floatVector << mesh->triangles[i].center.z + mesh->triangles[i].triangle_normal.z*0.2;
			glcMesh->addVerticeGroup(floatVector);
		}*/
	}
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
float ObjectLODGenerator::getLODResolution(int lod) {
	float quality = fw_editor_settings->value("rendering.lod_quality",32.0f).toFloat();
	return quality * (1 + lod);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectLODGenerator::ObjectLODGenerator(Object* in_object, int in_lods) {
	object = in_object;
	numLods = in_lods;

	//Initialize temporary object
	object_copy = 0;
	mesh = (EVDS_MESH**)malloc(numLods*sizeof(EVDS_MESH*));
	for (int i = 0; i < numLods; i++) mesh[i] = 0;

	//Delete thread when work is finished
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));	
	doStopWork = false;
	needMesh = false; 
	meshCompleted = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
EVDS_MESH* ObjectLODGenerator::getMesh(int lod) { 
	if ((!needMesh) && mesh && meshCompleted && this->isRunning()) { 
		return mesh[lod];
	} else {
		return 0;
	} 
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Get a temporary copy of the rendered object
////////////////////////////////////////////////////////////////////////////////
void ObjectLODGenerator::updateMesh() {
	needMesh = true;
	if (this->isRunning()) {
		readingLock.lock();
		EVDS_Object_CopySingle(object->getEVDSObject(),0,&object_copy);
		readingLock.unlock();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectLODGenerator::run() {
	msleep(2000); //Give enough time for the rest of application to initialize
	while (!doStopWork) {
		if (needMesh) {
			readingLock.lock();

			//Start making the mesh
			needMesh = false;
			meshCompleted = false;

			//Transfer and initialize work object
			EVDS_OBJECT* work_object = object_copy; //Fetch the pointer
			EVDS_Object_TransferInitialization(work_object); //Get rights to work with variables
			EVDS_Object_Initialize(work_object,1);
			object_copy = 0;

			for (int lod = 0; lod < numLods; lod++) {
				//printf("Generating mesh %p for level %d\n",object,lod);
			
				//Remove old mesh
				if (mesh[lod]) {
					EVDS_Mesh_Destroy(mesh[lod]);
					mesh[lod] = 0;
				}

				//Check if job must be aborted
				if (needMesh) {
					qDebug("ObjectLODGenerator: aborted job early");
					break;
				}

				//Create new one
				EVDS_Mesh_Generate(work_object,&mesh[lod],getLODResolution(numLods-lod-1),EVDS_MESH_USE_DIVISIONS);
				//printf("Done mesh %p %p for level %d\n",object,mesh,lod);
			}

			//Release the object that was worked on
			if (work_object) {
				EVDS_Object_Destroy(work_object);
			}
			readingLock.unlock();

			//Finish working
			meshCompleted = true;

			//If new mesh is needed, do not return generated one - return actually needed one instead
			if (!needMesh) {
				emit signalLODsReady();
			}
		}
		msleep(50);
	}

	//Finish thread work and destroy HQ mesh
	//if (mesh) EVDS_Mesh_Destroy(mesh);
}