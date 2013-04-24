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
#include <QStack>
#include <QXmlStreamReader>

#include <math.h>

#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_model.h"
#include "fwe_evds_glwidget.h"
#include "fwe_prop_sheet.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::Editor(MainWindow* window) : MdiChild(window)
{
	//Create EVDS system. Use flag that lists all children, even uninitialized ones to make sure
	// tree controls list all objects while they are messed around with.
	EVDS_System_Create(&system);
	EVDS_Common_Register(system);
	EVDS_Antenna_Register(system);
	EVDS_Common_LoadDatabase(system);

	//Create empty root object
	EVDS_Object_Create(system,0,&root);
	EVDS_Object_Initialize(root,1);
	root_obj = new Object(root,0,this);
   
	//Create dock panels
	createListDock();
	createPropertiesDock();
	createCSectionDock();

	//Create working area/main 3D widget
	QGLFormat format;
	format.setVersion(2,0);
	format.setSampleBuffers(true);

	glview = new GLWidget(root_obj,format,this);
	setCentralWidget(glview);
	selected = NULL;

	//Create informational docks
	createCutsectionDock();
	createBodyInfoDock();

	//Setup initial layout
	list_dock->raise();

	//Clear EVDS objects no longer used in other threads
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(cleanupTimer()));
	timer->start(1000);

	//Set MDI style
	setAttribute(Qt::WA_DeleteOnClose);

	//Load object types
	loadObjectData();


	//Edit menu structure
	QAction* action;
	//actions.append(getMainWindow()->getEditMenu()->addSeparator());

	//action = new QAction(QIcon(":/previous.png"), tr("Update Shaders"), this);
	//action->setShortcuts(QKeySequence::SelectAll);
	//connect(action, SIGNAL(triggered()), this, SLOT(updateShaders()));
	//actions.append(action);
	//getMainWindow()->getEditMenu()->addAction(action);

	//View menu structure
	action = new QAction(QIcon(":/evds/hierarchy.png"), tr("Object &Hierarchy"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showHierarchy()));
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);

	action = new QAction(tr("Object &Properties"), this); //QIcon(":/evds/properties.png"),
	connect(action, SIGNAL(triggered()), this, SLOT(showProperties()));
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);

	action = new QAction(tr("Edit &Cross Sections"), this); //QIcon(":/evds/csections.png"), 
	connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);

	actions.append(getMainWindow()->getViewMenu()->addSeparator());

	action = new QAction(tr("Show Cut&section..."), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showCutsection()));
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);

	actions.append(getMainWindow()->getViewMenu()->addSeparator());

	action = new QAction(tr("&Rigid Body Information..."), this);
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);

	action = new QAction(tr("&Rocket Engine Designer..."), this);
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);

	actions.append(getMainWindow()->getViewMenu()->addSeparator());

	action = new QAction(QIcon(":/evds/materials.png"), tr("&Materials Database..."), this);
	//connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	actions.append(action);
	getMainWindow()->getViewMenu()->addAction(action);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::~Editor()
{
	delete root_obj;
	EVDS_System_Destroy(system);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createListDock() {
	//Create object hierarchy window and dock
	list_dock = new QDockWidget(tr("Hierarchy"), this);
	list_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	list_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	list = new QWidget(list_dock);
	list_dock->setWidget(list);
	addDockWidget(Qt::LeftDockWidgetArea, list_dock);

	//Create remaining interface
	list_layout = new QVBoxLayout;
	list_tree = new QTreeView(this);
	list_model = new ObjectTreeModel(this,this);
	list_tree->setModel(list_model);
	list_tree->expandAll();
	list_tree->setColumnWidth(0,150);
	list_tree->setMinimumWidth(200);

	list_tree->viewport()->setAcceptDrops(true);
	list_tree->setDragDropMode(QAbstractItemView::DragDrop);
	list_tree->setDragEnabled(true);
	list_tree->setDropIndicatorShown(true);
	list_tree->setDragDropOverwriteMode(false);
	list_tree->setDefaultDropAction(Qt::MoveAction);

	list_add = new QPushButton(QIcon(":/add.png"),"Add object",this);
	list_remove = new QPushButton(QIcon(":/remove.png"),"Remove selected",this);
	connect(list_add, SIGNAL(released()), this, SLOT(addObject()));
	connect(list_remove, SIGNAL(released()), this, SLOT(removeObject()));
	connect(list_tree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectObject(const QModelIndex&)));

	list_layout->addWidget(list_add);
	list_layout->addWidget(list_tree);
	list_layout->addWidget(list_remove);
	list->setLayout(list_layout);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createPropertiesDock() {
	//Create properties interface
	properties_dock = new QDockWidget(tr("Properties"), this);
	properties_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	properties_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	properties = new QWidget(properties_dock);
	properties_dock->setWidget(properties);
	addDockWidget(Qt::LeftDockWidgetArea, properties_dock);

	properties_layout = new QStackedLayout;
	properties->setLayout(properties_layout);
	properties->setMinimumHeight(128);

	//properties_none = new QLabel(properties);
	//properties_none->setText("No selection");
	//properties_none->setPixmap(QPixmap(":/none.png"));
	//properties_none->setAlignment(Qt::AlignCenter);
	properties_document = new FWPropertySheet(this);
	properties_layout->addWidget(properties_document);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createCSectionDock() {
	//Create cross-sections editor interface
	csection_dock = new QDockWidget(tr("Cross sections"), this);
	csection_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	csection_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	//csection_dock->hide();

	csection = new QWidget(properties_dock);
	csection_dock->setWidget(csection);
	//addDockWidget(Qt::BottomDockWidgetArea, csection_dock);
	tabifyDockWidget(list_dock,csection_dock);

	csection_layout = new QStackedLayout;
	csection->setLayout(csection_layout);
	csection->setMinimumHeight(192);

	csection_none = new QLabel(csection);
	//csection_none->setText("Ain't no cross-sections to edit");
	//csection_none->setPixmap(QPixmap(":/none.png"));
	csection_none->setText("No object selected");
	csection_none->setAlignment(Qt::AlignCenter);
	csection_layout->addWidget(csection_none);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createCutsectionDock() {
	//Create cross-sections editor interface
	cutsection_dock = new QDockWidget(tr("Cutsection"), this);
	cutsection_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	cutsection_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	cutsection_dock->hide();

	cutsection = new QWidget(properties_dock);
	cutsection_dock->setWidget(cutsection);
	//tabifyDockWidget(properties_dock,cutsection_dock);
	addDockWidget(Qt::RightDockWidgetArea,cutsection_dock);

	QFormLayout* cutsection_layout = new QFormLayout;
	cutsection->setLayout(cutsection_layout);
	//cutsection_layout->setSpacing(0);

	cutsection_enabled = new QCheckBox("Enabled");
	cutsection_pitch = new QSlider(Qt::Horizontal);
	//cutsection_roll = new QSlider(Qt::Horizontal);
	cutsection_yaw = new QSlider(Qt::Horizontal);
	cutsection_offset = new QSlider(Qt::Horizontal);

	cutsection_pitch_label = new QLabel();
	cutsection_yaw_label = new QLabel();
	cutsection_offset_label = new QLabel();

	cutsection_pitch->setMinimum(0);
	cutsection_pitch->setMaximum(3600);
	cutsection_pitch->setTickPosition(QSlider::TicksBelow);
	cutsection_pitch->setTickInterval(100);
	cutsection_yaw->setMinimum(0);
	cutsection_yaw->setMaximum(3600);
	cutsection_yaw->setTickPosition(QSlider::TicksBelow);
	cutsection_yaw->setTickInterval(100);
	cutsection_offset->setMinimum(-10000);
	cutsection_offset->setMaximum( 10000);
	cutsection_offset->setTickPosition(QSlider::TicksBelow);
	cutsection_offset->setTickInterval(1000);

	connect(cutsection_enabled, SIGNAL(stateChanged(int)), this, SLOT(updateCutsection()));
	connect(cutsection_pitch, SIGNAL(valueChanged(int)), this, SLOT(updateCutsection()));
	connect(cutsection_yaw, SIGNAL(valueChanged(int)), this, SLOT(updateCutsection()));
	connect(cutsection_offset, SIGNAL(valueChanged(int)), this, SLOT(updateCutsection()));

	cutsection_layout->setRowWrapPolicy(QFormLayout::WrapAllRows);
	cutsection_layout->addRow(tr("Plane 1 parameters:"),cutsection_enabled);
	cutsection_layout->addRow(cutsection_pitch_label,cutsection_pitch);
	cutsection_layout->addRow(cutsection_yaw_label,cutsection_yaw);
	cutsection_layout->addRow(cutsection_offset_label,cutsection_offset);

	cutsection_layout->addRow(tr("Plane 2 parameters:"),cutsection_enabled);
	cutsection_layout->addRow(cutsection_pitch_label,cutsection_pitch);
	cutsection_layout->addRow(cutsection_yaw_label,cutsection_yaw);
	cutsection_layout->addRow(cutsection_offset_label,cutsection_offset);

	updateCutsection();
}

void Editor::updateCutsection() {
	double prad = EVDS_RAD(cutsection_pitch->value()/10.0);
	double qrad = EVDS_RAD(cutsection_yaw->value()/10.0);

	cutsection_pitch_label->setText(tr("Pitch angle (%1 deg):").arg(cutsection_pitch->value()/10.0));
	cutsection_yaw_label->setText(tr("Yaw angle (%1 deg):").arg(cutsection_yaw->value()/10.0));
	cutsection_offset_label->setText(tr("Offset (%1 m):").arg(cutsection_offset->value()/100.0));

	if (cutsection_enabled->isChecked()) {
		glview->setClipPlane(QVector4D(
			sin(prad)*cos(qrad),
			sin(qrad),
			cos(prad)*cos(qrad),
			cutsection_offset->value()/100.0));
		glview->update();
	} else {
		glview->setClipPlane(QVector4D(0,0,0,0));
		glview->update();
	}

	//QVector4D(0,1,0,1e7)
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createBodyInfoDock() {
	bodyinfo_dock = new QDockWidget(tr("Rigid Body Information"), this);
	bodyinfo_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	bodyinfo_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	//bodyinfo_dock->hide();

	bodyinfo = new QWidget(bodyinfo_dock);
	bodyinfo_dock->setWidget(bodyinfo);
	//tabifyDockWidget(properties_dock,bodyinfo_dock);
	addDockWidget(Qt::RightDockWidgetArea,bodyinfo_dock);

	QFormLayout* bodyinfo_layout = new QFormLayout;
	bodyinfo->setLayout(bodyinfo_layout);

	bodyinfo_mass = new QLabel();
	bodyinfo_total_mass = new QLabel();
	bodyinfo_cm = new QLabel();
	bodyinfo_total_cm = new QLabel();
	bodyinfo_inertia_tensor = new QLabel();
	bodyinfo_gyration_tensor = new QLabel();
	bodyinfo_f1 = new QLabel();
	bodyinfo_f2 = new QLabel();

	bodyinfo_mass->setProperty("outputValueBox",true);
	bodyinfo_total_mass->setProperty("outputValueBox",true);
	bodyinfo_cm->setProperty("outputValueBox",true);
	bodyinfo_total_cm->setProperty("outputValueBox",true);
	bodyinfo_inertia_tensor->setProperty("outputValueBox",true);
	bodyinfo_gyration_tensor->setProperty("outputValueBox",true);
	bodyinfo_f1->setProperty("outputValueBox",true);
	bodyinfo_f2->setProperty("outputValueBox",true);

	bodyinfo_layout->addRow(new QLabel("Total for object:"));
	bodyinfo_layout->addRow("Total mass:",bodyinfo_total_mass);
	bodyinfo_layout->addRow("Total center of mass:",bodyinfo_total_cm);
	bodyinfo_layout->addRow("Inertia tensor:",bodyinfo_inertia_tensor);
	bodyinfo_layout->addRow(new QLabel(""));
	bodyinfo_layout->addRow(new QLabel("Only selected object:"));
	bodyinfo_layout->addRow("Body Mass:",bodyinfo_mass);
	bodyinfo_layout->addRow("Center of mass:",bodyinfo_cm);
	bodyinfo_layout->addRow("Radius of gyration:",bodyinfo_gyration_tensor);
	bodyinfo_layout->setRowWrapPolicy(QFormLayout::WrapAllRows);

	bodyinfo_layout->addRow("Fuel mass:",bodyinfo_f1);
	bodyinfo_layout->addRow("Fuel volume:",bodyinfo_f2);

	//bodyinfo_layout->setSpacing(2);
	updateBodyInformation(NULL);
}

void Editor::updateObjectInformation(Object* object) {
	if (selected == object) {
		updateBodyInformation(object);
	}
}

void Editor::updateBodyInformation(Object* object) {
	if (!object) {
		bodyinfo_cm->setText("...");
		bodyinfo_total_cm->setText("...");
		bodyinfo_mass->setText("...");
		bodyinfo_total_mass->setText("...");
		bodyinfo_inertia_tensor->setText("(...)<br>(...)<br>(...)");
		bodyinfo_gyration_tensor->setText("(...)<br>(...)<br>(...)");
		return;
	}

	Object* iobject = object->getInformationObject();
	if (iobject) {
		QVector3D cm = iobject->getVector("cm");
		bodyinfo_cm->setText(tr("(%1; %2; %3) m")
			.arg(cm.x(),0,'F',3)
			.arg(cm.y(),0,'F',3)
			.arg(cm.z(),0,'F',3));

		bodyinfo_mass->setText(tr("%1 kg").arg(iobject->getVariable("mass")));

		QVector3D jx = iobject->getVector("jx");
		QVector3D jy = iobject->getVector("jy");
		QVector3D jz = iobject->getVector("jz");
		bodyinfo_gyration_tensor->setText(tr(
			"(%1; %2; %3) m\xB2<br>"
			"(%4; %5; %6) m\xB2<br>"
			"(%7; %8; %9) m\xB2")
			.arg(jx.x(),0,'F',3).arg(jx.y(),0,'F',3).arg(jx.z(),0,'F',3)
			.arg(jy.x(),0,'F',3).arg(jy.y(),0,'F',3).arg(jy.z(),0,'F',3)
			.arg(jz.x(),0,'F',3).arg(jz.y(),0,'F',3).arg(jz.z(),0,'F',3));


		QVector3D total_cm = iobject->getVector("total_cm");
		bodyinfo_total_cm->setText(tr("(%1; %2; %3) m")
			.arg(total_cm.x(),0,'F',3)
			.arg(total_cm.y(),0,'F',3)
			.arg(total_cm.z(),0,'F',3));

		bodyinfo_total_mass->setText(tr("%1 kg").arg(iobject->getVariable("total_mass")));

		QVector3D ix = iobject->getVector("total_ix");
		QVector3D iy = iobject->getVector("total_iy");
		QVector3D iz = iobject->getVector("total_iz");
		bodyinfo_inertia_tensor->setText(tr(
			"(%1; %2; %3) kg m\xB2<br>"
			"(%4; %5; %6) kg m\xB2<br>"
			"(%7; %8; %9) kg m\xB2")
			.arg(ix.x(),0,'G',3).arg(ix.y(),0,'G',3).arg(ix.z(),0,'G',3)
			.arg(iy.x(),0,'G',3).arg(iy.y(),0,'G',3).arg(iy.z(),0,'G',3)
			.arg(iz.x(),0,'G',3).arg(iz.y(),0,'G',3).arg(iz.z(),0,'G',3));


		bodyinfo_f1->setText(tr("%1 kg").arg(iobject->getVariable("fuel_mass")));
		bodyinfo_f2->setText(tr("%1 m\xB3").arg(iobject->getVariable("fuel_volume")));
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::addObject() {
	QModelIndex index = list_tree->selectionModel()->currentIndex();
	static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->insertRow(0,index);
	//static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->appendRow(index);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::removeObject() {
	QModelIndex index = list_tree->selectionModel()->currentIndex();
	static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->removeRow(index.row(),index.parent());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::selectObject(const QModelIndex& index) {
	//Should selection be cleared
	if (!index.isValid()) {
		properties_layout->setCurrentWidget(properties_document);
		csection_layout->setCurrentWidget(csection_none);
		selected = NULL;
		return;
	}

	//Check if selection must be cleared
	Object* object = (Object*)index.internalPointer();
	if (selected == object) {
		list_tree->setCurrentIndex(QModelIndex());
		selectObject(QModelIndex());
		updateObject(NULL);
		return;
	}
	selected = object;

	//Show proper properties sheet
	QWidget* property_sheet = object->getPropertySheet();
	if (properties_layout->indexOf(property_sheet) < 0) {
		properties_layout->addWidget(property_sheet);
	}
	properties_layout->setCurrentWidget(property_sheet);

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

	//Show information
	object->updateInformationObject();

	//Redraw
	updateObject(NULL);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateObject(Object* object) {
	if (object) {
		list_model->updateObject(object);
	}
	glview->update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet) {
	bool resetCurrent = true; //FIXME
	//if (properties_layout->widget() == old_sheet) resetCurrent = true;
	properties_layout->removeWidget(old_sheet);
	properties_layout->addWidget(new_sheet);
	if (resetCurrent) {	
		properties_layout->setCurrentWidget(new_sheet);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::loadObjectData() {
	//Create special mapping from materials databases
	QMap<QString,QString> special_mapping;
	
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
	QFile xml_file("./evds.xml");
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
			/*if (currentTag.value(currentTag.count()-2) == "object_types") {
				ObjectType* data = new ObjectType();
				data->evds_type = currentName.top();
				data->print_name = xml.text().toString();
				objectTypes.append(data);
			}*/
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
			//printf("[%s] %s = %s\n",currentTag.top().toAscii().data(),currentName.top().toAscii().data(),xml.text().toAscii().data());
		}
	}

}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::cleanupTimer() {
	EVDS_System_CleanupObjects(system);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::newFile() {
	//Setup window
	currentFile = "";
	isModified = false;
	updateTitle();

	//Load vessel FIXME
	//EVDS_OBJECT* satellite;
	//EVDS_Object_LoadEx(root,"atest.evds",0);
	root_obj->invalidateChildren();

	//EVDS_OBJECT_SAVEEX str = { 0 };
	//str.saveChildren = 1;
	//EVDS_Object_SaveEx(root,"export.evds",&str);

	//EVDS::BaseObject* base = nameToObjectFactory["vessel"]->newObject(satellite,properties);
	//base->initializeProperties();
	//properties_layout->addWidget(base->getBrowser());
	//properties_layout->setCurrentWidget(base->getBrowser());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateTitle() {
	if (currentFile == "") {
		if (isModified) {
			setWindowTitle("New vessel*");
		} else {
			setWindowTitle("New vessel");
		}
	} else {
		if (isModified) {
			setWindowTitle(QFileInfo(currentFile).fileName() + "*");
		} else {
			setWindowTitle(QFileInfo(currentFile).fileName());
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
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

	//Reset current file
	currentFile = fileName;
	isModified = false;

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
	root_obj->invalidateChildren();
	QApplication::restoreOverrideCursor();

	//Setup window
	updateTitle();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool Editor::save() {
	if (currentFile == "") {
		return saveAs();
	} else {
		return saveFile(currentFile);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool Editor::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save As", currentFile,
		"External Vessel Dynamics Simulator (*.evds);;All files (*.*)");

	if (fileName.isEmpty())	return false;
	return saveFile(fileName);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool Editor::saveFile(const QString &fileName) {
	EVDS_OBJECT_SAVEEX info = { 0 };
	info.flags = EVDS_OBJECT_SAVEEX_ONLY_CHILDREN;
	EVDS_Object_SaveEx(root,fileName.toUtf8().data(),&info);

	isModified = false;
	currentFile = fileName;
	updateTitle();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::closeEvent(QCloseEvent *event) {
	if (trySave()) {
		event->accept();
	} else {
		event->ignore();
	}
	//close();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool Editor::trySave() {
	if (isModified) {
		QMessageBox::StandardButton ret;
		QString fileName = QFileInfo(currentFile).fileName();
		if (currentFile == "") fileName = "New vessel";

		ret = QMessageBox::warning(this, tr("EVDS Editor"),
					 tr("'%1' has been modified.\n"
						"Do you want to save pending changes?")
						.arg(fileName),
					 QMessageBox::Save | QMessageBox::Discard
			 | QMessageBox::Cancel);
		if (ret == QMessageBox::Save)
			return save();
		else if (ret == QMessageBox::Cancel)
			return false;
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateShaders() {
	glview->reloadShaders();
	glview->update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateMenus(bool isInFront) {
	for (int i = 0; i < actions.count(); i++) {
		actions[i]->setVisible(isInFront);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::showProperties() {
	properties_dock->show();
	properties_dock->show();
}

void Editor::showCrossSections() {
	csection_dock->show();
	csection_dock->raise();
}

void Editor::showHierarchy() {
	list_dock->show();
	list_dock->raise();
}

void Editor::showCutsection() {
	cutsection_dock->show();
	cutsection_dock->raise();
}

void Editor::showBodyInformation() {
	bodyinfo_dock->show();
	bodyinfo_dock->raise();
}