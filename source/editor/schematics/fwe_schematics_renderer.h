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