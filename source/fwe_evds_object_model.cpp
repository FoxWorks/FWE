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
#include <QtGui>
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_model.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectTreeModel::ObjectTreeModel(EVDS::Editor* in_editor, QWidget* parent) 
	: QAbstractItemModel(parent) 
{
	editor = in_editor;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectTreeModel::~ObjectTreeModel() {
	// ...
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
int ObjectTreeModel::columnCount(const QModelIndex &parent) const {
	return 2; //Name, type
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QVariant ObjectTreeModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid()) return QVariant();
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	Object* object = (Object*)(index.internalPointer());
	if (index.column() == 0) {
		return QVariant(object->getName());
	} else {
		return QVariant(object->getType());		
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Qt::ItemFlags ObjectTreeModel::flags(const QModelIndex &index) const {
	if (!index.isValid()) return Qt::ItemIsDropEnabled; //Allow dropping into root

	//Object* object = (Object*)(index.internalPointer());
	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

	//Can edit object names
	if (index.column() == 0) {
		return flags | Qt::ItemIsEditable;
	} else {
		return flags;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QStringList ObjectTreeModel::mimeTypes() const
{
	 QStringList types;
	 types << "application/vnd.evds+xml";
	 return types;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QMimeData* ObjectTreeModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QString encodedData = "";

	//Store all EVDS objects (actually only a single selected one supported right now)
	for (int i = 0; i < 1; i++) { //indexes.count()
		QModelIndex index = indexes[i];
		if (index.isValid() && (index.column() == 0)) {
			EVDS_OBJECT_SAVEEX info = { 0 };
			Object* object = (Object*)(index.internalPointer());

			//Copy copies as normal objects if copy is selected
			info.flags = EVDS_OBJECT_SAVEEX_SAVE_UIDS;
			//if (object->isCopy()) info.flags |= EVDS_OBJECT_SAVEEX_SAVE_COPIES;// | EVDS_OBJECT_SAVEEX_SKIP_MODIFIERS;
			EVDS_Object_SaveEx(object->getEVDSObject(),0,&info);

			//Encode data
			encodedData = info.description; //encodedData + info.description + "\n"
			free(info.description);
		}
	}

	mimeData->setText(encodedData);
	mimeData->setData("application/vnd.evds+xml",encodedData.toUtf8());
	return mimeData;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ObjectTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, 
								   int row, int column, const QModelIndex &parent) {
	//Verify drop can be accepted
	if (action == Qt::IgnoreAction)	return true;
	if (!data->hasFormat("application/vnd.evds+xml")) return false;
	if (column > 0) return false;

	//Get index
	int beginRow;
	if (row != -1) {
		beginRow = row;
	} else if (parent.isValid()) {
		beginRow = parent.row(); // rowCount(parent);//
	} else {
		beginRow = rowCount(QModelIndex());
	}

	//Insert new object from encoded data
	Object* object;
	if (!parent.isValid()) {
		object = editor->getEditRoot();
	} else {
		object = (Object*)(parent.internalPointer());
	}

	beginInsertRows(parent,beginRow,beginRow);
	object->insertChild(beginRow,data->text());
	endInsertRows();

	editor->setModified();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QVariant ObjectTreeModel::headerData(int section, Qt::Orientation orientation,
									 int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if (section == 0) {
			return QVariant("Name");
		} else {
			return QVariant("Type");
		}
	}
	return QVariant();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QModelIndex ObjectTreeModel::index(int row, int column, const QModelIndex &parent) const {
	if (!hasIndex(row, column, parent)) return QModelIndex();

	Object* object;
	if (!parent.isValid()) {
		object = editor->getEditRoot();
	} else {
		object = (Object*)(parent.internalPointer());
	}

	Object* child = object->getChild(row);
	if (child)
		return createIndex(row, column, child);
	else
		return QModelIndex();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QModelIndex ObjectTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid()) return QModelIndex();

	Object* object = (Object*)(index.internalPointer());
	Object* parent = object->getParent();

	if (parent == editor->getEditRoot()) return QModelIndex();

	Object* parent_parent = parent->getParent();
	int idx = parent_parent->getChildIndex(parent);
	return createIndex(idx, 0, parent);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
int ObjectTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0) return 0;

	Object* object;
	if (!parent.isValid()) {
		object = editor->getEditRoot();
	} else {
		object = (Object*)(parent.internalPointer());
	}
	return object->getChildrenCount();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
//Object* ObjectTreeModel::appendRow(const QModelIndex& parent) {
//bool ObjectTreeModel::insertRow(int row, const QModelIndex& parent) {
bool ObjectTreeModel::insertRows(int row, int count, const QModelIndex &parent) {
	Object* object;
	if (!parent.isValid()) {
		object = editor->getEditRoot();
	} else {
		object = (Object*)(parent.internalPointer());
	}

	beginInsertRows(parent,row,row+count-1);
	for (int r=row;r<row+count;r++) {
		object->insertNewChild(r);
	}
	endInsertRows();
	editor->setModified();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ObjectTreeModel::removeRows(int row, int count, const QModelIndex &parent) {
	Object* object;
	if (!parent.isValid()) {
		object = editor->getEditRoot();
	} else {
		object = (Object*)(parent.internalPointer());
	}

	beginRemoveRows(parent,row,row+count-1);
	for (int r=row;r<row+count;r++) {
		object->removeChild(r);
	}
	endRemoveRows();
	editor->setModified();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ObjectTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole) return false;

	Object* object = (Object*)(index.internalPointer());
	object->setName(value.toString());

	emit dataChanged(index, index);
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ObjectTreeModel::updateObject(Object* object) {
	Object* parent = object->getParent();
	int idx = parent->getChildIndex(object);

	emit dataChanged(
		createIndex(idx, 0, object),
		createIndex(idx, 1, object));
}
