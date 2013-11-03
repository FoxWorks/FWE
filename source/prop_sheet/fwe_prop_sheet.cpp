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
#include <QString>
#include <QDateTime>
#include <math.h>

#include "fwe_prop_sheet.h"
#include "fwe_prop_stringenum.h"
#include "fwe_prop_thumbwheel.h"


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
FWEPropertySheet::FWEPropertySheet(QWidget* parent) : QtTreePropertyBrowser(parent) {
	initialValueHack = 0;

	//Create managers and factories
	groupManager = new QtGroupPropertyManager();
	doubleManager = new ThumbWheelPropertyManager();
	doubleFactory = new ThumbWheelFactory();
	enumManager = new StringEnumPropertyManager();
	enumFactory = new StringEnumFactory();
	booleanManager = new QtBoolPropertyManager();
	booleanFactory = new QtCheckBoxFactory();
	stringManager = new QtStringPropertyManager();
	stringFactory = new QtLineEditFactory();
	datetimeManager = new QtDateTimePropertyManager();
	datetimeFactory = new QtDateTimeEditFactory();

	connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
			this, SLOT(doublePropertyChanged(QtProperty *, double)));
	connect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
			this, SLOT(enumPropertyChanged(QtProperty *, int)));
	connect(booleanManager, SIGNAL(valueChanged(QtProperty *, bool)),
			this, SLOT(booleanPropertyChanged(QtProperty *, bool)));
	connect(stringManager, SIGNAL(valueChanged(QtProperty *, const QString&)),
			this, SLOT(stringPropertyChanged(QtProperty *, const QString&)));
	connect(datetimeManager, SIGNAL(valueChanged(QtProperty *, const QDateTime&)),
			this, SLOT(dateTimePropertyChanged(QtProperty *, const QDateTime&)));

	//Create browser
	setFactoryForManager(doubleManager, doubleFactory);
	setFactoryForManager(enumManager, enumFactory);
	setFactoryForManager(booleanManager, booleanFactory);
	setFactoryForManager(stringManager, stringFactory);
	setFactoryForManager(datetimeManager, datetimeFactory);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
FWEPropertySheet::~FWEPropertySheet() {
	// ...
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::setProperties(const QList<QMap<QString,QString> >& list) {
	for (int i = 0; i < list.count(); i++) {
		setProperty(list[i]);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QtProperty* FWEPropertySheet::setProperty(const QMap<QString,QString> &data) {
	QtProperty* property = NULL;

	//Get information about property being added
	QString type = data["type"];
	bool important = data["important"] == "true";
	QString var_name = data["variable"];
	QString group = data["group"];
	QString name = data["name"];
	QString step_str = data["step"];
	QString decimals_str = data["digits"];
	QString units = data["units"];
	QString prefix = data["prefix"];
	QString desc = data["description"];
	QStringList values = data["values"].split("\n",QString::SkipEmptyParts);

	if (propertyByName.contains(var_name)) {
		qCritical("Property already exists");
		//return 0;
	}

	//Add to property sheet
	if (type == "double") {
		property = doubleManager->addProperty(name);
		if (units != "") doubleManager->setSuffix(property, " " + units);
		if (prefix != "") doubleManager->setPrefix(property, prefix);
		//doubleManager->setMinimum(property,0.0);
		doubleManager->setHideZeroes(property,!important);

		bool ok;
		int decimals = decimals_str.toInt(&ok);
		if (ok) {
			doubleManager->setDecimals(property,decimals);
		} else {
			doubleManager->setDecimals(property,3);
		}

		double step = step_str.toDouble(&ok);
		if (ok) doubleManager->setSingleStep(property,step);
	} else if (type == "bool") {
		property = booleanManager->addProperty(name);
	} else if (type == "enum") {
		QStringList enum_names;
		QStringList enum_values;
		enum_values << "";
		enum_names << "...";
		for (int i = 0; i < values.count()/2; i++) {
			enum_values << values[i*2+0];
			enum_names << values[i*2+1];
		}

		property = enumManager->addProperty(name);
		initialValueHack = 1;
		enumManager->setEnumValues(property,enum_values);
		enumManager->setEnumNames(property,enum_names);
		initialValueHack = 0;
	} else if (type == "string") {
		property = stringManager->addProperty(name);
	} else if (type == "datetime") {
		property = datetimeManager->addProperty(name);
	}

	//Add to group
	if (group != "") {
		if (!propertyGroupByName[group]) {
			if (propertyByName[group]) {
				propertyGroupByName[group] = propertyByName[group];
			} else {
				propertyGroupByName[group] = groupManager->addProperty(group);
				setExpanded(addProperty(propertyGroupByName[group]),true);
			}
		}
		propertyGroupByName[group]->addSubProperty(property);
	} else {
		setExpanded(this->addProperty(property),false);
	}

	//Remember variable name
	if (property) {
		propertyName[property] = var_name;
		propertyType[property] = type;
		propertyByName[var_name] = property;

		//property->setStatusTip(desc);
		//property->setToolTip(desc);
	}

	//Request initial value
	updateProperty(var_name);
	return property;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::doublePropertyChanged(QtProperty* property, double value) {
	if (initialValueHack) return;

	emit doubleChanged(propertyName[property],value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::enumPropertyChanged(QtProperty* property, int value) {
	if (initialValueHack) return;

	emit stringChanged(propertyName[property],enumManager->stringValue(property));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::booleanPropertyChanged(QtProperty* property, bool value) {
	if (initialValueHack) return;

	if (value) {
		emit doubleChanged(propertyName[property],1.0);
	} else {
		emit doubleChanged(propertyName[property],0.0);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::stringPropertyChanged(QtProperty* property, const QString& value) {
	if (initialValueHack) return;

	emit stringChanged(propertyName[property],value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::dateTimePropertyChanged(QtProperty* property, const QDateTime &value) {
	if (initialValueHack) return;

	emit stringChanged(propertyName[property],value.toString(Qt::ISODate));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::setDouble(const QString& name, double value) {
	if (!propertyByName.contains(name)) return;

	if (propertyType[propertyByName[name]] == "double") {
		doubleManager->setValue(propertyByName[name],value);
		//doubleManager->properties()[
	} else {
		booleanManager->setValue(propertyByName[name],value > 0.5);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::setHiddenDouble(const QString& name, double value) {
	if (!propertyByName.contains(name)) return;
	doubleManager->setHiddenValue(propertyByName[name],value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::updateProperty(const QString& name) {
	if (!propertyByName.contains(name)) return;
	initialValueHack = 1;
	emit propertyUpdate(name);
	initialValueHack = 0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::setString(const QString& name, const QString& value) {
	if (!propertyByName.contains(name)) return;
	enumManager->setStringValue(propertyByName[name],value);
	stringManager->setValue(propertyByName[name],value);
	datetimeManager->setValue(propertyByName[name],QDateTime::fromString(value,Qt::ISODate));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::hackPitchYawRoll(double* p, double *y, double *r) {
	*p = doubleManager->value(propertyByName["@4"]);
	*y = doubleManager->value(propertyByName["@5"]);
	*r = doubleManager->value(propertyByName["@6"]);
}
