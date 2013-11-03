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
#ifndef FWE_EVDS_MODIFIERS_H
#define FWE_EVDS_MODIFIERS_H

#include <QThread>

#include <GLC_Mesh>
#include <GLC_3DViewInstance>

#include "evds.h"

namespace EVDS {
	class Editor;
	class Object;
	struct ObjectRendererModifierInstance {
		GLC_3DViewInstance* instance; //Instance of the modified copy
		GLC_3DViewInstance* base_instance; //Instance, which the modified copy is based off
		GLC_3DViewInstance* real_base_instance; //Instace, which is the original object
		GLC_3DViewInstance* modifier_instance; //Instance, which is the modifier
		GLC_3DRep* base_representation; //3D representation of the original object
		GLC_Matrix4x4 transformation; //Modifiers transformation
	};
	class ObjectModifiersManager : public QObject
	{
		Q_OBJECT

	public:
		ObjectModifiersManager(Editor* in_editor);
		~ObjectModifiersManager();

		//Update all modifiers
		void updateModifiers();
		//Object was removed - make sure all modifiers are updated accordingly
		void objectRemoved(Object* object);
		//Object was added - make sure all modifiers are updated accordingly
		void objectAdded(Object* object);
		//Update position of all instances of the modified object
		void objectPositionChanged(Object* object);
		//Update modifier object parameters
		void modifierChanged(Object* object);

		//Are the objects initializing? (objectAdded calls are ignored)
		void setInitializing(bool value) { initializing = value; }
		//Is initializing
		bool isInitializing() { return initializing; }

		//Get modifier instances
		QList<ObjectRendererModifierInstance>& getInstances(Object* object) { return modifierInstances[object]; }

	private slots:
		void doUpdateModifiers();

	private:
		//Finds modifiers in the given object, and updates the instances created by them
		void processUpdateModifiers(Object* object);
		//Updates positions of all things
		void processUpdatePosition(Object* object);

		//Creates a modified copy from the given object
		void createModifiedCopy(Object* modifier, Object* object);
		//Sets position of the modified instance
		void setInstancePosition(ObjectRendererModifierInstance* modifier_instance);

		//Instances created by modifier
		QMap<Object*,QList<ObjectRendererModifierInstance> > modifierInstances;

		//EVDS editor
		Editor* editor;
		//Is editor initializing
		bool initializing;
		//Should modifiers be updated
		bool shouldUpdateModifiers;
	};
}

#endif