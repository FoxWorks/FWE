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
/*#include "fwe_evds.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_modifiers.h"*/

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
SchematicsEditor::SchematicsEditor(ChildWindow* in_window, EVDS::Editor* in_editor) : QMainWindow(in_window) {
	window = in_window;
	editor = in_editor;
	selected = NULL;
	root = NULL;
	sheet = NULL;
	prev_sheet = NULL;
	objectlist_model = NULL;

	//Create parts of main UI (other parts are created later)
	createMenuToolbar();

	//Create working area/main 3D widget
	glview = new GLView(this);
	glscene = new GLScene(0,editor,this,this);
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
	for (int i = 0; i < actions.count(); i++) {
		actions[i]->deleteLater();
	}

	//delete glview;
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
void SchematicsEditor::createListDock() {
	list = new QWidget();
	list->setMinimumWidth(250);
	list->setMinimumHeight(100);

	//Create object hierarchy window and dock
	list_dock = new QDockWidget(tr("Schematics Elements"), this);
	list_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	list_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	list_dock->setWidget(list);
	addDockWidget(Qt::LeftDockWidgetArea, list_dock);

	//Create remaining interface
	list_model = new ObjectTreeModel(editor,root,this);
	list_model->setAcceptedMimeType("application/vnd.evds.ref+xml");
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

	//Setup initial layout (a bit late but still)
	list_dock->raise();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::createObjectListDock() {
	objectlist = new QWidget();
	objectlist->setMinimumWidth(250);
	objectlist->setMinimumHeight(100);

	//Create object hierarchy window and dock
	objectlist_dock = new QDockWidget(tr("Objects Hierarchy"), this);
	objectlist_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	objectlist_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	objectlist_dock->setWidget(objectlist);
	addDockWidget(Qt::RightDockWidgetArea, objectlist_dock);

	//Create remaining interface
	objectlist_model = new ObjectTreeModel(editor,editor->getEditRoot(),this);
	objectlist_model->setAcceptedMimeType("application/vnd.evds.none");
	objectlist_tree = new QTreeView(this);
	objectlist_tree->setModel(objectlist_model);
	objectlist_tree->expandAll();
	objectlist_tree->setColumnWidth(0,150);

	//objectlist_tree->viewport()->setAcceptDrops(true);
	objectlist_tree->setDragDropMode(QAbstractItemView::DragDrop);
	objectlist_tree->setDragEnabled(true);
	//objectlist_tree->setDropIndicatorShown(true);
	//objectlist_tree->setDragDropOverwriteMode(false);
	//objectlist_tree->setDefaultDropAction(Qt::MoveAction);

	//Create layout
	objectlist_layout = new QVBoxLayout;
	objectlist_layout->addWidget(objectlist_tree);
	objectlist->setLayout(objectlist_layout);

	//Setup initial layout (a bit late but still)
	objectlist_dock->raise();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::createPropertiesDock() {
	properties = new QWidget();
	properties->setMinimumWidth(250);
	properties->setMinimumHeight(80);

	//Create properties interface
	properties_dock = new QDockWidget(tr("Element Properties"), this);
	properties_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	properties_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	properties_dock->setWidget(properties);
	addDockWidget(Qt::LeftDockWidgetArea, properties_dock);

	//Create layout
	properties_layout = new QStackedLayout;
	properties->setLayout(properties_layout);
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
		editor->getEditDocument()->setVariable("text",comments->toPlainText());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback from when object was modified
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::setModified() {
	editor->setModified();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Callback from when object was modified
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::setEditorHidden(bool isHidden) {
	if (isHidden) sheet = 0;
	rendering_manager->updateInstances(); //Clear out all modifier-created instances to avoid crashes
	if (!isHidden) { //Invalidate objects tree
		if (objectlist_model) {
			delete objectlist_model;
			
			objectlist_model = new ObjectTreeModel(editor,editor->getEditRoot(),this);
			objectlist_model->setAcceptedMimeType("application/vnd.evds.none");
			objectlist_tree->setModel(objectlist_model);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::addObject() {
	QModelIndex index = list_tree->selectionModel()->currentIndex();
	static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->insertRow(0,index);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::removeObject() {
	QModelIndex index = list_tree->selectionModel()->currentIndex();
	static_cast<EVDS::ObjectTreeModel*>(list_tree->model())->removeRow(index.row(),index.parent());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::selectObject(const QModelIndex& index) {
	//Should selection be cleared
	if (!index.isValid()) {
		QWidget* property_sheet = editor->getEditDocument()->getPropertySheet();
		if (properties_layout->indexOf(property_sheet) < 0) {
			properties_layout->addWidget(property_sheet);
		}
		properties_layout->setCurrentWidget(property_sheet);
		selected = NULL;

		//Update information
		updateObject(NULL);

		//Update comments
		disconnect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
		comments->setText(editor->getEditDocument()->getString("text"));
		connect(comments, SIGNAL(textChanged()), this, SLOT(commentsChanged()));
		return;
	}

	//Check if selection must be cleared
	Object* object = (Object*)index.internalPointer();
	if (selected == object) {
		list_tree->setCurrentIndex(QModelIndex());
		selectObject(QModelIndex());
		return;
	}
	selected = object;

	//Show proper properties sheet
	QWidget* property_sheet = object->getPropertySheet();
	if (properties_layout->indexOf(property_sheet) < 0) {
		properties_layout->addWidget(property_sheet);
	}
	properties_layout->setCurrentWidget(property_sheet);

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
	invalidateChildren(root);
	if (object) {
		list_model->updateObject(object);
	}
	//rendering_manager->updateInstances();
	glscene->update();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void SchematicsEditor::propertySheetUpdated(QWidget* old_sheet, QWidget* new_sheet) {
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
void SchematicsEditor::invalidateChildren(Object* object) {
	object->setSchematicsEditor(this);
	for (int i = 0; i < object->getChildrenCount(); i++) {
		invalidateChildren(object->getChild(i));
	}
}

void SchematicsEditor::initializeForFile() {
	//Read schematics
	Object* document = editor->getEditDocument();
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
	invalidateChildren(root);

	//Create schematics editor itself
	createObjectListDock();
	createListDock();
	createPropertiesDock();
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
void SchematicsEditor::updateInterface(bool isInFront) {
	for (int i = 0; i < actions.count(); i++) {
		actions[i]->setVisible(isInFront);
	}
	//cutsection_menu->menuAction()->setVisible(isInFront);
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