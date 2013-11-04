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

#include "evds.h"
#include "evds_antenna.h"
#include "evds_train_wheels.h"

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

		//General editor functions
		void newFile();
		bool loadFile(const QString &fileName);
		bool saveFile(const QString &fileName);
		void updateInterface(bool isInFront);
		void setModified(bool informationUpdate = true);

		//EVDS-editor specific
		void updateInformation(bool ready);
		void updateObject(Object* object);
		void propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		void loadError(const QString& error);

		//Various references to other objects
		FWE::EditorWindow* getWindow() { return window; }
		GLScene* getGLScene() { return glscene; }
		Object* getEditRoot() { return root_obj; }
		Object* getEditDocument() { return document; }
		ObjectModifiersManager* getModifiersManager() { return modifiers_manager; }

		//Object selection
		Object* getSelected() { return selected; }
		void clearSelection() { selected = NULL; }

		//List of object variables, cross-section variables by object type
		QMap<QString,QList<QMap<QString,QString> > > objectVariables;
		QMap<QString,QList<QMap<QString,QString> > > csectionVariables;

		//Counter for active working threads
		QSemaphore activeThreads;

	protected:
		void dropEvent(QDropEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);

	private slots:
		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);
		void commentsChanged();
		void cleanupTimer();
		void rootInitialized();

		void showProperties();
		void showCrossSections();
		void showHierarchy();
		void showInformation();
		void showCutsection();
		void showComments();

	private:
		QString currentFile;

		void createMenuToolbar();
		void createCSectionDock();
		void createInformationDock();
		void createCommentsDock();

		//Load different object types
		void loadObjectVariablesData();

		//Dock windows
		Dock::ObjectList*	object_list;		//List of objects
		Dock::Properties*	object_properties;	//Property sheets for objects

		//Object cross-sections
		QDockWidget*		csection_dock;
		QWidget*			csection;
		QStackedLayout*		csection_layout;
		QLabel*				csection_none;

		//Rigid body information
		QDockWidget*		bodyinfo_dock;
		QTextEdit*			bodyinfo;

		//Comments dock
		QDockWidget*		comments_dock;
		QTextEdit*			comments;

		//Main workspace
		GLScene*			glscene;
		GLView*				glview;
		ObjectModifiersManager* modifiers_manager;

		//Menus and actions
		QList<QAction*>		actions;
		QMenu*				cutsection_menu;
		QAction*			cutsection_x;
		QAction*			cutsection_y;
		QAction*			cutsection_z;

		//Parent window
		FWE::EditorWindow*	window;

		//EVDS objects (editing area)
		EVDS_SYSTEM* system;
		EVDS_OBJECT* root;
		EVDS::Object* root_obj;
		EVDS::Object* selected;
		EVDS::Object* document;

		//EVDS objects (initialized/simulation area)
		ObjectInitializer* initializer;
	};
}

#endif
