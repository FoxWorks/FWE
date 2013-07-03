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
#include "fwe_schematics.h"
#include "fwe_schematics_renderer.h"
#include "fwe_evds_glscene.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_modifiers.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
SchematicsRenderingManager::SchematicsRenderingManager(SchematicsEditor* in_editor) {
	schematics_editor = in_editor;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
SchematicsRenderingManager::~SchematicsRenderingManager() {
	GLScene* glview = schematics_editor->getGLScene();

	//Remove all instances from glview
	/*for (int i = 0; i < modifierInstances.count(); i++) {
		if (glview->getCollection()->contains(modifierInstances[i].instance->id())) {
			glview->getCollection()->remove(modifierInstances[i].instance->id());
		}
		delete modifierInstances[i].instance;
	}*/
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsRenderingManager::updateInstances() {
	GLScene* glview = schematics_editor->getGLScene();

	qDebug("SchematicsRenderingManager::updateInstances()");

	//Remove all instances from glview
	for (int i = 0; i < schematicsInstances.count(); i++) {
		if (glview->getCollection()->contains(schematicsInstances[i].instance->id())) {
			glview->getCollection()->remove(schematicsInstances[i].instance->id());
		}
		delete schematicsInstances[i].instance;
	}
	schematicsInstances.clear();

	//Process all object starting from root
	if (schematics_editor->getCurrentSheet()) {
		processUpdateInstances(schematics_editor->getCurrentSheet());
	}

	//Set position
	updatePositions();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsRenderingManager::updatePositions() {
	//Set positions of all children
	for (int i = 0; i < schematicsInstances.count(); i++) {
		setInstancePosition(&schematicsInstances[i]);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsRenderingManager::processUpdateInstances(Object* element) {
	//Process all children first
	for (int i = 0; i < element->getChildrenCount(); i++) {
		processUpdateInstances(element->getChild(i));
	}

	//Create instances for object elements
	if (element->getType() == "foxworks.schematics.element") {
		EVDS_OBJECT* evds_object = 0;
		QString reference = element->getString("reference");

		if (reference != "") {
			EVDS_SYSTEM* system;
			EVDS_Object_GetSystem(element->getEVDSObject(),&system);
			EVDS_System_QueryObject(schematics_editor->getEVDSEditor()->getEditRoot()->getEVDSObject(),
				reference.toAscii().data(),0,&evds_object);

			if (evds_object) {
				Object* object;
				EVDS_Object_GetUserdata(evds_object,(void**)&object);
				createInstance(element,object);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsRenderingManager::setInstancePosition(SchematicsObjectInstance* schematics_instance) {
	//Copy transformation from original instance and add extra one defined by transformation
	schematics_instance->instance->resetMatrix();

	//Start with instances objects own matrix
	schematics_instance->instance->multMatrix(schematics_instance->base_instance->matrix());
	//Add transformation for the current modifier
	schematics_instance->instance->multMatrix(schematics_instance->transformation);

	//Update visibility of this object
	schematics_instance->instance->setVisibility(schematics_instance->base_instance->isVisible());

	//Add/replace to update position
	GLScene* glview = schematics_editor->getGLScene();
	if (glview->getCollection()->contains(schematics_instance->instance->id())) {
		glview->getCollection()->remove(schematics_instance->instance->id());
	}
	glview->getCollection()->add(*schematics_instance->instance);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsRenderingManager::createInstance(Object* element, Object* object) {
	//Calculate scale
	double scale = element->getVariable("scale");
	if (scale <= 0.0) {
		scale = schematics_editor->getCurrentSheet()->getVariable("sheet.scale");
		if (scale <= 0.0) scale = 1.0;
	}

	//Calculate transformation matrix
	EVDS_STATE_VECTOR vector;
	EVDS_Object_GetStateVector(element->getEVDSObject(),&vector);

	//Apply transformations
	GLC_Matrix4x4 scaling;
	GLC_Matrix4x4 transformation;
	EVDS_MATRIX rotationMatrix;
	EVDS_Quaternion_ToMatrix(&vector.orientation,rotationMatrix);
	scaling.setMatScaling(1/scale,1/scale,1/scale);

	transformation = transformation*GLC_Matrix4x4(vector.position.x,vector.position.y,vector.position.z);
	transformation = transformation*scaling;
	transformation = transformation*GLC_Matrix4x4(rotationMatrix);

	//Create instance
	SchematicsObjectInstance schematics_instance;
	schematics_instance.base_instance = object->getRenderer()->getInstance();
	schematics_instance.base_representation = object->getRenderer()->getRepresentation();
	schematics_instance.instance = new GLC_3DViewInstance(*schematics_instance.base_representation);
	schematics_instance.transformation = transformation;

	//Add instance to scene
	schematics_editor->getGLScene()->getCollection()->add(*schematics_instance.instance);
	//Append instance
	schematicsInstances.append(schematics_instance);


	//Create instances for modifiers of this object
	QList<ObjectRendererModifierInstance>& list = object->getEVDSEditor()->getModifiersManager()->getInstances(object);
	for (int i = 0; i < list.count(); i++) { //FIXME: call update from modifiers updater
		//Create instance
		SchematicsObjectInstance schematics_instance;
		schematics_instance.base_instance = list[i].instance;
		schematics_instance.base_representation = list[i].base_representation;
		schematics_instance.instance = new GLC_3DViewInstance(*schematics_instance.base_representation);
		schematics_instance.transformation = transformation;

		//Add instance to scene
		schematics_editor->getGLScene()->getCollection()->add(*schematics_instance.instance);
		//Append instance
		schematicsInstances.append(schematics_instance);
	}


	//Create instances for children
	for (int i = 0; i < object->getChildrenCount(); i++) {
		createInstance(element,object->getChild(i));
	}
		
	//if ((object->getType() == "modifier") && (object != modifier)) {
		//for (int i = 0; i < object->getChildrenCount(); i++) {
			//createModifiedCopy(modifier,object->getChild(i));
		//}
	//}
}