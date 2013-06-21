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
	struct ObjectRendererModifierInstance {
		GLC_3DViewInstance* instance;
		GLC_3DViewInstance* base_instance;
		GLC_3DRep* representation;
		GLC_Matrix4x4 transformation;
	};
	class ObjectRenderer : public QObject
	{
		Q_OBJECT

	public:
		ObjectRenderer(Object* in_object);
		~ObjectRenderer();

		GLC_3DViewInstance* getInstance() { return glcInstance; }
		GLC_3DRep* getRepresentation() { return glcMeshRep; }

		//Instances created by modifier
		QList<ObjectRendererModifierInstance> modifierInstances;

	public slots:
		//Notifies that objects mesh has changed and must be re-generated
		void meshChanged();
		//Notifies that objects position in space has changed, and all children must be recalculated
		void positionChanged(bool travel_up = false);
		//Notifies that LOD meshes have been generated
		void lodMeshesGenerated();

	private:
		//Recursively add modifier instances
		void addModifierInstances(Object* child);
		//Upwards recursion through objects tree that removes instances of this object from modifiers
		void removeFromModifiers(Object* parent);

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