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

#include <QtOpenGL/QGLFunctions>
#include <QThread>
#include <QMutex>
#include "evds.h"
//FIXME: remove evds from includes here

namespace EVDS {
	class Object;
	class HQRendererThread;
	class ObjectRenderer : public QObject, protected QGLFunctions
	{
		Q_OBJECT

	public:
		ObjectRenderer(Object* in_object);
		~ObjectRenderer();

		//Draws mesh using OpenGL functions
		void drawMesh(bool objectSelected);

	public slots:
		//Notifies that mesh has changed
		void meshChanged();
		//Notifies that HQ mesh is ready
		void slotHQMeshReady();
	signals:
		void signalHQMeshReady();

	private:
		//Returns EVDS_MESH for the object
		EVDS_MESH* getMesh();
		//Call this until it returns true to build a HQ mesh
		bool isHQMeshReady();

		//HQ rendering thread
		HQRendererThread* thread;
		//Was object modified?
		bool objectModified;
		//Last stored low quality mesh
		EVDS_MESH* mesh;

		//Object to render
		Object* object;
	};

	class HQRendererThread : public QThread {
		Q_OBJECT

	public:
		HQRendererThread(Object* in_object, float in_resolution);

		EVDS_MESH* getMesh();
		void updateMesh();
		void stopWork() { doStopWork = true; }
		QMutex readingLock;

	signals:
		void signalHQMeshReady();

	protected:
		void run();
	
	private:
		bool doStopWork;
		float resolution;
		bool needMesh;
		bool meshCompleted;

		EVDS_MESH* mesh;
		Object* object;
		EVDS_OBJECT* temp_object;
	};
}

#endif