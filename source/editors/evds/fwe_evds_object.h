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
		Object(EVDS_OBJECT* in_object, EVDS::Object* in_parent, EVDS::Editor* in_editor, EVDS::SchematicsEditor* in_schematics_editor);
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
		TemporaryObject(EVDS_OBJECT* in_object, QMutex* in_unlockMutex) : Object(in_object,0,0,0) {
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
