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