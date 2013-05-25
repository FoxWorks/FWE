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
#include <QTextEdit>
#include <QStack>
#include <QXmlStreamReader>

#include <GLC_3DViewCollection>
#include <GLC_3DViewInstance>
#include <GLC_Context>
#include <QGLWidget>

#include <math.h>

#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_model.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_glscene.h"
#include "fwe_prop_sheet.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::Editor(ChildWindow* in_window) : QMainWindow(in_window) {
	window = in_window;
	selected = NULL;

	//Create EVDS system. Use flag that lists all children, even uninitialized ones to make sure
	// tree controls list all objects while they are messed around with.
	EVDS_System_Create(&system);
	EVDS_System_Create(&initialized_system);
	EVDS_Common_Register(system);
	EVDS_Common_Register(initialized_system);
	EVDS_Common_LoadDatabase(system);
	EVDS_Common_LoadDatabase(initialized_system);
	//EVDS_Antenna_Register(system);
	//EVDS_Antenna_Register(initialized_system);

	//Load object types
	loadObjectData();

	//Clear EVDS objects no longer used in other threads
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(cleanupTimer()));
	timer->start(1000);

	//Create empty root object
	EVDS_Object_Create(system,0,&root);
	//EVDS_Object_Initialize(root,1);
	root_obj = new Object(root,0,this);

	//Create initializer thread
	initializer = new ObjectInitializer(root_obj);
	connect(initializer, SIGNAL(signalObjectReady()), this, SLOT(rootInitialized()), Qt::QueuedConnection);
	initializer->start();
	initializer->updateObject(); //Must be called before first call to getObject
   

	//Create parts of main UI
	createMenuToolbar();
	createListDock();
	createPropertiesDock();
	createCSectionDock();

	//Create working area/main 3D widget
	glscene = new GLScene(0,this,this);
	glview = new GLView(this);
	glview->setScene(glscene);
	setCentralWidget(glview);

	//Create informational docks
	//createCutsectionDock();
	createInformationDock();

	//Setup initial layout
	list_dock->raise();


	//Set MDI style, enable drag and drop
	setAttribute(Qt::WA_DeleteOnClose);
	setAcceptDrops(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Editor::~Editor() {
	delete root_obj;
	EVDS_System_Destroy(system); //FIXME: must be done after all threads shut down, otherwise potential crash
	EVDS_System_Destroy(initialized_system);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createMenuToolbar() {
	//View menu structure
	QAction* action;
	action = new QAction(QIcon(":/icon/evds/hierarchy.png"), tr("Objects &Hierarchy"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showHierarchy()));
	actions.append(action);
	window->getMainWindow()->getViewMenu()->addAction(action);

	action = new QAction(QIcon(":/icon/evds/properties.png"), tr("Object &Properties"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showProperties()));
	actions.append(action);
	window->getMainWindow()->getViewMenu()->addAction(action);

	action = new QAction(QIcon(":/icon/evds/csections.png"), tr("&Cross Sections Editor"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	actions.append(action);
	window->getMainWindow()->getViewMenu()->addAction(action);

	action = new QAction(tr("Body &Information"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showInformation()));
	actions.append(action);
	window->getMainWindow()->getViewMenu()->addAction(action);

	actions.append(window->getMainWindow()->getViewMenu()->addSeparator());

	//action = new QAction(tr("Show Cut&section..."), this);
	//connect(action, SIGNAL(triggered()), this, SLOT(showCutsection()));
	//actions.append(action);
	//window->getMainWindow()->getViewMenu()->addAction(action);

	//actions.append(window->getMainWindow()->getViewMenu()->addSeparator());

	//action = new QAction(tr("&Rocket Engine Designer..."), this);
	//actions.append(action);
	//window->getMainWindow()->getViewMenu()->addAction(action);

	//actions.append(window->getMainWindow()->getViewMenu()->addSeparator());

	action = new QAction(QIcon(":/icon/evds/materials.png"), tr("&Materials Database..."), this);
	//connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	action->setEnabled(false);
	actions.append(action);
	window->getMainWindow()->getViewMenu()->addAction(action);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createListDock() {
	list = new QWidget();
	list->setMinimumWidth(200);
	list->setMinimumHeight(80);

	//Create object hierarchy window and dock
	list_dock = new QDockWidget(tr("Objects Hierarchy"), this);
	list_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	list_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	list_dock->setWidget(list);
	addDockWidget(Qt::LeftDockWidgetArea, list_dock);

	//Create remaining interface
	list_model = new ObjectTreeModel(this,this);
	list_tree = new QTreeView(this);
	list_tree->setModel(list_model);
	list_tree->expandAll();
	list_tree->setColumnWidth(0,150);

	list_tree->viewport()->setAcceptDrops(true);
	list_tree->setDragDropMode(QAbstractItemView::DragDrop);
	list_tree->setDragEnabled(true);
	list_tree->setDropIndicatorShown(true);
	list_tree->setDragDropOverwriteMode(false);
	list_tree->setDefaultDropAction(Qt::MoveAction);

	list_add = new QPushButton(QIcon(":/icon/add.png"),"Add object",this);
	list_remove = new QPushButton(QIcon(":/icon/remove.png"),"Remove selected",this);
	connect(list_add, SIGNAL(released()), this, SLOT(addObject()));
	connect(list_remove, SIGNAL(released()), this, SLOT(removeObject()));
	connect(list_tree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectObject(const QModelIndex&)));

	//Create layout
	list_layout = new QVBoxLayout;
	list_layout->addWidget(list_add);
	list_layout->addWidget(list_tree);
	list_layout->addWidget(list_remove);
	list->setLayout(list_layout);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::createPropertiesDock() {
	properties = new QWidget();
	properties->setMinimumWidth(200);
	properties->setMinimumHeight(80);

	//Create properties interface
	properties_dock = new QDockWidget(tr("Object Properties"), this);
	properties_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	properties_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	properties_dock->setWidget(properties);
	addDockWidget(Qt::LeftDockWidgetArea, properties_dock);

	//Create document properties
	properties_document = new FWEPropertySheet(this);

	//Create layout
	properties_layout = new QStackedLayout;
	properties_layout->addWidget(properties_document);
	properties->setLayout(properties_layout);
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
	bodyinfo->setReadOnly(true);
	bodyinfo->setObjectName("EVDS_BodyInformation");

	bodyinfo_dock = new QDockWidget(tr("Object Information"), this);
	bodyinfo_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	bodyinfo_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	bodyinfo_dock->setWidget(bodyinfo);
	//tabifyDockWidget(properties_dock,bodyinfo_dock);
	addDockWidget(Qt::RightDockWidgetArea,bodyinfo_dock);
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
void Editor::setModified() {
	window->setModified();
	initializer->updateObject();
	updateInformation(false);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::updateInformation(bool ready) {
	qDebug("Editor::updateInformation: is ready: %s",(ready ? "true" : "false"));

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
		if (selected) {
			QString information = "";
			QVector3D cm = selected->getInformationVector("total_cm");
			if (!selected->isInformationDefined("total_cm")) cm = selected->getInformationVector("cm");
			information = information + tr("CoM: (%1; %2; %3) m\n")
				.arg(cm.x(),0,'F',3)
				.arg(cm.y(),0,'F',3)
				.arg(cm.z(),0,'F',3);

			information = information + tr("Mass: %2 kg (part: %1 kg)\n")
				.arg(selected->getInformationVariable("mass"))
				.arg(selected->getInformationVariable("total_mass"));

			if (selected->getType() == "fuel_tank") {
				information = information + tr("\nFuel mass: %1 kg\n")
				.arg(selected->getInformationVariable("fuel_mass"));
				information = information + tr("Fuel volume: %1 m\xB3\n")
				.arg(selected->getInformationVariable("fuel_volume"));
			}

			QVector3D ix = selected->getInformationVector("total_ix");
			QVector3D iy = selected->getInformationVector("total_iy");
			QVector3D iz = selected->getInformationVector("total_iz");
			information = information + tr(
				"\nInertia tensor:\n"
				"(%1; %2; %3) kg m\xB2\n"
				"(%4; %5; %6) kg m\xB2\n"
				"(%7; %8; %9) kg m\xB2\n")
				.arg(ix.x(),0,'G',3).arg(ix.y(),0,'G',3).arg(ix.z(),0,'G',3)
				.arg(iy.x(),0,'G',3).arg(iy.y(),0,'G',3).arg(iy.z(),0,'G',3)
				.arg(iz.x(),0,'G',3).arg(iz.y(),0,'G',3).arg(iz.z(),0,'G',3);

			QVector3D jx = selected->getInformationVector("jx");
			QVector3D jy = selected->getInformationVector("jy");
			QVector3D jz = selected->getInformationVector("jz");
			information = information + tr(
				"\nRadius of gyration squared:\n"
				"(%1; %2; %3) m\xB2\n"
				"(%4; %5; %6) m\xB2\n"
				"(%7; %8; %9) m\xB2\n")
				.arg(jx.x(),0,'F',3).arg(jx.y(),0,'F',3).arg(jx.z(),0,'F',3)
				.arg(jy.x(),0,'F',3).arg(jy.y(),0,'F',3).arg(jy.z(),0,'F',3)
				.arg(jz.x(),0,'F',3).arg(jz.y(),0,'F',3).arg(jz.z(),0,'F',3);

			/*bodyinfo_f1->setText(tr("%1 kg").arg(iobject->getVariable("fuel_mass")));
			bodyinfo_f2->setText(tr("%1 m\xB3").arg(iobject->getVariable("fuel_volume")));*/

			bodyinfo->setText(
				tr("Object: %1\n\n%2")
				.arg(selected->getName())
				.arg(information)
			);
		} else {
			bodyinfo->setText("Object: (none)");
		}
	} else {
		bodyinfo->setText("Hold on...");
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::addObject() {
	QModelIndex index = list_tree->selectionModel()->currentIndex();
	static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->insertRow(0,index);
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
		list_model->updateObject(object);
	}
	glscene->update();
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
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::newFile() {
	EVDS_OBJECT_LOADEX info = { 0 };
	//EVDS_Object_LoadEx(root,"RV-505.evds",&info);
	EVDS_Object_LoadEx(root,"RV-505_fixed.evds",&info);
	root_obj->invalidateChildren();
	initializer->updateObject();
	updateInformation(false);
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
	root_obj->invalidateChildren();
	initializer->updateObject();
	updateInformation(false);
	QApplication::restoreOverrideCursor();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWE_SaveFile_RemoveRedundantVariables(EVDS_OBJECT* object) {
	//Search for geometry
	EVDS_VARIABLE* variable;
	if (EVDS_Object_GetVariable(object,"geometry.cross_sections",&variable) == EVDS_OK) {
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
		}
	}

	//Get list of children
	SIMC_LIST* list;
	SIMC_LIST_ENTRY* entry;
	EVDS_Object_GetAllChildren(object,&list);

	//Do same for every child
	entry = SIMC_List_GetFirst(list);
	while (entry) {
		FWE_SaveFile_RemoveRedundantVariables((EVDS_OBJECT*)SIMC_List_GetData(list,entry));
		entry = SIMC_List_GetNext(list,entry);
	}
}

bool Editor::saveFile(const QString &fileName) {
	//Filter out redundant variables
	FWE_SaveFile_RemoveRedundantVariables(root);

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
	list_model->dropMimeData(event->mimeData(),Qt::CopyAction,-1,-1,QModelIndex());
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
void Editor::updateInterface(bool isInFront) {
	for (int i = 0; i < actions.count(); i++) {
		actions[i]->setVisible(isInFront);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::showProperties() {
	properties_dock->show();
	properties_dock->raise();
}
void Editor::showCrossSections() {
	csection_dock->show();
	csection_dock->raise();
}
void Editor::showHierarchy() {
	list_dock->show();
	list_dock->raise();
}
/*void Editor::showCutsection() {
	cutsection_dock->show();
	cutsection_dock->raise();
}*/
void Editor::showInformation() {
	bodyinfo_dock->show();
	bodyinfo_dock->raise();
}