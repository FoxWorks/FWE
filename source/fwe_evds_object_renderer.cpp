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
	glcMesh = 0;
	glcInstance = 0;
	meshChanged();
	positionChanged();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectRenderer::~ObjectRenderer() {
	delete glcInstance;
	//if (mesh) EVDS_Mesh_Destroy(mesh);
	//thread->stopWork();
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
		GLWidget* glview = object->getEVDSEditor()->getGLWidget();
		if (glview->getCollection()->contains(glcInstance->id())) {
			glview->getCollection()->remove(glcInstance->id());			
		}
		glview->getCollection()->add(*glcInstance);
		//object->getEVDSEditor()->getGLWidget()->getCollection()->
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

	//Update GLC mesh from coarse mesh FIXME
	if (!glcMesh) {
		glcMesh = new GLC_Mesh();
		glcInstance = new GLC_3DViewInstance(glcMesh);
	}
	glcMesh->clear();

	//Crate temporary object and generate coarse mesh from it
	//int lod_offset = 0;
	//for (int lod = 0; lod < 5; lod++) {
	EVDS_OBJECT* temp_object;
	EVDS_Object_CopySingle(object->getEVDSObject(),0,&temp_object);
	EVDS_Object_Initialize(temp_object,1);
	EVDS_Mesh_Generate(temp_object,&coarse_mesh,16.0f,EVDS_MESH_USE_DIVISIONS); ///((float)(1+lod))
	EVDS_Object_Destroy(temp_object);

	if (coarse_mesh->num_triangles > 0) {
		GLfloatVector verticesVector;
		GLfloatVector normalsVector;
		IndexList indicesList;
		GLC_Material* glcMaterial = new GLC_Material();

		if (object->getType() == "fuel_tank") {
			if (object->isOxidizerTank()) {
				glcMaterial->setDiffuseColor(QColor(0,0,255));
			} else {
				glcMaterial->setDiffuseColor(QColor(255,255,0));
			}
		}

		for (int i = 0; i < coarse_mesh->num_vertices; i++) {
			verticesVector << coarse_mesh->vertices[i].x;
			verticesVector << coarse_mesh->vertices[i].y;
			verticesVector << coarse_mesh->vertices[i].z;
			normalsVector << coarse_mesh->normals[i].x;
			normalsVector << coarse_mesh->normals[i].y;
			normalsVector << coarse_mesh->normals[i].z;
		}
		glcMesh->addVertice(verticesVector);
		glcMesh->addNormals(normalsVector);
		for (int i = 0; i < coarse_mesh->num_triangles; i++) {
			if ((coarse_mesh->triangles[i].vertex[0].y > 0.0) &&
				(coarse_mesh->triangles[i].vertex[1].y > 0.0) &&
				(coarse_mesh->triangles[i].vertex[2].y > 0.0)) {
				indicesList << coarse_mesh->triangles[i].indices[0];//+lod_offset;
				indicesList << coarse_mesh->triangles[i].indices[1];//+lod_offset;
				indicesList << coarse_mesh->triangles[i].indices[2];//+lod_offset;
			}
		}
		//lod_offset += coarse_mesh->num_vertices;
		glcMesh->addTriangles(glcMaterial, indicesList, 0); //coarse_mesh->resolution);
		glcMesh->finish();
		//glcMesh->reverseNormals();
	} else {
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
	}

	//Add/replace in GL widget to update mesh
	/*GLWidget* glview = object->getEVDSEditor()->getGLWidget();
	if (glview->getCollection()->contains(glcInstance->id())) {
		glview->getCollection()->remove(glcInstance->id());
		//delete glcInstance;
		//glcInstance = new GLC_3DViewInstance(glcMesh);
	}
	glview->getCollection()->add(*glcInstance);*/

	//}
	//coarse_mesh
	//createMeshAndWire(*glcMesh);

	//glcMesh->finish();
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
