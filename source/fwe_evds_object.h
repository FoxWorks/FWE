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
#ifndef FWE_EVDS_OBJECT_H
#define FWE_EVDS_OBJECT_H

#include <QObject>
#include <QString>
#include <QVector3D>
#include <QThread>
#include <QMutex>
#include <QHash>
#include <QTimer>
#include "evds.h"

QT_BEGIN_NAMESPACE
class QtProperty;
QT_END_NAMESPACE

class FWEPropertySheet;
namespace EVDS {
	class Editor;
	class SchematicsEditor;
	class ObjectRenderer;
	class ObjectInitializer;
	class CrossSectionEditor;
	class Object : public QObject {
		Q_OBJECT

	public:
		Object(EVDS_OBJECT* in_object, EVDS::Object* in_parent, EVDS::Editor* in_editor);
		~Object();

		void setVariable(const QString &name, double value);
		void setVariable(const QString &name, const QString &value);
		double getVariable(const QString &name);
		QString getString(const QString &name);
		QVector3D getVector(const QString &name);
		bool isVariableDefined(const QString &name);
		QString getName();
		void setName(const QString &name);
		QString getType();
		void setType(const QString &type);

		int getChildrenCount() { return children.count(); }
		Object* getChild(int index) { return children.at(index); }
		int getChildIndex(Object* child) { return children.indexOf(child); }
		Object* getParent() { return parent; }
		Object* insertNewChild(int index);
		Object* appendHiddenChild();
		void hideChild(int index);
		Object* insertChild(int index, const QString &description);
		void removeChild(int index);
		void invalidateChildren();

		bool isOxidizerTank();
		void getSheetPaperSizeInCM(float* width, float* height);

		QWidget* getPropertySheet();
		QWidget* getCrossSectionsEditor();
		EVDS::Editor* getEVDSEditor() { return editor; }
		EVDS_OBJECT* getEVDSObject() { return object; }
		int getSelectedCrossSection();
		void deleteCrossSectionsEditor();
		void setSchematicsEditor(SchematicsEditor* editor) { schematics_editor = editor; }
		SchematicsEditor* getSchematicsEditor() { return schematics_editor; }

		Object* getInitializedObject(); //Part of the entirely initialized copy of the vessel
		ObjectRenderer* getRenderer() { return renderer; }
		int getEditorUID() { return editor_uid; }

		void update(bool visually);
		//void draw(bool objectSelected);

		//Information
		void recursiveUpdateInformation(ObjectInitializer* initializer);
		double getInformationVariable(const QString &name);
		QVector3D getInformationVector(const QString &name);
		bool isInformationDefined(const QString &name);

	private slots:
		void doubleChanged(const QString& name, double value);
		void stringChanged(const QString& name, const QString& value);
		void propertyUpdate(const QString& name);
		void meshReady();

	private:
		int editor_uid;
		QHash<QString,QVector3D> info_vectors;
		QHash<QString,double> info_variables;
		QHash<QString,bool> info_defined;

		EVDS_OBJECT* object;
		EVDS::Editor* editor;
		EVDS::SchematicsEditor* schematics_editor;

		EVDS::Object* parent;
		QList<Object*> children;
		QList<Object*> hidden_children;

		ObjectRenderer* renderer;
		CrossSectionEditor* csection_editor;
		FWEPropertySheet* property_sheet;
	};


	class TemporaryObject : public Object {
		Q_OBJECT

	public:
		TemporaryObject(EVDS_OBJECT* in_object, QMutex* in_unlockMutex) : Object(in_object,0,0) {
			unlockMutex = in_unlockMutex;
		}
		~TemporaryObject() {
			if (unlockMutex) unlockMutex->unlock();
		}

	private:
		QMutex* unlockMutex;
	};


	class ObjectInitializer : public QThread {
		Q_OBJECT

	public:
		ObjectInitializer(Object* in_object);

		//Re-initialize object
		void updateObject();
		//Abort thread work
		void stopWork();
		//Locked when object is still inconsistent or when it's being read
		QMutex readingLock;

		//Get temporary object for a real object (by unique identifier)
		TemporaryObject* getObject(Object* object);

	public slots:
		void doUpdateObject();

	signals:
		void signalObjectReady();

	protected:
		void run();
	
	private:
		QTimer updateCallTimer;
		bool doStopWork; //Stop threads work
		bool needObject; //Is new object required
		bool objectCompleted; //Is object ready to be read

		Object* object; //Object which is initialized
		EVDS_OBJECT* object_copy;
	};
}

#endif
