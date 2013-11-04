////////////////////////////////////////////////////////////////////////////////
/// @file
////////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2012-2013, Black Phoenix
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see http://www.gnu.org/licenses/.
////////////////////////////////////////////////////////////////////////////////
#include <evds.h>
#include "fwe_schematics.h"
#include "fwe_schematics_renderer.h"
#include "fwe_glscene.h"
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

	//Update GL scene
	glview->update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsRenderingManager::updatePositions() {
	qDebug("SchematicsRenderingManager::updatePositions()");

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
			EVDS_System_QueryByReference(schematics_editor->getEVDSEditor()->getEditRoot()->getEVDSObject(),
				reference.toAscii().data(),0,&evds_object);

			if (evds_object) {
				Object* object;
				EVDS_Object_GetUserdata(evds_object,(void**)&object);
				if (object != schematics_editor->getEVDSEditor()->getEditRoot()) {
					createInstance(element,object,true);
				}
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
GLC_Matrix4x4 SchematicsRenderingManager::getTransformationMatrix(Object* element) {
	//Calculate scale
	double scale = element->getVariable("scale");
	if (scale <= 0.0) {
		//if (firstRecursive) { //Use default scale for firstmost object
			scale = schematics_editor->getCurrentSheet()->getVariable("sheet.scale");
			if (scale <= 0.0) scale = 1.0;
		//} else { //No scaling change
			//scale = 1.0;
		//}
	}

	//Get state vector
	EVDS_STATE_VECTOR vector;
	EVDS_Object_GetStateVector(element->getEVDSObject(),&vector);

	//Calculate transformation
	GLC_Matrix4x4 scaling;
	GLC_Matrix4x4 transformation;
	EVDS_MATRIX rotationMatrix;
	EVDS_Quaternion_ToMatrix(&vector.orientation,rotationMatrix);
	scaling.setMatScaling(1/scale,1/scale,1/scale);

	transformation = transformation*GLC_Matrix4x4(vector.position.x,vector.position.y,vector.position.z);
	transformation = transformation*GLC_Matrix4x4(rotationMatrix);
	if (element->getParent() && (element->getParent()->getType() != "foxworks.schematics.sheet")) {
		transformation = transformation*getTransformationMatrix(element->getParent());
	} else {
		transformation = transformation*scaling; //First operation is scaling
	}
	return transformation;
}

void SchematicsRenderingManager::setInstancePosition(SchematicsObjectInstance* schematics_instance) {
	//Copy transformation from original instance and add extra one defined by transformation
	schematics_instance->instance->resetMatrix();

	//Start with instances objects own matrix
	schematics_instance->instance->multMatrix(schematics_instance->base_instance->matrix());
	//Add transformation for the current modifier
	schematics_instance->instance->multMatrix(getTransformationMatrix(schematics_instance->element));

	//Update visibility of this object
	if (schematics_instance->resetVisibility) {
		schematics_instance->instance->setVisibility(true);
	} else {
		schematics_instance->instance->setVisibility(schematics_instance->base_instance->isVisible());
	}

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
void SchematicsRenderingManager::createInstance(Object* element, Object* object, bool resetVisibility) {
	//Create instance
	SchematicsObjectInstance schematics_instance;
	schematics_instance.base_instance = object->getRenderer()->getInstance();
	schematics_instance.base_representation = object->getRenderer()->getRepresentation();
	schematics_instance.instance = new GLC_3DViewInstance(*schematics_instance.base_representation);
	schematics_instance.element = element;
	schematics_instance.resetVisibility = resetVisibility;

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
		schematics_instance.element = element;
		schematics_instance.resetVisibility = resetVisibility;

		//Add instance to scene
		schematics_editor->getGLScene()->getCollection()->add(*schematics_instance.instance);
		//Append instance
		schematicsInstances.append(schematics_instance);
	}


	//Create instances for children
	for (int i = 0; i < object->getChildrenCount(); i++) {
		createInstance(element,object->getChild(i),resetVisibility);
	}
		
	//if ((object->getType() == "modifier") && (object != modifier)) {
		//for (int i = 0; i < object->getChildrenCount(); i++) {
			//createModifiedCopy(modifier,object->getChild(i));
		//}
	//}
}