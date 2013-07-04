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
#ifndef FWE_SCHEMATICS_RENDERER_H
#define FWE_SCHEMATICS_RENDERER_H

#include <QThread>

#include <GLC_Mesh>
#include <GLC_3DViewInstance>

#include "evds.h"

namespace EVDS {
	class Object;
	class SchematicsEditor;
	struct SchematicsObjectInstance {
		GLC_3DViewInstance* instance; //Instance of the object at schematics
		GLC_3DViewInstance* base_instance; //Instance of the original object
		GLC_3DRep* base_representation; //3D representation of the original object
		Object* element; //Element that created this instance
		bool resetVisibility;
	};
	class SchematicsRenderingManager : public QObject
	{
		Q_OBJECT

	public:
		SchematicsRenderingManager(SchematicsEditor* in_editor);
		~SchematicsRenderingManager();

		//Re-create instances
		void updateInstances();
		//Re-position instances
		void updatePositions();

	private:
		//Find all elements and update instances for them
		void processUpdateInstances(Object* element);
		//Create instance given element description and target object
		void createInstance(Object* element, Object* object, bool resetVisibility);
		//Sets position of the modified instance
		void setInstancePosition(SchematicsObjectInstance* schematics_instance);

		//Get transformation matrix for element
		GLC_Matrix4x4 getTransformationMatrix(Object* element);

		//Instances created for schematics
		QList<SchematicsObjectInstance> schematicsInstances;

		//Schematics editor
		SchematicsEditor* schematics_editor;
	};
}

#endif