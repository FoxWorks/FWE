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
#include <QCloseEvent>
#include <QWidget>
#include <QDockWidget>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QLabel>
#include <QTimer>
#include <QModelIndex>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QSlider>
#include <QCheckBox>
#include <QTextEdit>
#include <QStack>
#include <QXmlStreamReader>

#include <GLC_3DViewCollection>
#include <GLC_3DViewInstance>
#include <GLC_Context>
#include <QGLWidget>

#include <math.h>

#include "fwe_main.h"
#include "fwe_editor.h"

#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_model.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_modifiers.h"
#include "fwe_glscene.h"
#include "fwe_prop_sheet.h"

#include "fwe_dock_objectlist.h"
#include "fwe_dock_properties.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::Editor(FWE::EditorWindow* in_window) : FWE::Editor(in_window) {
	selected = NULL;
	setAcceptDrops(true);

	//Create initializer thread
	initializer = new ObjectInitializer(getEditorWindow()->getEditRoot());
	connect(initializer, SIGNAL(signalObjectReady()), this, SLOT(rootInitialized()), Qt::QueuedConnection);
	initializer->start();
	initializer->updateObject(); //Must be called before first call to getObject
   
	//Create parts of main UI
	createMenuToolbar();

	//Create list of objects
	object_list = new Dock::ObjectList(getEditorWindow()->getEditRoot(),this);
	connect(object_list, SIGNAL(addObject()), this, SLOT(addObject()));
	connect(object_list, SIGNAL(removeObject()), this, SLOT(removeObject()));
	connect(object_list, SIGNAL(selectObject(const QModelIndex&)), this, SLOT(selectObject(const QModelIndex&)));
	addDockWidget(Qt::LeftDockWidgetArea, object_list);

	//Create properties sheet for objects
	object_properties = new Dock::Properties(this);
	addDockWidget(Qt::LeftDockWidgetArea, object_properties);

	//Create properties sheet for cross-sections
	csection_properties = new Dock::Properties(this);
	csection_properties->setWindowTitle("Cross-section Properties");
	addDockWidget(Qt::RightDockWidgetArea, csection_properties);

	//Setup initial layout
	object_list->raise();

	//Create informational docks
	//createInformationDock();
	//createCommentsDock();


	//Create working area/main 3D widget
	glview = new GLView(this);
	glscene = new GLScene(0,this,0,this);
	glview->setScene(glscene);
	setCentralWidget(glview);
	
	//Create modifiers manager
	modifiers_manager = new ObjectModifiersManager(this);
	modifiers_manager->setInitializing(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::~Editor() {
	/*modifiers_manager->setInitializing(true); //Prevent updates of modifiers list

	initializer->stopWork();
	delete initializer;

	qDebug("Editor::~Editor: cleaning up EVDS objects");
	delete root_obj;
	qDebug("Editor::~Editor: waiting for remaining threads");
	while (activeThreads.available()) ;

	//Destroy EVDS system
	qDebug("Editor::~Editor: destroying system");
	EVDS_System_Destroy(system);

	cutsection_menu->deleteLater();*/
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createMenuToolbar() {
	/*QAction* action;

	//--------------------------------------------------------------------------
	// View menu
	action = new QAction(QIcon(":/icon/evds/hierarchy.png"), tr("Objects &Hierarchy"), this);
	action->setStatusTip("Show hierarchy of objects in the EVDS file");
	connect(action, SIGNAL(triggered()), this, SLOT(showHierarchy()));
	getMainWindow()->getViewMenu()->addAction(action);
	addAction(action);

	action = new QAction(QIcon(":/icon/evds/properties.png"), tr("Object &Properties"), this);
	action->setStatusTip("Show properties sheet of the object or current EVDS file");
	connect(action, SIGNAL(triggered()), this, SLOT(showProperties()));
	getMainWindow()->getViewMenu()->addAction(action);
	addAction(action);

	action = new QAction(QIcon(":/icon/evds/csections.png"), tr("&Cross Sections Editor"), this);
	action->setStatusTip("Show cross sections editor");
	connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	getMainWindow()->getViewMenu()->addAction(action);
	addAction(action);

	addAction(getMainWindow()->getViewMenu()->addSeparator());

	action = new QAction(tr("Object &Information"), this);
	action->setStatusTip("Show information about selected object or the current EVDS file");
	connect(action, SIGNAL(triggered()), this, SLOT(showInformation()));
	getMainWindow()->getViewMenu()->addAction(action);
	addAction(action);

	action = new QAction(tr("Object &Comments"), this);
	action->setStatusTip("Show comments associated with the selected object");
	connect(action, SIGNAL(triggered()), this, SLOT(showComments()));
	getMainWindow()->getViewMenu()->addAction(action);
	addAction(action);

	//action = new QAction(QIcon(":/icon/evds/cutsection.png"), tr("Show Cut&section"), this);
	//action->setStatusTip("Show cutsection by an primary plane");
	//actions.append(action);
	//window->getMainWindow()->getViewMenu()->addAction(action);

	cutsection_x = new QAction(QIcon(":/icon/glview/view_left.png"),  tr("Plane &X"), this);
	cutsection_y = new QAction(QIcon(":/icon/glview/view_front.png"), tr("Plane &Y"), this);
	cutsection_z = new QAction(QIcon(":/icon/glview/view_top.png"),   tr("Plane &Z"), this);
	cutsection_x->setProperty("index",0);
	cutsection_y->setProperty("index",1);
	cutsection_z->setProperty("index",2);
	cutsection_x->setCheckable(true);
	cutsection_y->setCheckable(true);
	cutsection_z->setCheckable(true);
	connect(cutsection_x, SIGNAL(triggered()), this, SLOT(showCutsection()));
	connect(cutsection_y, SIGNAL(triggered()), this, SLOT(showCutsection()));
	connect(cutsection_z, SIGNAL(triggered()), this, SLOT(showCutsection()));

	cutsection_menu = getMainWindow()->getViewMenu()->
		addMenu(QIcon(":/icon/evds/cutsection.png"), tr("Show Cut&section"));
	cutsection_menu->setStatusTip("Show cutsection by an primary plane");
	cutsection_menu->addAction(cutsection_x);
	cutsection_menu->addAction(cutsection_y);
	cutsection_menu->addAction(cutsection_z);

	addAction(cutsection_menu->menuAction());
	addAction(window->getMainWindow()->getViewMenu()->addSeparator());

	//action = new QAction(tr("&Rocket Engine Designer..."), this);
	//action->setStatusTip("Define rocket engine using graphical UI dialog");
	//actions.append(action);
	//window->getMainWindow()->getViewMenu()->addAction(action);

	//actions.append(window->getMainWindow()->getViewMenu()->addSeparator());

	action = new QAction(QIcon(":/icon/evds/materials.png"), tr("&Materials Database..."), this);
	action->setStatusTip("Display list of materials and their physical properties");
	//connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	action->setEnabled(false);
	getMainWindow()->getViewMenu()->addAction(action);
	addAction(action);*/
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback from when object was modified
////////////////////////////////////////////////////////////////////////////////
void Editor::setModified(bool informationUpdate) {
	if (informationUpdate) updateInformation(false);
	getEditorWindow()->setModified();
	initializer->updateObject();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback when root object was initialized
////////////////////////////////////////////////////////////////////////////////
void Editor::rootInitialized() {
	getEditorWindow()->getEditRoot()->recursiveUpdateInformation(initializer);
	updateInformation(true);
	update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief 
////////////////////////////////////////////////////////////////////////////////
void Editor::commentChanged() {
	if (selected) {
		//selected->setVariable("comments",comments->toPlainText());
	} else {
		//document->setVariable("comments",comments->toPlainText());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief 
////////////////////////////////////////////////////////////////////////////////
void Editor::updateObject(Object* object) {
	if (object) {
		object_list->getModel()->updateObject(object);
	}
	glscene->update();
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::addObject() {
	QModelIndex index = object_list->currentIndex();
	object_list->getModel()->insertRow(0,index);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::removeObject() {
	QModelIndex index = object_list->currentIndex();
	object_list->getModel()->removeRow(index.row(),index.parent());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::selectObject(const QModelIndex& index) {
	//Should selection be cleared
	if (!index.isValid()) {
		object_properties->setPropertySheet(getEditorWindow()->getEditDocument()->getPropertySheet());
		csection_properties->setPropertySheet(0);
		selected = NULL;

		//Update information
		updateInformation(true);
		getEditorWindow()->updateObject(NULL);

		//Update comments
		//disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentChanged()));
		//comments->setText(document->getString("comments"));
		//connect(comments, SIGNAL(textChanged()), this, SLOT(commentChanged()));
		return;
	}

	//Check if selection must be cleared
	Object* object = (Object*)index.internalPointer();
	if (selected == object) {
		object_list->setCurrentIndex(QModelIndex());
		selectObject(QModelIndex());
		return;
	}
	selected = object;

	//Show proper properties sheet
	object_properties->setPropertySheet(object->getPropertySheet());

	//Show cross-sections editor
	csection_properties->setPropertySheet(object->getCrossSectionsEditor());

	//Update comments
	//disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentChanged()));
	//comments->setText(object->getString("comments"));
	//connect(comments, SIGNAL(textChanged()), this, SLOT(commentChanged()));

	//Update information
	updateInformation(true);

	//Redraw
	getEditorWindow()->updateObject(NULL);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::objectPropertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet) {
	object_properties->removePropertySheet(old_sheet);
	object_properties->setPropertySheet(new_sheet);
}

void Editor::csectionPropertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet) {
	csection_properties->removePropertySheet(old_sheet);
	csection_properties->setPropertySheet(new_sheet);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::finishInitializing() {
	initializer->updateObject();
	updateInformation(false);

	//Finish initialization
	modifiers_manager->setInitializing(false);
	modifiers_manager->updateModifiers();
}





/*

////////////////////////////////////////////////////////////////////////////////
/// @brief 
////////////////////////////////////////////////////////////////////////////////
void Editor::createInformationDock() {
	bodyinfo = new QTextEdit();
	//bodyinfo->setMinimumWidth(250);
	//bodyinfo->setMinimumHeight(80);
	//bodyinfo->setGeometry(0,0,250,80);
	bodyinfo->setReadOnly(true);
	bodyinfo->setObjectName("EVDS_BodyInformation");

	bodyinfo_dock = new QDockWidget(tr("Object Information"), this);
	bodyinfo_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	bodyinfo_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	bodyinfo_dock->setWidget(bodyinfo);
	//tabifyDockWidget(properties_dock,bodyinfo_dock);
	addDockWidget(Qt::RightDockWidgetArea,bodyinfo_dock);

	bodyinfo->setMaximumHeight(135);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief 
////////////////////////////////////////////////////////////////////////////////
void Editor::createCommentsDock() {
	comments = new QTextEdit();
	comments->setObjectName("EVDS_Comments");

	comments_dock = new QDockWidget(tr("Object Comments"), this);
	comments_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	comments_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	comments_dock->setWidget(comments);
	tabifyDockWidget(bodyinfo_dock,comments_dock);
	bodyinfo_dock->raise();
	//addDockWidget(Qt::RightDockWidgetArea,comments_dock);

	comments->setMaximumHeight(135);
}*/




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateInformation(bool ready) {
	/*if (ready) {
		Object* object = root_obj;
		if (selected) object = selected;

		QString information = "";
		QVector3D cm = object->getInformationVector("total_cm");
		if (!object->isInformationDefined("total_cm")) cm = object->getInformationVector("cm");
		information = information + tr("CoM: (%1; %2; %3) m\n")
			.arg(cm.x(),0,'F',3)
			.arg(cm.y(),0,'F',3)
			.arg(cm.z(),0,'F',3);

		information = information + tr("Mass: %2 kg (part: %1 kg)\n")
			.arg(object->getInformationVariable("mass"))
			.arg(object->getInformationVariable("total_mass"));

		if (!selected) {
			information = information + tr("Dimensions: %1 x %2 x %3 m\n")
				.arg(glscene->getCollection()->boundingBox().xLength(),0,'G',3)
				.arg(glscene->getCollection()->boundingBox().yLength(),0,'G',3)
				.arg(glscene->getCollection()->boundingBox().zLength(),0,'G',3);
		}

		if (object->getType() == "fuel_tank") {
			information = information + tr("\nFuel mass: %1 kg\n")
			.arg(object->getInformationVariable("fuel_mass"));
			information = information + tr("Fuel volume: %1 m\xB3\n")
			.arg(object->getInformationVariable("fuel_volume"));
		}
		if (object->getType() == "rocket_engine") {
			information = information + tr("\nVacuum parameters:\n");
			information = information + tr(
				"Isp: %1 sec\n"
				"Ve: %2 m/s\n"
				"Thrust: %3 kN\n"
				"Mass flow: %4 (O: %5, F: %6) kg/sec\n")
			.arg(object->getInformationVariable("vacuum.isp"),0,'G',3)
			.arg(object->getInformationVariable("vacuum.exhaust_velocity"),0,'F',0)
			.arg(object->getInformationVariable("vacuum.thrust")/1e3,0,'G',5)
			.arg(object->getInformationVariable("vacuum.mass_flow"),0,'G',3)
			.arg(object->getInformationVariable("vacuum.oxidizer_flow"),0,'G',3)
			.arg(object->getInformationVariable("vacuum.fuel_flow"),0,'G',3);

			information = information + tr("\n1 bar parameters:\n");
			information = information + tr(
				"Isp: %1 sec\n"
				"Ve: %2 m/s\n"
				"Thrust: %3 kN\n"
				"Mass flow: %4 (O: %5, F: %6) kg/sec\n")
			.arg(object->getInformationVariable("atmospheric.isp"),0,'G',3)
			.arg(object->getInformationVariable("atmospheric.exhaust_velocity"),0,'F',0)
			.arg(object->getInformationVariable("atmospheric.thrust")/1e3,0,'G',5)
			.arg(object->getInformationVariable("atmospheric.mass_flow"),0,'G',3)
			.arg(object->getInformationVariable("atmospheric.oxidizer_flow"),0,'G',3)
			.arg(object->getInformationVariable("atmospheric.fuel_flow"),0,'G',3);

			information = information + tr("\nCombustion:\n");
			information = information + tr(
				"O:F ratio: %1\n"
				"Temperature: %2 K\n"
				"Pressure: %3 Pa\n")
			.arg(object->getInformationVariable("combustion.of_ratio"),0,'G',3)
			.arg(object->getInformationVariable("combustion.temperature"),0,'G',3)
			.arg(object->getInformationVariable("combustion.pressure"),0,'G',3);
		}

		QVector3D ix = object->getInformationVector("total_ix");
		QVector3D iy = object->getInformationVector("total_iy");
		QVector3D iz = object->getInformationVector("total_iz");

		QVector3D jx = ix / (EVDS_EPS+object->getInformationVariable("total_mass"));
		QVector3D jy = iy / (EVDS_EPS+object->getInformationVariable("total_mass"));
		QVector3D jz = iz / (EVDS_EPS+object->getInformationVariable("total_mass"));
		information = information + tr(
			"\nRadius of gyration squared:\n"
			"(%1; %2; %3) m\xB2\n"
			"(%4; %5; %6) m\xB2\n"
			"(%7; %8; %9) m\xB2")
			.arg(jx.x(),0,'F',3).arg(jx.y(),0,'F',3).arg(jx.z(),0,'F',3)
			.arg(jy.x(),0,'F',3).arg(jy.y(),0,'F',3).arg(jy.z(),0,'F',3)
			.arg(jz.x(),0,'F',3).arg(jz.y(),0,'F',3).arg(jz.z(),0,'F',3);

		if (selected) {
			bodyinfo->setText(
				tr("Object: %1\n\n%2")
				.arg(object->getName())
				.arg(information)
			);
		} else {
			bodyinfo->setText(
				tr("Total summary: \n\n%1")
				.arg(information)
			);
		}
	} else {
		bodyinfo->setText("Hold on...");
	}*/
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::dropEvent(QDropEvent *event) {
	object_list->getModel()->dropMimeData(event->mimeData(),Qt::CopyAction,-1,-1,QModelIndex());
	//qDebug("Contents: %s", event->mimeData()->text().toLatin1().data());
}

void Editor::dragMoveEvent(QDragMoveEvent *event) {
	event->accept();
}

void Editor::dragEnterEvent(QDragEnterEvent *event) {
	event->acceptProposedAction();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::showProperties() {
	object_properties->show();
	object_properties->raise();
}
void Editor::showCrossSections() {
	csection_properties->show();
	csection_properties->raise();
}
void Editor::showHierarchy() {
	object_list->show();
	object_list->raise();
}
void Editor::showInformation() {
	//bodyinfo_dock->show();
	//bodyinfo_dock->raise();
}
void Editor::showComments() {
	//comments_dock->show();
	//comments_dock->raise();
}
void Editor::showCutsection() {
	QAction* action = dynamic_cast<QAction*>(sender());
	glscene->setCutsectionPlane(action->property("index").toInt(),action->isChecked());
	glscene->update();
}