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
#ifndef FWE_EDITOR_H
#define FWE_EDITOR_H

#include <QMainWindow>
#include <QHash>
#include <QMap>
#include <QSemaphore>

#include "evds.h"
#include "evds_antenna.h"
#include "evds_train_wheels.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QStackedLayout;
class QAction;
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
namespace EVDS {
	class Object;
	class Editor;
	class SchematicsEditor;
}
namespace FWE {
	class MainWindow;
	class EditorWindow;
	class Editor : public QMainWindow {
		Q_OBJECT

	public:
		Editor(EditorWindow* parent) : QMainWindow() { editorWindow = parent; }

		EditorWindow* getEditorWindow() { return editorWindow; }
		MainWindow* getMainWindow();
		EVDS::Object* getEditRoot();
		EVDS::Object* getEditDocument();
		EVDS::Editor* getEVDSEditor();
		EVDS::SchematicsEditor* getSchematicsEditor();

		void setModified();
		void addAction(QAction* action);

		bool getActive() { return isActive; }
		void setActive(bool active) { isActive = active; }

	protected:
		bool isActive;

	private:
		EditorWindow* editorWindow;
	};


	class EditorWindow : public QMainWindow {
		Q_OBJECT

	public:
		EditorWindow(MainWindow* window);
		~EditorWindow();

		//File operations
		void newFile();
		bool saveFile(const QString &fileName, bool autoSave = false);
		bool loadFile(const QString &fileName);

		//Shorthands for working with the current file
		QString getCurrentFile() { return currentFile; }
		bool save();
		bool saveAs();

		//Return interesting things
		MainWindow* getMainWindow() { return mainWindow; }
		EVDS::Object* getEditRoot() { return root_object; }
		EVDS::Object* getEditDocument() { return document; }
		EVDS::Editor* getEVDSEditor() { return EVDSEditor; }
		EVDS::SchematicsEditor* getSchematicsEditor() { return SchematicsEditor; }

		//Update interface
		void updateInterface(bool isInFront);
		void updateObject(EVDS::Object* object);
		void showLoadingError(const QString& errorMessage);
		void addEditorAction(Editor* editor, QAction* action);

		//List of object variables, cross-section variables by object type
		QMap<QString,QList<QMap<QString,QString> > > objectVariables;
		QMap<QString,QList<QMap<QString,QString> > > csectionVariables;

	protected:
		void closeEvent(QCloseEvent *event);

	public slots:
		//General operations
		void autoSave();
		void showVME();
		void showSchematics();
		void cut();
		void copy();
		void paste();

		//Set global project modified flag
		void setModified() { isModified = true; updateTitle(); }

		//Keep-tracker for the number of active threads
		void threadStarted() { activeThreads.release(1); }
		void threadEnded() { activeThreads.acquire(1); }

	private slots:
		void cleanupTimer();

	private:
		QSemaphore activeThreads;
		MainWindow* mainWindow;
		bool isModified;
		bool trySave();

		//Load different object types
		void loadObjectVariablesData();

		//Current opened file
		QString currentFile;
		void updateTitle();

		//Editors
		EVDS::Editor* EVDSEditor;
		EVDS::SchematicsEditor* SchematicsEditor;

		//List of editor widgets
		QWidget* editorsWidget;
		QStackedLayout* editorsLayout;
		QHash<Editor*, QAction*> editorsActions;

		//EVDS objects (editing area)
		EVDS_SYSTEM* system;
		EVDS_OBJECT* root;
		EVDS::Object* root_object;
		EVDS::Object* document;
	};
}

#endif
