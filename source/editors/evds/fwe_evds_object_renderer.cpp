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
#include "fwe_glscene.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::ObjectRenderer(Object* in_object) {
	object = in_object;

	//Create meshes
	glcMesh = new GLC_Mesh();
	glcMeshRep = new GLC_3DRep(glcMesh);
	glcInstance = new GLC_3DViewInstance(*glcMeshRep);

	//Read LOD count and make sure it's sane
	int lod_count = fw_editor_settings->value("rendering.lod_count").toInt();
	if (lod_count < 1) lod_count = 1;
	if (lod_count > 20) lod_count = 20;

	//Create mesh generators
	lodMeshGenerator = new ObjectLODGenerator(object,lod_count);
	connect(lodMeshGenerator, SIGNAL(signalLODsReady()), this, SLOT(lodMeshesGenerated()), Qt::QueuedConnection);
	if (fw_editor_settings->value("rendering.no_lods") == false) {
		lodMeshGenerator->start();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::~ObjectRenderer() {
	//Remove instances from modifiers
	//removeFromModifiers(object);

	//Remove instances from glview
	GLScene* glview = object->getEVDSEditor()->getGLScene();
	if (glview->getCollection()->contains(glcInstance->id())) {
		glview->getCollection()->remove(glcInstance->id());
	}

	delete glcInstance;
	lodMeshGenerator->stopWork();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::positionChanged() {
	GLScene* glview = object->getEVDSEditor()->getGLScene();

	//Offset GLC instance relative to objects parent
	Object* parent = object->getParent();
	if (parent) {
		//Get state vector
		EVDS_OBJECT* obj = object->getEVDSObject();
		EVDS_STATE_VECTOR vector;
		EVDS_Object_GetStateVector(obj,&vector);

		//Get rotation matrix
		EVDS_MATRIX rotationMatrix;
		EVDS_Quaternion_ToMatrix(&vector.orientation,rotationMatrix);

		//Apply transformations
		glcInstance->resetMatrix();
		glcInstance->multMatrix(GLC_Matrix4x4(rotationMatrix));
		glcInstance->translate(vector.position.x,vector.position.y,vector.position.z);

		//Update visibility of this object
		if (object->getVariable("disable") > 0.5) {
			glcInstance->setVisibility(false);
		} else {
			glcInstance->setVisibility(true);
		}

		//Add parents transformation to place this object relative to its parent
		if (parent->getRenderer()) {
			//The multiplication is right instead of left in multMatrix. So the order is "all wrong"
			glcInstance->multMatrix(parent->getRenderer()->getInstance()->matrix());

			//Take visibility from parent
			if (parent->getRenderer()->getInstance()->isVisible() == false) {
				glcInstance->setVisibility(false);
			}
		}
		
		//Add/replace in GL widget to update position
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
	if (object->getType() != "modifier") {
		EVDS_MESH* mesh;

		//Create temporary object
		EVDS_OBJECT* temp_object;
		EVDS_OBJECT* inertial_root;
		EVDS_SYSTEM* system;
		EVDS_Object_GetSystem(object->getEVDSObject(),&system);
		EVDS_System_GetRootInertialSpace(system,&inertial_root);
		EVDS_Object_CopySingle(object->getEVDSObject(),inertial_root,&temp_object);
		EVDS_Object_Initialize(temp_object,1);

		//Ask dear generator LOD thing to generate LODs
		lodMeshGenerator->updateMesh();

		//Do the quick hack job anyway
		glcMesh->clear();
			ObjectLODGeneratorResult result;
				EVDS_MESH_GENERATEEX info = { 0 };
				info.resolution = 32.0f;
				info.min_resolution = fw_editor_settings->value("rendering.min_resolution").toFloat();
				info.flags = EVDS_MESH_USE_DIVISIONS;

				EVDS_Mesh_GenerateEx(temp_object,&mesh,&info);
				result.appendMesh(mesh,0);
				EVDS_Mesh_Destroy(mesh);
			result.setGLCMesh(glcMesh,object);
		glcMesh->finish();
		glcMesh->clearBoundingBox(); //Clear bounding box to update it
		EVDS_Object_Destroy(temp_object);
	} else { //Cannot have an empty mesh..
		glcMesh->clear();
			GLfloatVector verticesVector;
			GLfloatVector normalsVector;
			IndexList indicesList;

			verticesVector << 0 << 0 << 0;
			normalsVector << 0 << 0 << 0;
			indicesList << 0 << 0 << 0;

			glcMesh->addVertice(verticesVector);
			glcMesh->addNormals(normalsVector);
			glcMesh->addTriangles(new GLC_Material(), indicesList, 0);
		glcMesh->finish();
		glcMesh->clearBoundingBox(); //Clear bounding box to update it
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::lodMeshesGenerated() {
	//qDebug("ObjectRenderer: LOD ready %p",this);
	
	lodMeshGenerator->readingLock.lock();
		glcMesh->clear();
		lodMeshGenerator->getResult()->setGLCMesh(glcMesh,object);
		glcMesh->finish();

		glcInstance->setMatrix(glcInstance->matrix()); //This causes bounding box to be updated
		object->getEVDSEditor()->updateObject(NULL); //Force into repaint
	lodMeshGenerator->readingLock.unlock();

	object->getEVDSEditor()->getWindow()->getMainWindow()->statusBar()->showMessage("Generating LODs...",1000);
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectLODGeneratorResult::appendMesh(EVDS_MESH* mesh, int lod) {
	if (!mesh) return;

	//Add empty mesh?
	if (mesh->num_triangles == 0) {
		verticesVector << 0 << 0 << 0;
		normalsVector << 0 << 0 << 0;
		indicesLists.append(IndexList() << 0 << 0 << 0);
		lodList.append(lod);
	} else {
		//Must have at least one smoothing group
		if (mesh->num_smoothing_groups == 0) mesh->num_smoothing_groups = 1;

		//Get first indices to append
		int firstVertexIndex = (verticesVector.count())/3;
		int firstSmoothingGroupIndex = indicesLists.count();

		//Prepare indices and materials lists for every group
		for (int i = 0; i < mesh->num_smoothing_groups; i++) {
			indicesLists.append(IndexList());
			lodList.append(lod);
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
		for (int i = 0; i < mesh->num_triangles; i++) {
			//if ((lod > 0) && (i > 50)) return;
			indicesLists[firstSmoothingGroupIndex + mesh->triangles[i].smoothing_group] << 
				mesh->triangles[i].indices[0] + firstVertexIndex;
			indicesLists[firstSmoothingGroupIndex + mesh->triangles[i].smoothing_group] << 
				mesh->triangles[i].indices[1] + firstVertexIndex;
			indicesLists[firstSmoothingGroupIndex + mesh->triangles[i].smoothing_group] << 
				mesh->triangles[i].indices[2] + firstVertexIndex;
		}
		
		//FIXME prevent empty lists
	}
}

//#include "C:\\Program Files\\Intel\\VTune Amplifier XE 2011\\include\\ittnotify.h"
//#pragma comment(lib, "C:\\Program Files\\Intel\\VTune Amplifier XE 2011\\lib32\\libittnotify.lib")
//__itt_frame pD = __itt_frame_create("Custom Domain");
//__itt_frame_begin(pD);
//__itt_frame_end(pD);

void ObjectLODGeneratorResult::setGLCMesh(GLC_Mesh* glcMesh, Object* object) {
	//QApplication::setOverrideCursor(Qt::WaitCursor);
	glcMesh->addVertice(verticesVector);
	glcMesh->addNormals(normalsVector);
	for (int i = 0; i < indicesLists.count(); i++) {
		if (!indicesLists[i].isEmpty()) {
			GLC_Material* glcMaterial = new GLC_Material();
			
			//Special color logic
			if (object->getType() == "fuel_tank") {
				if (object->isOxidizerTank()) {
					glcMaterial->setDiffuseColor(QColor(0,0,255));
				} else {
					glcMaterial->setDiffuseColor(QColor(255,255,0));
				}
			}

			//Add smoothing group
			glcMesh->addTriangles(glcMaterial, indicesLists[i], lodList[i]);
		}
	}
	//QApplication::restoreOverrideCursor();
}

void ObjectLODGeneratorResult::clear() {
	verticesVector.clear();
	normalsVector.clear();
	indicesLists.clear();
	lodList.clear();
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
float ObjectLODGenerator::getLODResolution(int lod) {
	float quality = fw_editor_settings->value("rendering.lod_quality").toFloat();
	return quality * (1 + lod);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectLODGenerator::ObjectLODGenerator(Object* in_object, int in_lods) {
	object = in_object;
	numLods = in_lods;
	editor = object->getEVDSEditor();

	//Initialize temporary object
	object_copy = 0;

	//Delete thread when work is finished
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
	connect(&updateCallTimer, SIGNAL(timeout()), this, SLOT(doUpdateMesh()));
	doStopWork = false;
	needMesh = false; 
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectLODGeneratorResult* ObjectLODGenerator::getResult() { 
	return &result;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Get a temporary copy of the rendered object
////////////////////////////////////////////////////////////////////////////////
void ObjectLODGenerator::doUpdateMesh() {
	updateCallTimer.stop();
	readingLock.lock();
		needMesh = true;
		if (this->isRunning()) {
			EVDS_OBJECT* inertial_root;
			EVDS_SYSTEM* system;
			EVDS_Object_GetSystem(object->getEVDSObject(),&system);
			EVDS_System_GetRootInertialSpace(system,&inertial_root);
			EVDS_Object_CopySingle(object->getEVDSObject(),inertial_root,&object_copy);
		}
	readingLock.unlock();
}

void ObjectLODGenerator::updateMesh() {
	//qDebug("ObjectLODGenerator::updateMesh: start timer");
	updateCallTimer.start(500);
}

void ObjectLODGenerator::stopWork() {
	doStopWork = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectLODGenerator::run() {
	editor->addActiveThread();
	//msleep(1000 + (qrand() % 5000)); //Give enough time for the rest of application to initialize
	while (!doStopWork) {
		readingLock.lock();
		if (needMesh) {
			//Start making the mesh
			needMesh = false;
			ObjectLODGenerator::threadsSemaphore.acquire();

			//Transfer and initialize work object
			EVDS_OBJECT* work_object = object_copy; //Fetch the pointer
			EVDS_Object_TransferInitialization(work_object); //Get rights to work with variables
			EVDS_Object_Initialize(work_object,1);
			object_copy = 0;

			result.clear();
			for (int lod = 0; lod < numLods; lod++) {
				//Check if job must be aborted
				if (needMesh || doStopWork) {
					qDebug("ObjectLODGenerator: aborted job early");
					break;
				}

				//Create new one
				EVDS_MESH* mesh;
				EVDS_MESH_GENERATEEX info = { 0 };
				info.resolution = getLODResolution(numLods-lod-1);
				info.min_resolution = fw_editor_settings->value("rendering.min_resolution").toFloat();
				info.flags = EVDS_MESH_USE_DIVISIONS;

				EVDS_Mesh_GenerateEx(work_object,&mesh,&info);
				result.appendMesh(mesh,lod);
				EVDS_Mesh_Destroy(mesh);
				//printf("Done mesh %p %p for level %d\n",object,mesh,lod);
			}

			//Make sure not too many threads run expensive tasks at once
			ObjectLODGenerator::threadsSemaphore.release();

			//Release the object that was worked on
			if (work_object) {
				EVDS_Object_Destroy(work_object);
			}

			//If new mesh is needed, do not return generated one - return actually needed one instead
			if (!needMesh) {
				emit signalLODsReady();
			}
		}
		readingLock.unlock();
		msleep(50);
	}

	//Finish thread work and destroy HQ mesh
	//qDebug("ObjectLODGenerator::run: stopped");
	editor->removeActiveThread();
}

QSemaphore ObjectLODGenerator::threadsSemaphore(QThread::idealThreadCount());