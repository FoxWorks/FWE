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
#include <QHBoxLayout>
#include "qtthumbwheel.h"
#include "fwe_prop_thumbwheel.h"

using namespace EVDS;

ThumbWheelPropertyManager::ThumbWheelPropertyManager(QObject *parent)
    : QtDoublePropertyManager(parent) {
}

ThumbWheelPropertyManager::~ThumbWheelPropertyManager() {
}

QString ThumbWheelPropertyManager::prefix(const QtProperty *property) const {
    if (!propertyToData.contains(property))
        return QString();
    return propertyToData[property].prefix;
}

QString ThumbWheelPropertyManager::suffix(const QtProperty *property) const {
    if (!propertyToData.contains(property))
        return QString();
    return propertyToData[property].suffix;
}

bool ThumbWheelPropertyManager::hideZeroes(const QtProperty *property) const {
	if (!propertyToData.contains(property))
        return false;
    return propertyToData[property].hideZeroes;
}

void ThumbWheelPropertyManager::setHideZeroes(QtProperty *property, bool value) {
    if (!propertyToData.contains(property))
        return;

    ThumbWheelPropertyManager::Data data = propertyToData[property];
    if (data.hideZeroes == value)
        return;

    data.hideZeroes = value;
    propertyToData[property] = data;

    emit propertyChanged(property);
    emit hideZeroesChanged(property, value);
}

void ThumbWheelPropertyManager::setHiddenValue(QtProperty *property, double value) {
    if (!propertyToData.contains(property))
        return;

    ThumbWheelPropertyManager::Data data = propertyToData[property];
    if (data.hiddenValue == value)
        return;

    data.hiddenValue = value;
    propertyToData[property] = data;

    emit propertyChanged(property);
    emit hiddenValueChanged(property, value);
}

void ThumbWheelPropertyManager::setPrefix(QtProperty *property, const QString &prefix) {
    if (!propertyToData.contains(property))
        return;

    ThumbWheelPropertyManager::Data data = propertyToData[property];
    if (data.prefix == prefix)
        return;

    data.prefix = prefix;
    propertyToData[property] = data;

    emit propertyChanged(property);
    emit prefixChanged(property, prefix);
}

void ThumbWheelPropertyManager::setSuffix(QtProperty *property, const QString &suffix) {
    if (!propertyToData.contains(property))
        return;

    ThumbWheelPropertyManager::Data data = propertyToData[property];
    if (data.suffix == suffix)
        return;

    data.suffix = suffix;
    propertyToData[property] = data;

    emit propertyChanged(property);
    emit suffixChanged(property, suffix);
}

QString ThumbWheelPropertyManager::valueText(const QtProperty *property) const {
    QString text = QtDoublePropertyManager::valueText(property);
    if (!propertyToData.contains(property))
        return text;

    ThumbWheelPropertyManager::Data data = propertyToData[property];
	if (data.hideZeroes) {
		if (QtDoublePropertyManager::value(property) == 0.0) {
			if (data.hiddenValue != 0.0) {
				text = "[" + tr("%1").arg(data.hiddenValue) + "]";
			} else {
				text = "...";
			}
		}
	}
    text = data.prefix + text + data.suffix;

    return text;
}

void ThumbWheelPropertyManager::initializeProperty(QtProperty *property) {
    propertyToData[property] = ThumbWheelPropertyManager::Data();
	propertyToData[property].hiddenValue = 0.0;
    QtDoublePropertyManager::initializeProperty(property);
}

void ThumbWheelPropertyManager::uninitializeProperty(QtProperty *property) {
    propertyToData.remove(property);
    QtDoublePropertyManager::uninitializeProperty(property);
}




ThumbWheelFactory::ThumbWheelFactory(QObject *parent)
	: QtAbstractEditorFactory<ThumbWheelPropertyManager>(parent) {
	originalFactory = new QtDoubleSpinBoxFactory(this);
}

