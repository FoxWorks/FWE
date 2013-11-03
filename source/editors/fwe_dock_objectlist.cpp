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
#include <QDockWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "fwe_dock_objectlist.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_model.h"

using namespace EVDS;
using namespace Dock;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectList::ObjectList(Object* root, QWidget* parent) : QDockWidget(tr("Objects Hierarchy"),parent) {
	//Setup widgets properties
	setFeatures(QDockWidget::AllDockWidgetFeatures);
	setAllowedAreas(Qt::AllDockWidgetAreas);

	//The model and the list
	model = new ObjectTreeModel(root->getEVDSEditor(),root,this);
	object_tree = new QTreeView(this);
	object_tree->setModel(model);
	object_tree->expandAll();
	object_tree->setColumnWidth(0,150);

	//Setup drag and drop
	object_tree->viewport()->setAcceptDrops(true);
	object_tree->setDragDropMode(QAbstractItemView::DragDrop);
	object_tree->setDragEnabled(true);
	object_tree->setDropIndicatorShown(true);
	object_tree->setDragDropOverwriteMode(false);
	object_tree->setDefaultDropAction(Qt::MoveAction);

	//Create control buttons
	button_add = new QPushButton(QIcon(":/icon/add.png"),"Add object",this);
	button_remove = new QPushButton(QIcon(":/icon/remove.png"),"Remove selected",this);
	connect(button_add, SIGNAL(released()), this, SLOT(doAddObject()));
	connect(button_remove, SIGNAL(released()), this, SLOT(doRemoveObject()));
	connect(object_tree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(doSelectObject(const QModelIndex&)));

	//Create form and layout
	form = new QWidget(this);
	form->setMinimumWidth(250);
	form->setMinimumHeight(100);
	setWidget(form);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(button_add);
	layout->addWidget(object_tree);
	layout->addWidget(button_remove);
	form->setLayout(layout);	
}