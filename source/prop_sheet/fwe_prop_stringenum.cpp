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
#include <QtCore/QMap>
#include "fwe_prop_stringenum.h"

StringEnumPropertyManager::StringEnumPropertyManager(QObject *parent)
    : QtEnumPropertyManager(parent)
{
}

StringEnumPropertyManager::~StringEnumPropertyManager()
{
}

QString StringEnumPropertyManager::stringValue(const QtProperty *property) const
{
    if (!propertyToValues.contains(property))
        return QString();
    return propertyToValues[property].at(this->value(property));
}

void StringEnumPropertyManager::setStringValue(QtProperty *property, QString value) {
    if (!propertyToValues.contains(property))
		return;

    this->setValue(property, propertyToValues[property].indexOf(value));
}

void StringEnumPropertyManager::setEnumValues(QtProperty *property, const QStringList &values)
{
    if (!propertyToValues.contains(property))
        return;

    propertyToValues[property] = values;

    emit enumValuesChanged(property,values);
}

void StringEnumPropertyManager::initializeProperty(QtProperty *property)
{
    propertyToValues[property] = QStringList();
    QtEnumPropertyManager::initializeProperty(property);
}

void StringEnumPropertyManager::uninitializeProperty(QtProperty *property)
{
    propertyToValues.remove(property);
    QtEnumPropertyManager::uninitializeProperty(property);
}




StringEnumFactory::StringEnumFactory(QObject *parent)
	: QtAbstractEditorFactory<StringEnumPropertyManager>(parent)
{
	originalFactory = new QtEnumEditorFactory(this);
}

StringEnumFactory::~StringEnumFactory()
{
}

void StringEnumFactory::connectPropertyManager(StringEnumPropertyManager *manager)
{
	originalFactory->addPropertyManager(manager);
	//connect(manager, SIGNAL(prefixChanged(QtProperty *, const QString &)), this, SLOT(slotPrefixChanged(QtProperty *, const QString &)));
	//connect(manager, SIGNAL(suffixChanged(QtProperty *, const QString &)), this, SLOT(slotSuffixChanged(QtProperty *, const QString &)));
}

QWidget *StringEnumFactory::createEditor(StringEnumPropertyManager *manager, QtProperty *property,
		QWidget *parent)
{
	QtAbstractEditorFactoryBase *base = originalFactory;
	QWidget *w = base->createEditor(property, parent);
	if (!w)
		return 0;

	createdEditors[property].append(w);
	editorToProperty[w] = property;
	return w;
}

void StringEnumFactory::disconnectPropertyManager(StringEnumPropertyManager *manager)
{
	originalFactory->removePropertyManager(manager);
	//disconnect(manager, SIGNAL(prefixChanged(QtProperty *, const QString &)), this, SLOT(slotPrefixChanged(QtProperty *, const QString &)));
	//disconnect(manager, SIGNAL(suffixChanged(QtProperty *, const QString &)), this, SLOT(slotSuffixChanged(QtProperty *, const QString &)));
}

void StringEnumFactory::slotEditorDestroyed(QObject *object)
{
	QMap<QWidget *, QtProperty *>::ConstIterator itEditor =
				editorToProperty.constBegin();
	while (itEditor != editorToProperty.constEnd()) {
		if (itEditor.key() == object) {
			QWidget *editor = itEditor.key();
			QtProperty *property = itEditor.value();
			editorToProperty.remove(editor);
			createdEditors[property].removeAll(editor);
			if (createdEditors[property].isEmpty())
				createdEditors.remove(property);
			return;
		}
		itEditor++;
	}
}