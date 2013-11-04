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
#ifndef FWE_EVDS_H
#define FWE_EVDS_H

#include <QMainWindow>
#include <QFileInfo>
#include <QMap>
#include <QList>
#include <QSemaphore>

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
class FWEPropertySheet;
namespace FWE {
	class EditorWindow;
}
namespace Dock {
	class ObjectList;
	class Properties;
}
namespace EVDS {
	class GLScene;
	class GLView;
	class Object;
	class ObjectInitializer;
	class ObjectTreeModel;
	class ObjectModifiersManager;
	class Editor : public FWE::Editor {
		Q_OBJECT

	public:
		Editor(FWE::EditorWindow* in_window);
		~Editor();

		//EVDS-editor specific
		void setModified(bool informationUpdate = true);
		void objectPropertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		void csectionPropertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		void finishInitializing();
		void updateInformation(bool ready);
		void updateObject(Object* object);

		//Object selection
		Object* getSelected() { return selected; }
		void clearSelection() { selected = NULL; }

		//Various references to other objects
		GLScene* getGLScene() { return glscene; }
		ObjectModifiersManager* getModifiersManager() { return modifiers_manager; }

	protected:
		void dropEvent(QDropEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);

	private slots:
		void rootInitialized(); //Information about initialized object is available
		void commentChanged();

		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);

		void showCutsection();
		void showProperties();
		void showCrossSections();
		void showHierarchy();
		void showInformation();
		void showComments();

	private:
		void createMenuToolbar(); // Create menu and toolbar
		//void createInformationDock();
		//void createCommentsDock();

		//Dock windows
		Dock::ObjectList*	object_list;			//List of objects
		Dock::Properties*	object_properties;		//Property sheets for objects
		Dock::Properties*	csection_properties;	//Property sheets for cross-sections

		//Selected object
		EVDS::Object* selected;

		//Rigid body information
		/*QDockWidget*		bodyinfo_dock;
		QTextEdit*			bodyinfo;

		//Comments dock
		QDockWidget*		comments_dock;
		QTextEdit*			comments;

		//Main workspace
		

		//Menus and actions
		QMenu*				cutsection_menu;
		QAction*			cutsection_x;
		QAction*			cutsection_y;
		QAction*			cutsection_z;*/

		//Rendering-related (OpenGL scene and modifiers manager)
		GLScene*			glscene;
		GLView*				glview;
		ObjectModifiersManager* modifiers_manager;

		//EVDS objects (initialized/simulation area)
		ObjectInitializer* initializer;
	};
}

#endif
