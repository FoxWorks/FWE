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
#include "evds_antenna.h"

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
namespace EVDS {
	class Object;
	class ObjectTreeModel;
	class GLWidget;
	class FWPropertySheet;
	struct ObjectType {
		QString evds_type;
		QString print_name;
	};
	class Editor : public MdiChild
	{
		Q_OBJECT

	public:
		Editor(MainWindow* window);
		~Editor();

		virtual void newFile();
		virtual bool loadFile(const QString &fileName);
		virtual bool save();
		virtual bool saveAs();
		virtual bool saveFile(const QString &fileName);

		virtual void updateMenus(bool isInFront);

		virtual QString getCurrentFile() { return currentFile; }

		Object* getRoot() { return root_obj; }
		Object* getSelected() { return selected; }

		QMap<QString,QList<QMap<QString,QString>>> objectVariables;
		QMap<QString,QList<QMap<QString,QString>>> csectionVariables;

		void updateObject(Object* object);
		void updateObjectInformation(Object* object);
		void propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet);
		void setModified() { isModified = true; updateTitle(); }
		void loadError(const QString& error);

	private slots:
		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);
		void cleanupTimer();
		void updateShaders();
		void showProperties();
		void showCrossSections();
		void showHierarchy();
		void showCutsection();
		void updateCutsection();
		void showBodyInformation();
		void updateBodyInformation(Object* object);

	protected:
		void closeEvent(QCloseEvent *event);

	private:
		bool trySave();
		void updateTitle();
		QString currentFile;
		bool isModified;

		void createListDock();
		void createPropertiesDock();
		void createCSectionDock();
		void createCutsectionDock();
		void createBodyInfoDock();

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
		FWPropertySheet*	properties_document;

		//Object cross-sections
		QDockWidget*		csection_dock;
		QWidget*			csection;
		QStackedLayout*		csection_layout;
		QLabel*				csection_none;

		//Setup clip planes for GLView
		QDockWidget*		cutsection_dock;
		QWidget*			cutsection;
		QCheckBox*			cutsection_enabled;
		QSlider*			cutsection_pitch;
		QLabel*				cutsection_pitch_label;
		QSlider*			cutsection_yaw;
		QLabel*				cutsection_yaw_label;
		QSlider*			cutsection_offset;
		QLabel*				cutsection_offset_label;

		//Rigid body information
		QDockWidget*		bodyinfo_dock;
		QWidget*			bodyinfo;
		QLabel*				bodyinfo_cm;
		QLabel*				bodyinfo_total_cm;
		QLabel*				bodyinfo_inertia_tensor;
		QLabel*				bodyinfo_gyration_tensor;
		QLabel*				bodyinfo_mass;
		QLabel*				bodyinfo_total_mass;
		QLabel*				bodyinfo_f1;
		QLabel*				bodyinfo_f2;

		//Main window
		GLWidget*			glview;

		//Menus and actions
		QList<QAction*>		actions;

		//EVDS objects
		EVDS_SYSTEM* system;
		EVDS_OBJECT* root;
		EVDS::Object* root_obj;
		EVDS::Object* selected;
	};
}

#endif
