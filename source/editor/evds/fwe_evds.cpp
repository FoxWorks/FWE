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
Editor::Editor(FWE::EditorWindow* in_window) : FWE::Editor(in_window), activeThreads(0) {
	window = in_window;
	selected = NULL;
	document = NULL;

	//Create EVDS system. Use flag that lists all children, even uninitialized ones to make sure
	// tree controls list all objects while they are messed around with.
	EVDS_System_Create(&system);
	EVDS_Common_Register(system);
	EVDS_Antenna_Register(system);
	EVDS_Train_WheelsGeometry_Register(system);

	//Clear EVDS objects no longer used in other threads
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(cleanupTimer()));
	timer->start(1000);

	//Load object types
	loadObjectVariablesData();


	//Create empty root object
	EVDS_OBJECT* inertial_root;
	EVDS_System_GetRootInertialSpace(system,&inertial_root);
	EVDS_Object_Create(inertial_root,&root);
	EVDS_Object_SetType(root,"rigid_body"); //Allows finding out parameters for the entire file
	//EVDS_Object_Initialize(root,1);
	root_obj = new Object(root,0,this,0);

	//Create initializer thread
	initializer = new ObjectInitializer(root_obj);
	connect(initializer, SIGNAL(signalObjectReady()), this, SLOT(rootInitialized()), Qt::QueuedConnection);
	initializer->start();
	initializer->updateObject(); //Must be called before first call to getObject
   

	//Create parts of main UI
	createMenuToolbar();

	//Create list of objects
	object_list = new Dock::ObjectList(root_obj,this);
	connect(object_list, SIGNAL(addObject()), this, SLOT(addObject()));
	connect(object_list, SIGNAL(removeObject()), this, SLOT(removeObject()));
	connect(object_list, SIGNAL(selectObject(const QModelIndex&)), this, SLOT(selectObject(const QModelIndex&)));
	addDockWidget(Qt::LeftDockWidgetArea, object_list);

	//Create properties sheet for objects
	object_properties = new Dock::Properties(this);
	addDockWidget(Qt::LeftDockWidgetArea, object_properties);

	//Create properties sheet for cross-sections
	createCSectionDock();

	//Create working area/main 3D widget
	glview = new GLView(this);
	glscene = new GLScene(0,this,0,this);
	glview->setScene(glscene);
	setCentralWidget(glview);
	modifiers_manager = new ObjectModifiersManager(this);

	//Create informational docks
	createInformationDock();
	createCommentsDock();

	//Setup initial layout
	object_list->raise();

	//Enable drag and drop
	setAcceptDrops(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::~Editor() {
	modifiers_manager->setInitializing(true); //Prevent updates of modifiers list

	initializer->stopWork();
	delete initializer;

	qDebug("Editor::~Editor: cleaning up EVDS objects");
	delete root_obj;
	qDebug("Editor::~Editor: waiting for remaining threads");
	while (activeThreads.available()) ;

	//Destroy EVDS system
	qDebug("Editor::~Editor: destroying system");
	EVDS_System_Destroy(system);

	cutsection_menu->deleteLater();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createMenuToolbar() {
	QAction* action;

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
	addAction(action);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createCSectionDock() {
	csection = new QWidget();
	csection->setMinimumWidth(250);
	csection->setMinimumHeight(80);

	//Create cross-sections editor interface
	csection_dock = new QDockWidget(tr("Cross Sections Editor"), this);
	csection_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	csection_dock->setAllowedAreas(Qt::AllDockWidgetAreas);	
	csection_dock->setWidget(csection);
	addDockWidget(Qt::RightDockWidgetArea, csection_dock);
	//csection_dock->hide();
	//tabifyDockWidget(list_dock,csection_dock);

	//No cross-sections
	csection_none = new QLabel(csection);
	//csection_none->setText("Ain't no cross-sections to edit");
	//csection_none->setPixmap(QPixmap(":/none.png"));
	csection_none->setText("No object selected");
	csection_none->setAlignment(Qt::AlignCenter);

	//Create layout
	csection->setLayout(new QVBoxLayout);
	csection->layout()->setSpacing(0);
	csection->layout()->setMargin(0);

	//QLabel* geometry_editor_test = new QLabel(csection);
	//geometry_editor_test->setText("Cross-section geometry editor here");

	QWidget* csection_properties = new QWidget(csection);
	csection->layout()->addWidget(csection_properties);
	//csection->layout()->addWidget(geometry_editor_test);

	csection_layout = new QStackedLayout;
	csection_layout->setSpacing(0);
	csection_layout->setMargin(0);
	csection_layout->addWidget(csection_none);
	csection_properties->setLayout(csection_layout);
}


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
}

void Editor::commentsChanged() {
	if (selected) {
		selected->setVariable("comments",comments->toPlainText());
	} else {
		document->setVariable("comments",comments->toPlainText());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::showCutsection() {
	QAction* action = dynamic_cast<QAction*>(sender());
	glscene->setCutsectionPlane(action->property("index").toInt(),action->isChecked());
	glscene->update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback when root object was initialized
////////////////////////////////////////////////////////////////////////////////
void Editor::rootInitialized() {
	root_obj->recursiveUpdateInformation(initializer);
	updateInformation(true);
	update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback from when object was modified
////////////////////////////////////////////////////////////////////////////////
void Editor::setModified(bool informationUpdate) {
	window->setModified();
	initializer->updateObject();
	if (informationUpdate) updateInformation(false);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateInformation(bool ready) {
	//qDebug("Editor::updateInformation: is ready: %s",(ready ? "true" : "false"));

	/*if (selected && ready) {
		TemporaryObject temp_object = initializer->getObject(selected);
		QVector3D cm = temp_object.getVector("cm");
		QString test = tr("(%1; %2; %3) m")
			.arg(cm.x(),0,'F',3)
			.arg(cm.y(),0,'F',3)
			.arg(cm.z(),0,'F',3);

		qDebug("Editor::updateInformation: selected: %s",test.toAscii().data());
	}*/

	if (ready) {
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
		/*information = information + tr(
			"\nInertia tensor:\n"
			"(%1; %2; %3) kg m\xB2\n"
			"(%4; %5; %6) kg m\xB2\n"
			"(%7; %8; %9) kg m\xB2\n")
			.arg(ix.x(),0,'G',3).arg(ix.y(),0,'G',3).arg(ix.z(),0,'G',3)
			.arg(iy.x(),0,'G',3).arg(iy.y(),0,'G',3).arg(iy.z(),0,'G',3)
			.arg(iz.x(),0,'G',3).arg(iz.y(),0,'G',3).arg(iz.z(),0,'G',3);*/

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
	}
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
		object_properties->setPropertySheet(document->getPropertySheet());
		csection_layout->setCurrentWidget(csection_none);
		selected = NULL;

		//Update information
		updateInformation(true);
		updateObject(NULL);

		//Update comments
		disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
		comments->setText(document->getString("comments"));
		connect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
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
	QWidget* csection_editor = object->getCrossSectionsEditor();
	if (csection_editor) {
		if (csection_layout->indexOf(csection_editor) < 0) {
			csection_layout->addWidget(csection_editor);
		}
		csection_layout->setCurrentWidget(csection_editor);
	} else {
		csection_layout->setCurrentWidget(csection_none);
	}

	//Update comments
	disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
	comments->setText(object->getString("comments"));
	connect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));

	//Update information
	updateInformation(true);

	//Redraw
	updateObject(NULL);
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
void Editor::propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet) {
	object_properties->removePropertySheet(old_sheet);
	object_properties->setPropertySheet(new_sheet);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::loadObjectVariablesData() {
	//Create special mapping from materials databases
	QMap<QString,QString> special_mapping;
	
	//Parse out list of materials by class
	EVDS_VARIABLE* database;
	if (EVDS_System_GetDatabaseByName(system,"material",&database) == EVDS_OK) {
		SIMC_LIST* entries;
		EVDS_Variable_GetList(database,&entries);

		SIMC_LIST_ENTRY* entry = SIMC_List_GetFirst(entries);
		while (entry) {
			char name[256] = { 0 };
			char print_name[256] = { 0 };
			char class_str[256] = { 0 };
			EVDS_VARIABLE* attribute;
			EVDS_VARIABLE* material = (EVDS_VARIABLE*)SIMC_List_GetData(entries,entry);

			if (EVDS_Variable_GetAttribute(material,"name",&attribute) == EVDS_OK) {
				EVDS_Variable_GetString(attribute,name,255,0);
			}
			if (EVDS_Variable_GetAttribute(material,"print",&attribute) == EVDS_OK) {
				EVDS_Variable_GetString(attribute,print_name,255,0);
			}
			if (!print_name[0]) strcpy(print_name,name);

			//Add to list
			special_mapping["@materials._"] += QString(name) + "\n" + QString(print_name) + "\n";
			if (EVDS_Variable_GetAttribute(material,"class",&attribute) == EVDS_OK) {
				EVDS_Variable_GetString(attribute,class_str,255,0);
				if (class_str[0]) {
					special_mapping["@materials."+QString(class_str)] += QString(name) + "\n" + QString(print_name) + "\n";
				} else {
					EVDS_BREAKPOINT();
				}
			}
			entry = SIMC_List_GetNext(entries,entry);
		}
	}

	//Load information about editable objects and properties
	QFile xml_file(":/evds.xml");
	if (!xml_file.open(QIODevice::ReadOnly)) {
		return;
	}

	QStack<QString> currentTag;
	QStack<QString> currentName;
	int variable_index = 0;

	QXmlStreamReader xml(&xml_file);
	while (!xml.atEnd()) {
		xml.readNext();
		if (xml.isStartElement()) {
			currentTag.push(xml.name().toString());
			currentName.push(xml.attributes().value("name").toString());
			//printf("[s] %s\n",xml.name().toAscii().data());
			if (currentTag.top() == "object_vars") {
				variable_index = 0;
			}
			if (currentTag.top() == "csection_vars") {
				variable_index = 0;
			}
		} else if (xml.isEndElement()) {
			if (currentTag.top() == "var") {
				variable_index++;
			}

			currentTag.pop();
			currentName.pop();
			//printf("[e] %s\n",xml.name().toAscii().data());
		} else if (xml.isCharacters() && !xml.isWhitespace()) {
			if (currentTag.value(currentTag.count()-3) == "object_vars") {
				QString objectType = currentName.value(currentName.count()-3);
				if (objectVariables[objectType].count() <= variable_index) {
					objectVariables[objectType].append(QMap<QString,QString>());
				}
				QString value = xml.text().toString();
				QMapIterator<QString, QString> i(special_mapping);
				while (i.hasNext()) {
					i.next();
					value.replace(i.key(),i.value());
				}

				objectVariables[objectType][variable_index][currentTag.top()] = value;
			}
			if (currentTag.value(currentTag.count()-3) == "csection_vars") {
				QString csectionType = currentName.value(currentName.count()-3);
				if (csectionVariables[csectionType].count() <= variable_index) {
					csectionVariables[csectionType].append(QMap<QString,QString>());
				}
				QString value = xml.text().toString();
				QMapIterator<QString, QString> i(special_mapping);
				while (i.hasNext()) {
					i.next();
					value.replace(i.key(),i.value());
				}

				csectionVariables[csectionType][variable_index][currentTag.top()] = value;
			}
		}
	}

}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::cleanupTimer() {
	EVDS_System_CleanupObjects(system);

	//Small hack for bodyinfo dock
	bodyinfo->setMaximumHeight(csection->maximumHeight());
	comments->setMaximumHeight(csection->maximumHeight());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::newFile() {
	//EVDS_OBJECT_LOADEX info = { 0 };
	//EVDS_Object_LoadEx(root,"test.evds",&info);
	//EVDS_Object_LoadEx(root,"RV-505_proper.evds",&info);
	modifiers_manager->setInitializing(false);
		root_obj->invalidateChildren();
		initializer->updateObject();
		updateInformation(false);
	modifiers_manager->setInitializing(true);
	modifiers_manager->updateModifiers();

	document = root_obj->appendHiddenChild();
	document->setType("metadata");
	document->setName("");
	selectObject(QModelIndex());
}

////////////////////////////////////////////////////////////////////////////////
/// @brief This is only relevant to pre-alpha-4 saves. Delete no later than year 2040
////////////////////////////////////////////////////////////////////////////////
void FWE_LoadFile_FixRxRyBug(EVDS_OBJECT* evds_object) {
	SIMC_LIST* list;
	SIMC_LIST_ENTRY* entry;

	//Search for geometry
	EVDS_VARIABLE* variable;
	if (EVDS_Object_GetVariable(evds_object,"geometry.cross_sections",&variable) == EVDS_OK) {
		EVDS_Variable_GetList(variable,&list);

		//Count number of entries
		entry = SIMC_List_GetFirst(list);
		while (entry) {
			EVDS_VARIABLE* ry;
			EVDS_VARIABLE* csection = (EVDS_VARIABLE*)SIMC_List_GetData(list,entry);
			if (EVDS_Variable_GetAttribute(csection,"ry",&ry) != EVDS_OK) {
				EVDS_VARIABLE* rx;
				EVDS_REAL value;
				EVDS_Variable_AddAttribute(csection,"ry",EVDS_VARIABLE_TYPE_FLOAT,&ry);
				EVDS_Variable_AddAttribute(csection,"rx",EVDS_VARIABLE_TYPE_FLOAT,&rx);
				EVDS_Variable_GetReal(rx,&value);
				EVDS_Variable_SetReal(ry,value);
			}

			entry = SIMC_List_GetNext(list,entry);
		}
	}

	//Call for every child
	EVDS_Object_GetAllChildren(evds_object,&list);
	entry = SIMC_List_GetFirst(list);
	while (entry) {
		FWE_LoadFile_FixRxRyBug((EVDS_OBJECT*)SIMC_List_GetData(list,entry));
		entry = SIMC_List_GetNext(list,entry);
	}
}


int FWE_LoadFile_OnSyntaxError(EVDS_OBJECT_LOADEX* info, const char* error) {
	Editor* editor = (Editor*)info->userdata;
	editor->loadError(QString(error));
	return EVDS_OK;
}

void Editor::loadError(const QString& error) {
	QApplication::restoreOverrideCursor();
	QMessageBox::warning(this, tr("EVDS Editor"),
						 tr("Cannot read file %1 (syntax error):\n%2.")
						 .arg(QFileInfo(currentFile).fileName())
						 .arg(error));
}

bool Editor::loadFile(const QString &fileName) {
	EVDS_OBJECT_LOADEX info = { 0 };
	info.OnSyntaxError = &FWE_LoadFile_OnSyntaxError;
	info.userdata = (void*)this;

	//Reset current file (hack for error messages)
	currentFile = fileName;

	//Load file
	QApplication::setOverrideCursor(Qt::WaitCursor);
	int error_code = EVDS_Object_LoadEx(root,fileName.toUtf8().data(),&info);
	if (error_code != EVDS_OK) {
		if (error_code != EVDS_ERROR_SYNTAX) {
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, tr("EVDS Editor"),
								 tr("Cannot read file %1:\nFile not found or not readable.")
								 .arg(fileName));
		}
		return false;

	}

	//Run old version conversions
	if (info.version == 0) {
		FWE_LoadFile_FixRxRyBug(root);
	}

	//Initialize everything
	modifiers_manager->setInitializing(true);
		root_obj->invalidateChildren();
		initializer->updateObject();
		updateInformation(false);
	modifiers_manager->setInitializing(false);
	modifiers_manager->updateModifiers();

	//Find the "document" object, or create it
	document = 0;
	for (int i = 0; i < root_obj->getChildrenCount(); i++) {
		if (root_obj->getChild(i)->getType() == "metadata") {
			document = root_obj->getChild(i);
			root_obj->hideChild(i);
		}
	}
	if (!document) {
		document = root_obj->appendHiddenChild();
		document->setType("metadata");
		document->setName("");
		//static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->insertRow(QModelIndex());
	}
	selectObject(QModelIndex());

	//Return control
	QApplication::restoreOverrideCursor();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWE_SaveFile_RemoveRedundantVariables(Object* object) {
	//FIXME: this code is good, but it deletes cross-sections editor right from under users hand
	/*EVDS_OBJECT* evds_object = object->getEVDSObject();

	//Search for geometry
	EVDS_VARIABLE* variable;
	if (EVDS_Object_GetVariable(evds_object,"geometry.cross_sections",&variable) == EVDS_OK) {
		SIMC_LIST* list;
		SIMC_LIST_ENTRY* entry;
		EVDS_Variable_GetList(variable,&list);

		//Count number of entries
		int count = 0;
		entry = SIMC_List_GetFirst(list);
		while (entry) {
			count++;
			entry = SIMC_List_GetNext(list,entry);
		}

		//Remove if only one cross-section present
		if (count <= 1) {
			EVDS_Variable_Destroy(variable);
			object->deleteCrossSectionsEditor();
		}
	}

	//Call for every child
	for (int i = 0; i < object->getChildrenCount(); i++) {
		FWE_SaveFile_RemoveRedundantVariables(object->getChild(i));
	}*/
}

bool Editor::saveFile(const QString &fileName) {
	//Filter out redundant variables
	FWE_SaveFile_RemoveRedundantVariables(root_obj);

	//Save the file itself
	EVDS_OBJECT_SAVEEX info = { 0 };
	info.flags = EVDS_OBJECT_SAVEEX_ONLY_CHILDREN;
	EVDS_Object_SaveEx(root,fileName.toUtf8().data(),&info);
	return true;
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
	csection_dock->show();
	csection_dock->raise();
}
void Editor::showHierarchy() {
	object_list->show();
	object_list->raise();
}
/*void Editor::showCutsection() {
	cutsection_dock->show();
	cutsection_dock->raise();
}*/
void Editor::showInformation() {
	bodyinfo_dock->show();
	bodyinfo_dock->raise();
}

void Editor::showComments() {
	comments_dock->show();
	comments_dock->raise();
}