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
#include <QWidget>

#include <GLC_3DViewCollection>
#include <GLC_3DViewInstance>
#include <GLC_Context>
#include <QGLWidget>

#include <math.h>

#include "fwe_schematics.h"
#include "fwe_schematics_renderer.h"
#include "fwe_glscene.h"
#include "fwe_prop_sheet.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_model.h"
#include "fwe_dock_objectlist.h"
#include "fwe_dock_properties.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
SchematicsEditor::SchematicsEditor(FWE::EditorWindow* in_window) : FWE::Editor(in_window) {
	selected = NULL;
	root = NULL;
	sheet = NULL;
	prev_sheet = NULL;

	//Create parts of main UI (other parts are created later)
	createMenuToolbar();

	//Create working area/main 3D widget
	glview = new GLView(this);
	glscene = new GLScene(0,getEVDSEditor(),this,this);
	glview->setScene(glscene);
	setCentralWidget(glview);

	//Create informational docks
	//createInformationDock();
	//createCommentsDock();

	//Create rendering manager
	rendering_manager = new SchematicsRenderingManager(this);

	//Enable drag and drop
	setAcceptDrops(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
SchematicsEditor::~SchematicsEditor() {
	delete rendering_manager;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::createMenuToolbar() {
	//action = new QAction(QIcon(":/icon/evds/materials.png"), tr("&Materials Database..."), this);
	//action->setStatusTip("Display list of materials and their physical properties");
	////connect(action, SIGNAL(triggered()), this, SLOT(showCrossSections()));
	//action->setEnabled(false);
	//actions.append(action);
	//window->getMainWindow()->getViewMenu()->addAction(action);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief 
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::createCommentsDock() {
	comments = new QTextEdit();
	comments->setObjectName("EVDS_Comments");

	comments_dock = new QDockWidget(tr("Element Text"), this);
	comments_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	comments_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	comments_dock->setWidget(comments);
	//tabifyDockWidget(bodyinfo_dock,comments_dock);
	addDockWidget(Qt::RightDockWidgetArea,comments_dock);
	//bodyinfo_dock->raise();

	comments->setMaximumHeight(135);
}

void SchematicsEditor::commentsChanged() {
	if (selected) {
		selected->setVariable("text",comments->toPlainText());
	} else {
		getEditDocument()->setVariable("text",comments->toPlainText());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback from when object was modified
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::setActive(bool active) {
	isActive = active;

	//Clear out or re-initialize all modifier-created instances to avoid crashes
	if (active) {
		rendering_manager->updateInstances(); 
	}
}
/*
void SchematicsEditor::setEditorHidden(bool isHidden) {
	if (isHidden) sheet = 0;
	
	if (!isHidden) { //Invalidate objects tree FIXME
		if (objectlist_model) {
			delete objectlist_model;
			
			objectlist_model = new ObjectTreeModel(editor,editor->getEditRoot(),this);
			objectlist_model->setAcceptedMimeType("application/vnd.evds.none");
			objectlist_tree->setModel(objectlist_model);
		}
	}
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::addObject() {
	Object* object = elements_list->getModel()->newObject(0,elements_list->currentIndex());

	if (object->getParent() == getSchematicsEditor()->getMetadataRoot()) {
		object->setType("foxworks.schematics.sheet");
	} else {
		object->setType("foxworks.schematics.element");
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::removeObject() {
	QModelIndex index = elements_list->currentIndex();
	elements_list->getModel()->removeRow(index.row(),index.parent());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::selectObject(const QModelIndex& index) {
	//Should selection be cleared
	if (!index.isValid()) {
		element_properties->setPropertySheet(getEditDocument()->getPropertySheet());
		selected = NULL;

		//Update information
		updateObject(NULL);

		//Update comments
		disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
		comments->setText(getEditDocument()->getString("text"));
		connect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
		return;
	}

	//Check if selection must be cleared
	Object* object = (Object*)index.internalPointer();
	if (selected == object) {
		elements_list->setCurrentIndex(QModelIndex());
		selectObject(QModelIndex());
		return;
	}
	selected = object;

	//Show proper properties sheet
	element_properties->setPropertySheet(object->getPropertySheet());

	//Get currently selected schematics sheet
	sheet = object;
	while (sheet && (sheet->getType() != "foxworks.schematics.sheet")) {
		sheet = sheet->getParent();
		if (sheet == root) sheet = NULL;
	}
	if (sheet != prev_sheet) {
		rendering_manager->updateInstances();
		glscene->doCenter();
	}
	prev_sheet = sheet;

	//Update comments
	disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
	comments->setText(object->getString("text"));
	connect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));

	//Redraw
	updateObject(NULL);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::updateObject(Object* object) {
	if (object) {
		elements_list->getModel()->updateObject(object);
	}
	rendering_manager->updateInstances();
	glscene->update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::elementPropertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet) {
	element_properties->removePropertySheet(old_sheet);
	element_properties->setPropertySheet(new_sheet);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::finishInitializing() {
	//Read schematics
	Object* document = getEditDocument();
	root = NULL;
	for (int i = 0; i < document->getChildrenCount(); i++) {
		if (document->getChild(i)->getType() == "foxworks.schematics") {
			root = document->getChild(i);
			break;
		}
	}

	//Create new if not present
	if (!root) {
		root = document->insertNewChild(0);
		root->setType("foxworks.schematics");
		root->setName("");
	}

	//Create schematics editor itself
	//createObjectListDock();
	//createListDock();
	//createPropertiesDock();

	//Object list
	object_list = new Dock::ObjectList(getEditRoot(),this);
	addDockWidget(Qt::LeftDockWidgetArea, object_list);

	//Make sure the object list does not accept any mime types
	object_list->getModel()->setAcceptedMimeType("application/vnd.evds.none");
	object_list->hideButtons();

	//Setup initial layout
	addDockWidget(Qt::RightDockWidgetArea, object_list);
	object_list->raise();


	//Element list
	elements_list = new Dock::ObjectList(root,this);
	elements_list->setWindowTitle(tr("Schematics Elements"));
	connect(elements_list, SIGNAL(addObject()), this, SLOT(addObject()));
	connect(elements_list, SIGNAL(removeObject()), this, SLOT(removeObject()));
	connect(elements_list, SIGNAL(selectObject(const QModelIndex&)), this, SLOT(selectObject(const QModelIndex&)));
	addDockWidget(Qt::LeftDockWidgetArea, object_list);

	//Make sure the element list only accepts references
	elements_list->getModel()->setAcceptedMimeType("application/vnd.evds.ref+xml");

	//Setup initial layout (a bit late but still)
	addDockWidget(Qt::RightDockWidgetArea, elements_list);
	elements_list->raise();


	//Element properties
	element_properties = new Dock::Properties(this);
	element_properties->setWindowTitle(tr("Element Properties"));
	addDockWidget(Qt::LeftDockWidgetArea, element_properties);

	createCommentsDock();
	selectObject(QModelIndex());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::dropEvent(QDropEvent *event) {
	//list_model->dropMimeData(event->mimeData(),Qt::CopyAction,-1,-1,QModelIndex());
	//qDebug("Contents: %s", event->mimeData()->text().toLatin1().data());
}

void SchematicsEditor::dragMoveEvent(QDragMoveEvent *event) {
	//event->accept();
}

void SchematicsEditor::dragEnterEvent(QDragEnterEvent *event) {
	//event->acceptProposedAction();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
/*void SchematicsEditor::showProperties() {
	properties_dock->show();
	properties_dock->raise();
}
void SchematicsEditor::showCrossSections() {
	csection_dock->show();
	csection_dock->raise();
}
void SchematicsEditor::showHierarchy() {
	list_dock->show();
	list_dock->raise();
}
void SchematicsEditor::showInformation() {
	bodyinfo_dock->show();
	bodyinfo_dock->raise();
}

void SchematicsEditor::showComments() {
	comments_dock->show();
	comments_dock->raise();
}*/