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
#include <QString>
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

	connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
			this, SLOT(doublePropertyChanged(QtProperty *, double)));
	connect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
			this, SLOT(enumPropertyChanged(QtProperty *, int)));
	connect(booleanManager, SIGNAL(valueChanged(QtProperty *, bool)),
			this, SLOT(booleanPropertyChanged(QtProperty *, bool)));

	//Create browser
	setFactoryForManager(doubleManager, doubleFactory);
	setFactoryForManager(enumManager, enumFactory);
	setFactoryForManager(booleanManager, booleanFactory);
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

	emit enumChanged(propertyName[property],enumManager->stringValue(property));
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
void FWEPropertySheet::setEnum(const QString& name, const QString& value) {
	if (!propertyByName.contains(name)) return;
	enumManager->setStringValue(propertyByName[name],value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWEPropertySheet::hackPitchYawRoll(double* p, double *y, double *r) {
	*p = doubleManager->value(propertyByName["@4"]);
	*y = doubleManager->value(propertyByName["@5"]);
	*r = doubleManager->value(propertyByName["@6"]);
}
