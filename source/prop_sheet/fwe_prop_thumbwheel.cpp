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
#include <QHBoxLayout>
#include "qtthumbwheel.h"
#include "fwe_prop_thumbwheel.h"

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
	delete originalFactory;
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