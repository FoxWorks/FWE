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
	glcMeshRep = new GLC_3DRep(glcMesh);
	glcInstance = new GLC_3DViewInstance(*glcMeshRep);

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
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::~ObjectRenderer() {
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
void ObjectRenderer::positionChanged(bool travel_up) {
	GLScene* glview = object->getEVDSEditor()->getGLScene();

	//Offset GLC instance relative to objects parent
	Object* parent = object->getParent();
	if (parent) { //FIXME: this is not needed when travel_up = true and object isn't a modifier
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

		if (parent->getRenderer()) {
			//The multiplication is right instead of left in multMatrix. So the order is "all wrong"
			glcInstance->multMatrix(parent->getRenderer()->getInstance()->matrix());
		}
		
		//Add/replace in GL widget to update position
		if (glview->getCollection()->contains(glcInstance->id())) {
			glview->getCollection()->remove(glcInstance->id());
		}
		glview->getCollection()->add(*(glcInstance));
	}

	//Update position of all children
	if (!travel_up) {
		for (int i = 0; i < object->getChildrenCount(); i++) {
			object->getChild(i)->getRenderer()->positionChanged();
		}
	}

	//Travel up and update the modifiers (FIXME: adds unneccessary lag!)
	if (parent && parent->getRenderer()) {
		parent->getRenderer()->positionChanged(true);
	}

	//Update position of all modifier instances
	for (int i = 0; i < modifierInstances.count(); i++) {
		//Move instance as requested by modifier
					//GLC_Matrix4x4 old_matrix = base_instance->matrix();
					//modifier_inst.instance->setMatrix(modifier_inst.base_instance->matrix());
					//modifier_inst.instance->resetMatrix();
					//modifier_inst.instance->translate(offset.x(),offset.y(),offset.z());
					//modifier_inst.instance->multMatrix(old_matrix);

		//Move instance as requested by modifier
		GLC_Matrix4x4 old_matrix = modifierInstances[i].base_instance->matrix();
		modifierInstances[i].instance->resetMatrix();
		modifierInstances[i].instance->multMatrix(old_matrix);
		modifierInstances[i].instance->multMatrix(glcInstance->matrix().inverted());
		modifierInstances[i].instance->multMatrix(modifierInstances[i].transformation);
		modifierInstances[i].instance->multMatrix(glcInstance->matrix());

		//Add/replace to update position
		if (glview->getCollection()->contains(modifierInstances[i].instance->id())) {
			glview->getCollection()->remove(modifierInstances[i].instance->id());
		}
		glview->getCollection()->add(*modifierInstances[i].instance);
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

	ObjectLODGeneratorResult result;
		EVDS_Mesh_Generate(temp_object,&mesh,32.0f,EVDS_MESH_USE_DIVISIONS);
		result.appendMesh(mesh,0);
		EVDS_Mesh_Destroy(mesh);
	result.setGLCMesh(glcMesh,object);

	glcMesh->finish();
	glcInstance->setMatrix(glcInstance->matrix()); //This causes bounding box to be updated
	EVDS_Object_Destroy(temp_object);

	//Add modifier instances
	if (object->getType() == "modifier") {
		//Remove copies of children instances
		GLScene* glview = object->getEVDSEditor()->getGLScene();
		for (int i = 0; i < modifierInstances.count(); i++) {
			if (glview->getCollection()->contains(modifierInstances[i].instance->id())) {
				glview->getCollection()->remove(modifierInstances[i].instance->id());
			}
			delete modifierInstances[i].instance;
		}
		modifierInstances.clear();

		//Add copies for every child
		addModifierInstances(object);

		//Update positions (for the new modifier instances)
		positionChanged();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::addModifierInstances(Object* child) {
	if (!child->getRenderer()) return; //Can happen when modifier is just being created

	//Get modifier information
	int axis1_count = object->getVariable("axis1.count");
	int axis2_count = object->getVariable("axis2.count");
	int axis3_count = object->getVariable("axis3.count");
	QVector3D axis1 = QVector3D(
		object->getVariable("axis1.x"),
		object->getVariable("axis1.y"),
		object->getVariable("axis1.z"));
	QVector3D axis2 = QVector3D(
		object->getVariable("axis2.x"),
		object->getVariable("axis2.y"),
		object->getVariable("axis2.z"));
	QVector3D axis3 = QVector3D(
		object->getVariable("axis3.x"),
		object->getVariable("axis3.y"),
		object->getVariable("axis3.z"));

	//Make sure master copy remains
	if (axis1_count < 1) axis1_count = 1;
	if (axis2_count < 1) axis2_count = 1;
	if (axis3_count < 1) axis3_count = 1;

	//Add instances as moved by modifier
	for (int i = 0; i < axis1_count; i++) {
		for (int j = 0; j < axis2_count; j++) {
			for (int k = 0; k < axis3_count; k++) {
				if ((i != 0) || (j != 0) || (k != 0)) {
					QVector3D offset = axis1*i + axis2*j + axis3*k;

					//Create copy of the child itself
					ObjectRendererModifierInstance modifier_inst;
					modifier_inst.base_instance = child->getRenderer()->getInstance();
					modifier_inst.representation = child->getRenderer()->getRepresentation();
					modifier_inst.instance = new GLC_3DViewInstance(*modifier_inst.representation);

					//Create transformation
					modifier_inst.transformation.setMatTranslate(offset.x(),offset.y(),offset.z());

					//Add instance to scene
					GLScene* glview = object->getEVDSEditor()->getGLScene();
					glview->getCollection()->add(*modifier_inst.instance);

					//Remember instance
					modifierInstances.append(modifier_inst);


					//Copy modifiers instances to this modifier
					if ((child->getType() == "modifier") && (child != object)) {
						ObjectRenderer* childRenderer = child->getRenderer();
						for (int j = 0; j < childRenderer->modifierInstances.count(); j++) {
							ObjectRendererModifierInstance modifier_inst;
							modifier_inst.base_instance = childRenderer->modifierInstances[j].base_instance;
							modifier_inst.representation = childRenderer->modifierInstances[j].representation;
							modifier_inst.instance = new GLC_3DViewInstance(*modifier_inst.representation);

							//Create transformation
							GLC_Matrix4x4 translation;
							translation.setMatTranslate(offset.x(),offset.y(),offset.z());
							modifier_inst.transformation = childRenderer->modifierInstances[j].transformation;
							modifier_inst.transformation = modifier_inst.transformation * translation;

							//Add instance to scene
							GLScene* glview = object->getEVDSEditor()->getGLScene();
							glview->getCollection()->add(*modifier_inst.instance);

							//Remember instance
							modifierInstances.append(modifier_inst);
						}
					}
				}
			}
		}
	}

	//Add same for every child
	for (int i = 0; i < child->getChildrenCount(); i++) {
		if (child->getChild(i)->getType() == "modifier") { //Recursively update modifiers
			child->getChild(i)->getRenderer()->meshChanged();
		}
		addModifierInstances(child->getChild(i));
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectRenderer::lodMeshesGenerated() {
	//qDebug("ObjectRenderer: LOD ready %p",this);
	QTime time; time.start();
	
	lodMeshGenerator->readingLock.lock();
		glcMesh->clear();
		lodMeshGenerator->getResult()->setGLCMesh(glcMesh,object);
		glcMesh->finish();

		glcInstance->setMatrix(glcInstance->matrix()); //This causes bounding box to be updated
		object->getEVDSEditor()->updateObject(NULL); //Force into repaint
	lodMeshGenerator->readingLock.unlock();

	object->getEVDSEditor()->getWindow()->getMainWindow()->statusBar()->showMessage("Generating LODs...",1000);

	int elapsed = time.elapsed();
	if (elapsed > 50) {
		qDebug("ObjectRenderer: setGLCMesh(%s): %d msec (%d verts)",
			object->getName().toAscii().data(),
			elapsed, glcMesh->VertexCount());
	}
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
void ObjectLODGeneratorResult::setGLCMesh(GLC_Mesh* glcMesh, Object* object) {
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
		//QApplication::processEvents();
	}
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
			EVDS_Object_CopySingle(object->getEVDSObject(),0,&object_copy);
		}
	readingLock.unlock();
}

void ObjectLODGenerator::updateMesh() {
	//qDebug("ObjectLODGenerator::updateMesh: start timer");
	updateCallTimer.start(500);
}

void ObjectLODGenerator::stopWork() {
	doStopWork = true;
	while (isRunning()) {
		msleep(10);
		//QApplication::processEvents();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectLODGenerator::run() {
	msleep(1000 + (qrand() % 5000)); //Give enough time for the rest of application to initialize
	while (!doStopWork) {
		readingLock.lock();
		if (needMesh) {
			//Start making the mesh
			needMesh = false;

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
				EVDS_Mesh_Generate(work_object,&mesh,getLODResolution(numLods-lod-1),EVDS_MESH_USE_DIVISIONS);
				result.appendMesh(mesh,lod);
				EVDS_Mesh_Destroy(mesh);
				//printf("Done mesh %p %p for level %d\n",object,mesh,lod);
			}

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
	//if (mesh) EVDS_Mesh_Destroy(mesh);
}