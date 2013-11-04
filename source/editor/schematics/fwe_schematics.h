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
#ifndef FWE_SCHEMATICS_H
#define FWE_SCHEMATICS_H

#include <QMainWindow>
#include "fwe_evds.h"

#include "fwe_editor.h"

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
namespace FWE {
	class EditorWindow;
}
namespace Dock {
	class ObjectList;
}
namespace EVDS {
	class GLScene;
	class GLView;
	class Object;
	class ObjectTreeModel;
	class SchematicsRenderingManager;
	class SchematicsEditor : public FWE::Editor {
		Q_OBJECT

	public:
		SchematicsEditor(FWE::EditorWindow* in_window, EVDS::Editor* in_editor);
		~SchematicsEditor();

		//General editor functions
		void initializeForFile();
		void updateInterface(bool isInFront);
		void setEditorHidden(bool isHidden);

		void updateObject(Object* object);
		void propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);

		//References to other objects
		EVDS::Editor* getEVDSEditor() { return editor; }
		Object* getCurrentSheet() { return sheet; }
		void setCurrentSheet(Object* new_sheet) { sheet = new_sheet; }
		Object* getRoot() { return root; }
		GLScene* getGLScene() { return glscene; }
		SchematicsRenderingManager* getSchematicsRenderingManager() { return rendering_manager; }

		//Object selection
		Object* getSelected() { return selected; }
		void clearSelection() { selected = NULL; }

	protected:
		void dropEvent(QDropEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);

	private slots:
		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);
		void commentsChanged();
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
		//void createCSectionDock();
		//void createInformationDock();
		void createCommentsDock();

		//Object types
		//void loadObjectData();

		//List of schematics elements
		Dock::ObjectList*	elements_list;
		Dock::ObjectList*	object_list;
		Dock::Properties*	element_properties;

		//Object cross-sections
		/*QDockWidget*		csection_dock;
		QWidget*			csection;
		QStackedLayout*		csection_layout;
		QLabel*				csection_none;

		//Rigid body information
		QDockWidget*		bodyinfo_dock;
		QTextEdit*			bodyinfo;*/

		//Comments dock
		QDockWidget*		comments_dock;
		QTextEdit*			comments;

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
