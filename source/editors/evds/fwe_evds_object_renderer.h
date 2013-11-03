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
#ifndef FWE_EVDS_OBJECT_RENDERER_H
#define FWE_EVDS_OBJECT_RENDERER_H

#include <QThread>
#include <QMutex>
#include <QSemaphore>

#include <GLC_Mesh>
#include <GLC_3DViewInstance>

#include "evds.h"

namespace EVDS {
	class Editor;
	class Object;
	class ObjectLODGenerator;
	class ObjectRenderer : public QObject
	{
		Q_OBJECT

	public:
		ObjectRenderer(Object* in_object);
		~ObjectRenderer();

		GLC_3DViewInstance* getInstance() { return glcInstance; }
		GLC_3DRep* getRepresentation() { return glcMeshRep; }

	public slots:
		//Notifies that objects mesh has changed and must be re-generated
		void meshChanged();
		//Notifies that objects position in space has changed, and all children must be recalculated
		void positionChanged();
		//Notifies that LOD meshes have been generated
		void lodMeshesGenerated();

	private:
		//GLC mesh for this object
		GLC_Mesh* glcMesh;
		GLC_3DRep* glcMeshRep;
		GLC_3DViewInstance* glcInstance;

		//Object to render
		Object* object;
		ObjectLODGenerator* lodMeshGenerator;
	};


	struct ObjectLODGeneratorResult {
		GLfloatVector verticesVector;
		GLfloatVector normalsVector;
		QList<IndexList> indicesLists;
		QList<EVDS_MESH*> meshList;
		QList<int> lodList;

		void clear();
		void appendMesh(EVDS_MESH* mesh, int lod);
		void setGLCMesh(GLC_Mesh* glcMesh, Object* object);
	};


	class ObjectLODGenerator : public QThread {
		Q_OBJECT

	public:
		ObjectLODGenerator(Object* in_object, int in_lods);

		//Get mesh (returns 0 if mesh was not generated yet)
		ObjectLODGeneratorResult* getResult();
		//Update mesh for the given object
		void updateMesh();
		//Abort thread work
		void stopWork();
		//Locked when mesh is being generated
		QMutex readingLock;

		//Get number of lods
		int getNumLODs() { return numLods; }

	public:
		static QSemaphore threadsSemaphore;

	public slots:
		void doUpdateMesh();

	signals:
		void signalLODsReady();

	protected:
		void run();
	
	private:
		float getLODResolution(int lod); //Get resolution for LOD level

		QTimer updateCallTimer;
		bool doStopWork; //Stop threads work
		bool needMesh; //Is new mesh required

		Object* object; //Object for which mesh is generated
		Editor* editor; //Objects editor
		EVDS_OBJECT* object_copy; //Copy of the object for this thread

		int numLods; //Total number of LODs
		ObjectLODGeneratorResult result; //Generated meshes
	};
}

#endif