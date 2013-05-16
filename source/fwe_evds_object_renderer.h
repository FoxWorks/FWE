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

#include <GLC_Mesh>
#include <GLC_3DViewInstance>

#include "evds.h"

//Maximum number of LOD levels in use
#define EVDS_OBJECT_RENDERER_MAX_LODS 2

namespace EVDS {
	class Object;
	class ObjectMeshGenerator;
	class ObjectRenderer : public QObject
	{
		Q_OBJECT

	public:
		ObjectRenderer(Object* in_object);
		~ObjectRenderer();

		GLC_3DViewInstance* getInstance(int index) { return glcInstance[index]; }

	public slots:
		//Notifies that objects mesh has changed and must be re-generated
		void meshChanged();
		//Notifies that objects position in space has changed, and all children must be recalculated
		void positionChanged();
		//Notifies that mesh for LOD has been generated
		void lodMeshGenerated(int lod);

	private:
		//Get resolution for LOD level
		float getLODResolution(int lod);
		//Add mesh for LOD level
		void addLODMesh(EVDS_MESH* mesh, int lod);

		//GLC mesh for this object
		int visibleLod;
		int lodPresent[EVDS_OBJECT_RENDERER_MAX_LODS];
		int lodFinished[EVDS_OBJECT_RENDERER_MAX_LODS];
		GLC_Mesh* glcMesh[EVDS_OBJECT_RENDERER_MAX_LODS];
		GLC_3DViewInstance* glcInstance[EVDS_OBJECT_RENDERER_MAX_LODS];

		//Threads for LOD levels
		ObjectMeshGenerator* lodMeshGenerators[EVDS_OBJECT_RENDERER_MAX_LODS];

		//Object to render
		Object* object;
	};

	class ObjectMeshGenerator : public QThread {
		Q_OBJECT

	public:
		ObjectMeshGenerator(Object* in_object, float in_resolution, int in_lod);

		//Get mesh (returns 0 if mesh was not generated yet)
		EVDS_MESH* getMesh();
		//Update mesh for the given object
		void updateMesh();
		//Abort thread work
		void stopWork() { doStopWork = true; }
		//Locked when mesh is being generated
		QMutex readingLock;

	signals:
		void signalMeshReady(int lod);

	protected:
		void run();
	
	private:
		float resolution; //Target resolution
		int lod; //Target LOD level

		bool doStopWork; //Stop threads work
		bool needMesh; //Is new mesh required
		bool meshCompleted; //Is mesh ready to be read

		Object* object; //Object for which mesh is generated
		EVDS_MESH* mesh; //Generated mesh
		EVDS_OBJECT* object_copy; //Copy of the object for this thread
	};
}

#endif