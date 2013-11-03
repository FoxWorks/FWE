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
#ifndef FWE_DOCK_OBJECTLIST_H
#define FWE_DOCK_OBJECTLIST_H

#include <QDockWidget>
#include <QTreeView>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QWidget;
class QPushButton;
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
namespace EVDS {
	class Object;
	class ObjectInitializer;
	class ObjectTreeModel;
	class ObjectModifiersManager;
}
namespace Dock {
	class ObjectList : public QDockWidget {
		Q_OBJECT

	public:
		ObjectList(EVDS::Object* root, QWidget* parent);

		void setCurrentIndex(QModelIndex index) { object_tree->setCurrentIndex(index); }
		QModelIndex currentIndex() { return object_tree->selectionModel()->currentIndex(); }
		EVDS::ObjectTreeModel* getModel() { return model; }

	signals:
		void addObject();
		void removeObject();
		void selectObject(const QModelIndex& index);

	private slots:
		void doAddObject() { addObject(); }
		void doRemoveObject() { removeObject(); }
		void doSelectObject(const QModelIndex& index) { selectObject(index); }

	private:
		EVDS::ObjectTreeModel*	model;
		QWidget*				form;
		QTreeView*				object_tree;
		QPushButton*			button_add;
		QPushButton*			button_remove;
	};
}

#endif
