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
#ifndef FWE_SCHEMATICS_H
#define FWE_SCHEMATICS_H

#include <QMainWindow>
#include "fwe_evds.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QPushButton;
class QTreeView;
class QDockWidget;
class QVBoxLayout;
class QStackedLayout;
class QLabel;
class QModelIndex;
class QSlider;
class QCheckBox;
class QTextEdit;
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
class GLC_3DViewInstance;
class ChildWindow;
namespace EVDS {
	class GLScene;
	class GLView;
	class Object;
	class ObjectTreeModel;
	class SchematicsRenderingManager;
	class SchematicsEditor : public QMainWindow
	{
		Q_OBJECT

	public:
		SchematicsEditor(ChildWindow* in_window, EVDS::Editor* in_editor);
		~SchematicsEditor();

		void initializeForFile();
		void updateInterface(bool isInFront);
		void setModified();
		void setEditorHidden(bool isHidden);

		EVDS::Editor* getEVDSEditor() { return editor; }
		Object* getCurrentSheet() { return sheet; }
		void setCurrentSheet(Object* new_sheet) { sheet = new_sheet; }
		Object* getRoot() { return root; }
		ChildWindow* getWindow() { return window; }
		GLScene* getGLScene() { return glscene; }
		Object* getSelected() { return selected; }
		void clearSelection() { selected = NULL; }
		SchematicsRenderingManager* getSchematicsRenderingManager() { return rendering_manager; }

		void updateObject(Object* object);
		void propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		//void loadError(const QString& error);

	protected:
		void dropEvent(QDropEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);

	private slots:
		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);
		/*

		void showProperties();
		void showCrossSections();
		void showHierarchy();
		void showInformation();
		void showCutsection();
		void showComments();*/

	private:
		QString currentFile;

		void invalidateChildren(Object* object);
		void createMenuToolbar();
		void createListDock();
		void createObjectListDock();
		void createPropertiesDock();
		//void createCSectionDock();
		//void createInformationDock();
		//void createCommentsDock();

		//Object types
		//void loadObjectData();

		//List of schematics elements
		ObjectTreeModel*	list_model;
		QDockWidget*		list_dock;
		QWidget*			list;
		QTreeView*			list_tree;
		QPushButton*		list_add;
		QPushButton*		list_remove;
		QVBoxLayout*		list_layout;

		//List of edited objects
		ObjectTreeModel*	objectlist_model;
		QDockWidget*		objectlist_dock;
		QWidget*			objectlist;
		QTreeView*			objectlist_tree;
		QVBoxLayout*		objectlist_layout;

		//Object properties
		QDockWidget*		properties_dock;
		QWidget*			properties;
		QStackedLayout*		properties_layout;

		//Object cross-sections
		/*QDockWidget*		csection_dock;
		QWidget*			csection;
		QStackedLayout*		csection_layout;
		QLabel*				csection_none;

		//Rigid body information
		QDockWidget*		bodyinfo_dock;
		QTextEdit*			bodyinfo;

		//Comments dock
		QDockWidget*		comments_dock;
		QTextEdit*			comments;*/

		//Main workspace
		GLScene*			glscene;
		GLView*				glview;

		//Menus and actions
		QList<QAction*>		actions;
/*
		QMenu*				cutsection_menu;
		QAction*			cutsection_x;
		QAction*			cutsection_y;
		QAction*			cutsection_z;
*/
		//Parent window
		ChildWindow*		window;

		//EVDS objects (editing area)
		EVDS::Editor* editor;
		EVDS::Object* root;
		EVDS::Object* selected;
		EVDS::Object* sheet;
		EVDS::Object* prev_sheet;

		SchematicsRenderingManager* rendering_manager;
	};
}

#endif
