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
#include <QtCore/QMap>
#include "fwe_prop_stringenum.h"

using namespace EVDS;

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