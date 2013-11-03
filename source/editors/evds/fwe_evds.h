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
#include <QAtomicInt>
#include "evds.h"
#include "evds_antenna.h"
#include "evds_train_wheels.h"

#include "fwe_main.h"

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
class ChildWindow;
namespace Dock {
	class ObjectList;
}
namespace EVDS {
	class GLScene;
	class GLView;
	class Object;
	class ObjectInitializer;
	class ObjectTreeModel;
	class ObjectModifiersManager;
	class Editor : public QMainWindow
	{
		Q_OBJECT

	public:
		Editor(ChildWindow* in_window);
		~Editor();

		void newFile();
		bool loadFile(const QString &fileName);
		bool saveFile(const QString &fileName);
		void updateInterface(bool isInFront);
		void setModified(bool informationUpdate = true);

		ChildWindow* getWindow() { return window; }
		GLScene* getGLScene() { return glscene; }
		Object* getEditRoot() { return root_obj; }
		Object* getEditDocument() { return document; }
		Object* getSelected() { return selected; }
		void clearSelection() { selected = NULL; }
		ObjectModifiersManager* getModifiersManager() { return modifiers_manager; }

		void updateInformation(bool ready);
		void updateObject(Object* object);
		void propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		void loadError(const QString& error);

	public:
		QMap<QString,QList<QMap<QString,QString> > > objectVariables;
		QMap<QString,QList<QMap<QString,QString> > > csectionVariables;

	private:
		QAtomicInt activeThreads;

	public:
		void addActiveThread();
		void removeActiveThread();
		void waitForThreads();

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
		void createListDock();
		void createPropertiesDock();
		void createCSectionDock();
		void createInformationDock();
		void createCommentsDock();

		//Object types
		void loadObjectData();

		//List of objects
		Dock::ObjectList*	object_list;

		//Object properties
		QDockWidget*		properties_dock;
		QWidget*			properties;
		QStackedLayout*		properties_layout;

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
		ChildWindow*		window;

		//EVDS objects (editing area)
		EVDS_SYSTEM* system;
		EVDS_OBJECT* root;
		EVDS::Object* root_obj;
		EVDS::Object* selected;
		EVDS::Object* document;

		//EVDS objects (initialized/simulation area)
		ObjectInitializer* initializer;
		EVDS_SYSTEM* initialized_system;
		EVDS_OBJECT* initialized_root;
	};
}

#endif
