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

QT_BEGIN_NAMESPACE
class QWidget;
class QStackedLayout;
class QAction;
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
namespace EVDS {
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

		EditorWindow* getChildWindow() { return editorWindow; }
		MainWindow* getMainWindow();
		void setModified();
		void addAction(QAction* action);

		bool getActive() { return isActive; }
		void setActive(bool active) { isActive = active; }

	private:
		bool isActive;
		EditorWindow* editorWindow;
	};


	class EditorWindow : public QMainWindow {
		Q_OBJECT

	public:
		EditorWindow(MainWindow* window);

		//File operations
		void newFile();
		bool saveFile(const QString &fileName, bool autoSave = false);
		bool loadFile(const QString &fileName);

		//Shorthands for working with the current file
		QString getCurrentFile() { return currentFile; }
		bool save();
		bool saveAs();

		//Return main window pointer
		MainWindow* getMainWindow() { return mainWindow; }

		//Update interface
		void updateInterface(bool isInFront);
		void addEditorAction(Editor* editor, QAction* action);

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

	private:
		bool isModified;
		bool trySave();

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

		//Main editor window
		MainWindow* mainWindow;
	};
}

#endif
