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
#ifndef FWE_EVDS_OBJECT_MODEL_H
#define FWE_EVDS_OBJECT_MODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

namespace FWE {
	class EditorWindow;
}
namespace EVDS {
	class Editor;
	class Object;
	class ObjectTreeModel : public QAbstractItemModel
	{
		Q_OBJECT

	public:
		ObjectTreeModel(FWE::EditorWindow* in_window, EVDS::Object* in_root, QWidget* parent);
		~ObjectTreeModel();

		QVariant data(const QModelIndex &index, int role) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		QVariant headerData(int section, Qt::Orientation orientation,
							int role = Qt::DisplayRole) const;
		QModelIndex index(int row, int column,
						  const QModelIndex &parent = QModelIndex()) const;
		QModelIndex parent(const QModelIndex &index) const;
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
		bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
		bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

		Object* newObject(int row, QModelIndex index);

		void updateObject(Object* object);
		void setAcceptedMimeType(const QString& type) { acceptedMimeType = type; }

		Qt::DropActions supportedDropActions() const { return Qt::CopyAction | Qt::MoveAction; }
		QStringList mimeTypes() const;
		QMimeData* mimeData(const QModelIndexList &indexes) const;
		bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

	private:
		QString acceptedMimeType;
		FWE::EditorWindow* window;
		EVDS::Object* root;
	};
}

#endif