ThumbWheelFactory::~ThumbWheelFactory()
{
}

void ThumbWheelFactory::connectPropertyManager(ThumbWheelPropertyManager *manager) {
	originalFactory->addPropertyManager(manager);
	connect(manager, SIGNAL(prefixChanged(QtProperty *, const QString &)), this, SLOT(slotPrefixChanged(QtProperty *, const QString &)));
	connect(manager, SIGNAL(suffixChanged(QtProperty *, const QString &)), this, SLOT(slotSuffixChanged(QtProperty *, const QString &)));
}

QWidget *ThumbWheelFactory::createEditor(ThumbWheelPropertyManager *manager, QtProperty *property,
		QWidget *parent) {
	/*QtAbstractEditorFactoryBase *base = originalFactory;
	QWidget *w = base->createEditor(property, parent);
	if (!w)
		return 0;

	QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox *>(w);
	if (!spinBox)
		return 0;*/

	QWidget* editBox = new QWidget(parent);

	QtAbstractEditorFactoryBase *base = originalFactory;
	QWidget *w = base->createEditor(property, editBox);
	QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox *>(w);
	//QSpinBox* spinBox = new QSpinBox(editBox);

	QtThumbWheel* thumbWheel = new QtThumbWheel(editBox);
	QHBoxLayout* layout = new QHBoxLayout;
	layout->addWidget(spinBox);
	layout->addWidget(thumbWheel);
	layout->setMargin(0);
	layout->setStretch(0,0);
	layout->setStretch(0,1);
	editBox->setLayout(layout);

	thumbWheel->setLimitedDrag(false);
	thumbWheel->setWrapsAround(true);
	spinBox->setMinimumWidth(64);
	spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	//thumbWheel->setMinimumWidth(32);

	spinBox->setPrefix(manager->prefix(property));
	spinBox->setSuffix(manager->suffix(property));

	//connect(spinBox, SIGNAL(valueChanged(double)), thumbWheel, SLOT(setDoubleValue(double)));
	connect(thumbWheel, SIGNAL(steppedUp()), spinBox, SLOT(stepUp()));
	connect(thumbWheel, SIGNAL(steppedDown()), spinBox, SLOT(stepDown()));

	createdEditors[property].append(editBox);
	editorToProperty[editBox] = property;

	return editBox;
}

void ThumbWheelFactory::disconnectPropertyManager(ThumbWheelPropertyManager *manager) {
	originalFactory->removePropertyManager(manager);
	disconnect(manager, SIGNAL(prefixChanged(QtProperty *, const QString &)), this, SLOT(slotPrefixChanged(QtProperty *, const QString &)));
	disconnect(manager, SIGNAL(suffixChanged(QtProperty *, const QString &)), this, SLOT(slotSuffixChanged(QtProperty *, const QString &)));
}

void ThumbWheelFactory::slotPrefixChanged(QtProperty *property, const QString &prefix) {
	if (!createdEditors.contains(property))
		return;

	ThumbWheelPropertyManager *manager = propertyManager(property);
	if (!manager)
		return;

	QList<QWidget *> editors = createdEditors[property];
	QListIterator<QWidget *> itEditor(editors);
	while (itEditor.hasNext()) {
		QWidget *editor = itEditor.next();
		//editor->setPrefix(prefix);
	}
}

void ThumbWheelFactory::slotSuffixChanged(QtProperty *property, const QString &prefix) {
	if (!createdEditors.contains(property))
		return;

	ThumbWheelPropertyManager *manager = propertyManager(property);
	if (!manager)
		return;

	QList<QWidget *> editors = createdEditors[property];
	QListIterator<QWidget *> itEditor(editors);
	while (itEditor.hasNext()) {
		QWidget *editor = itEditor.next();
		//editor->setSuffix(prefix);
	}
}

void ThumbWheelFactory::slotEditorDestroyed(QObject *object) {
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