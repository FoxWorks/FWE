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
#ifndef FWE_EVDS_H
#define FWE_EVDS_H

#include <QMainWindow>
#include <QFileInfo>
#include <QMap>
#include <QList>
#include "evds.h"
//#include "evds_antenna.h"

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
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
class GLC_3DViewInstance;
class FWEPropertySheet;
class ChildWindow;
namespace EVDS {
	class GLScene;
	class GLView;
	class Object;
	class ObjectInitializer;
	class ObjectTreeModel;
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
		void setModified();

		GLScene* getGLScene() { return glscene; }
		Object* getEditRoot() { return root_obj; }
		Object* getSelected() { return selected; }
		void clearSelection() { selected = NULL; }

		void updateInformation(bool ready);
		void updateObject(Object* object);
		void propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		void loadError(const QString& error);

	public:
		QMap<QString,QList<QMap<QString,QString>>> objectVariables;
		QMap<QString,QList<QMap<QString,QString>>> csectionVariables;

	protected:
		void dropEvent(QDropEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);

	private slots:
		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);
		void cleanupTimer();
		void rootInitialized();

	private:
		QString currentFile;

		void createMenuToolbar();
		void createListDock();
		void createPropertiesDock();
		void createCSectionDock();
		//void createCutsectionDock();
		//void createBodyInfoDock();

		//Object types
		void loadObjectData();

		//List of objects
		ObjectTreeModel*	list_model;
		QDockWidget*		list_dock;
		QWidget*			list;
		QTreeView*			list_tree;
		QPushButton*		list_add;
		QPushButton*		list_remove;
		QVBoxLayout*		list_layout;

		//Object properties
		QDockWidget*		properties_dock;
		QWidget*			properties;
		QStackedLayout*		properties_layout;
		FWEPropertySheet*	properties_document;

		//Object cross-sections
		QDockWidget*		csection_dock;
		QWidget*			csection;
		QStackedLayout*		csection_layout;
		QLabel*				csection_none;

		//Main workspace
		GLScene*			glscene;
		GLView*				glview;

		//Menus and actions
		QList<QAction*>		actions;

		//Parent window
		ChildWindow*		window;

		//EVDS objects (editing area)
		EVDS_SYSTEM* system;
		EVDS_OBJECT* root;
		EVDS::Object* root_obj;
		EVDS::Object* selected;

		//EVDS objects (initialized/simulation area)
		ObjectInitializer* initializer;
		EVDS_SYSTEM* initialized_system;
		EVDS_OBJECT* initialized_root;
	};
}

#endif
