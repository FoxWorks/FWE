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
#include "fwe_evds_modifiers.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectModifiersManager::ObjectModifiersManager(Editor* in_editor) {
	editor = in_editor;
	initializing = false;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectModifiersManager::~ObjectModifiersManager() {
	GLScene* glview = editor->getGLScene();

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
void ObjectModifiersManager::updateModifiers() {
	GLScene* glview = editor->getGLScene();

	qDebug("ObjectModifiersManager::updateModifiers()");

	//Remove all instances from glview
	QMapIterator<Object*,QList<ObjectRendererModifierInstance> > iterator(modifierInstances);
	while (iterator.hasNext()) {
		iterator.next();
		for (int i = 0; i < iterator.value().count(); i++) {
			if (glview->getCollection()->contains(iterator.value()[i].instance->id())) {
				glview->getCollection()->remove(iterator.value()[i].instance->id());
			}
			delete iterator.value()[i].instance;
		}
		iterator.value();
	}
	modifierInstances.clear();

	//Process all object starting from root
	processUpdateModifiers(editor->getEditRoot());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::processUpdateModifiers(Object* object) {
	//Process all children first
	for (int i = 0; i < object->getChildrenCount(); i++) {
		processUpdateModifiers(object->getChild(i));
	}

	//If object is a modifier, create copies of its children
	if (object->getType() == "modifier") {
		for (int i = 0; i < object->getChildrenCount(); i++) {
			createModifiedCopy(object,object->getChild(i));
		}

		//Set positions of all children
		for (int i = 0; i < modifierInstances[object].count(); i++) {
			setInstancePosition(&modifierInstances[object][i]);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::processUpdatePosition(Object* object) {
	//Process all children first
	for (int i = 0; i < object->getChildrenCount(); i++) {
		processUpdatePosition(object->getChild(i));
	}

	//Set positions of all children
	if (object->getType() == "modifier") {
		for (int i = 0; i < modifierInstances[object].count(); i++) {
			setInstancePosition(&modifierInstances[object][i]);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::setInstancePosition(ObjectRendererModifierInstance* modifier_instance) {
	//Move instance as requested by modifier
	modifier_instance->instance->resetMatrix();

	//Start with instances objects own matrix
	modifier_instance->instance->multMatrix(modifier_instance->base_instance->matrix());
	//Subtract matrix of the current modifier (to go into its local coords)
	modifier_instance->instance->multMatrix(modifier_instance->modifier_instance->matrix().inverted());
	//Add transformation for the current modifier
	modifier_instance->instance->multMatrix(modifier_instance->transformation);
	//Add matrix of current transformation again (go into global coords)
	modifier_instance->instance->multMatrix(modifier_instance->modifier_instance->matrix());

	//Add/replace to update position
	GLScene* glview = editor->getGLScene();
	if (glview->getCollection()->contains(modifier_instance->instance->id())) {
		glview->getCollection()->remove(modifier_instance->instance->id());
	}
	glview->getCollection()->add(*modifier_instance->instance);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::createModifiedCopy(Object* modifier, Object* object) {
	//Get modifier information
	int vector1_count = modifier->getVariable("vector1.count");
	int vector2_count = modifier->getVariable("vector2.count");
	int vector3_count = modifier->getVariable("vector3.count");
	float circular_step = modifier->getVariable("circular.step");
	float circular_radial_step = modifier->getVariable("circular.radial_step");
	float circular_normal_step = modifier->getVariable("circular.normal_step");
	float circular_arc_length = modifier->getVariable("circular.arc_length");
	float circular_radius = modifier->getVariable("circular.radius");
	float circular_rotate = modifier->getVariable("circular.rotate");
	QVector3D vector1 = QVector3D(
		modifier->getVariable("vector1.x"),
		modifier->getVariable("vector1.y"),
		modifier->getVariable("vector1.z"));
	QVector3D vector2 = QVector3D(
		modifier->getVariable("vector2.x"),
		modifier->getVariable("vector2.y"),
		modifier->getVariable("vector2.z"));
	QVector3D vector3 = QVector3D(
		modifier->getVariable("vector3.x"),
		modifier->getVariable("vector3.y"),
		modifier->getVariable("vector3.z"));

	//Make sure master copy remains
	if (vector1_count < 1) vector1_count = 1;
	if (vector2_count < 1) vector2_count = 1;
	if (vector3_count < 1) vector3_count = 1;

	//Fix modifier parameters just like the EVDS does
	if (circular_step == 0.0) {
		if (circular_arc_length == 0.0) circular_arc_length = 360.0;
		circular_step = circular_arc_length / ((double)vector1_count);
	}

	//Add instances as moved by modifier
	for (int i = 0; i < vector1_count; i++) {
		for (int j = 0; j < vector2_count; j++) {
			for (int k = 0; k < vector3_count; k++) {
				//Create transformation
				GLC_Matrix4x4 transformation;
				if (modifier->getString("pattern") == "circular") {
					//Get circle parameters
					QVector3D normal = vector1;
					QVector3D direction = vector2;
					if (normal.length() == 0.0) normal.setX(1.0);
					if (direction.length() == 0.0) direction.setZ(1.0);
					normal.normalize();
					direction.normalize();

					//Do not generate first ring if radius is zero (only concentric objects)
					if ((circular_radius == 0.0) && (j == 0)) continue;
					//Do not generate first object is radius is non-zero
					if ((circular_radius != 0.0) && (i == 0)) continue;

					//Local coordinate system
					QVector3D u = -direction;
					QVector3D v = normal.crossProduct(direction,normal);

					//Get point on circle
					double x = (circular_radius + j*circular_radial_step)*cos(EVDS_RAD(i * circular_step));
					double y = (circular_radius + j*circular_radial_step)*sin(EVDS_RAD(i * circular_step));
					QVector3D offset = direction*circular_radius + u*x + v*y + normal*circular_normal_step*k;

					//Generate transformation
					if (circular_rotate > 0.5) {
						transformation = 
							GLC_Matrix4x4(offset.x(),offset.y(),offset.z()) *
							GLC_Matrix4x4(GLC_Vector3d(normal.x(),normal.y(),normal.z()), EVDS_RAD(i * circular_step));
					} else {
						transformation = 
							//GLC_Matrix4x4(GLC_Vector3d(normal.x(),normal.y(),normal.z()), EVDS_RAD(i * circular_step)) * 
							GLC_Matrix4x4(offset.x(),offset.y(),offset.z());
					}						
				} else {
					QVector3D offset = vector1*i + vector2*j + vector3*k;
					transformation.setMatTranslate(offset.x(),offset.y(),offset.z());

					//Skip the first part of the matrix
					if ((i == 0) && (j == 0) && (k == 0)) continue;
				}


				//Create copy of the child itself
				ObjectRendererModifierInstance modifier_inst;
				modifier_inst.modifier_instance = modifier->getRenderer()->getInstance();
				modifier_inst.base_instance = object->getRenderer()->getInstance();
				modifier_inst.real_base_instance = object->getRenderer()->getInstance();
				modifier_inst.base_representation = object->getRenderer()->getRepresentation();
				modifier_inst.instance = new GLC_3DViewInstance(*modifier_inst.base_representation);
				modifier_inst.transformation = transformation;

				//Add instance to scene
				editor->getGLScene()->getCollection()->add(*modifier_inst.instance);

				//Append instance
				modifierInstances[modifier].append(modifier_inst);

				//Copy modifiers instances of the child to this modifier
				if ((object->getType() == "modifier") && (object != modifier)) {
					for (int j = 0; j < modifierInstances[object].count(); j++) {
						ObjectRendererModifierInstance modifier_inst;
						//Use the modified instance instead of original base instance
						modifier_inst.modifier_instance = modifier->getRenderer()->getInstance();
						modifier_inst.base_instance = modifierInstances[object][j].instance;
						modifier_inst.real_base_instance = modifierInstances[object][j].real_base_instance;
						modifier_inst.base_representation = modifierInstances[object][j].base_representation;
						modifier_inst.instance = new GLC_3DViewInstance(*modifier_inst.base_representation);
						modifier_inst.transformation = transformation;

						//Add instance to scene
						editor->getGLScene()->getCollection()->add(*modifier_inst.instance);

						//Remember instance
						modifierInstances[modifier].append(modifier_inst);
					}
				}
			}
		}
	}

	//Create copies of the modifiers children (not included in modifiers instances)
	//if ((object->getType() == "modifier") && (object != modifier)) {
		for (int i = 0; i < object->getChildrenCount(); i++) {
			createModifiedCopy(modifier,object->getChild(i));
		}
	//}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::objectPositionChanged(Object* object) {
	if (!initializing) {
		//updateModifiers();
		processUpdatePosition(editor->getEditRoot());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::modifierChanged(Object* object) {
	//if (object->getType() == "modifier")
	if (!initializing) updateModifiers();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::objectRemoved(Object* object) {
	if (!initializing) updateModifiers();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectModifiersManager::objectAdded(Object* object) {
	if (!initializing) updateModifiers();
}
