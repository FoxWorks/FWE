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
#ifndef FWE_PROP_SHEET_H
#define FWE_PROP_SHEET_H

#include <QList>
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "qttreepropertybrowser.h"

class ThumbWheelPropertyManager;
class ThumbWheelFactory;
class StringEnumPropertyManager;
class StringEnumFactory;

class FWEPropertySheet : public QtTreePropertyBrowser 
{
	Q_OBJECT

public:
	FWEPropertySheet(QWidget* parent = NULL);
	~FWEPropertySheet();

public:
	QtProperty* setProperty(const QMap<QString,QString> &data);
	void setProperties(const QList<QMap<QString,QString> >& list);
	void setDouble(const QString& name, double value);
	void setHiddenDouble(const QString& name, double value);
	void setString(const QString& name, const QString& value);
	void updateProperty(const QString& name);
	void hackPitchYawRoll(double* p, double *y, double *r);

signals:
	/// Called when a double property value is changed by user
	void doubleChanged(const QString& name, double value);
	/// Called when an enumeration value is changed by user
	void stringChanged(const QString& name, const QString& value);
	/// Called value displayed must be updated
	void propertyUpdate(const QString& name);

private slots:
	void doublePropertyChanged(QtProperty* property, double value);
	void enumPropertyChanged(QtProperty* property, int value);
	void booleanPropertyChanged(QtProperty* property, bool value);
	void stringPropertyChanged(QtProperty* property, const QString& value);
	void dateTimePropertyChanged(QtProperty* property, const QDateTime &value);

private:
	QtGroupPropertyManager *groupManager;
	ThumbWheelPropertyManager *doubleManager;
	ThumbWheelFactory *doubleFactory;
	StringEnumPropertyManager *enumManager;
	StringEnumFactory *enumFactory;
	QtBoolPropertyManager* booleanManager;
	QtCheckBoxFactory *booleanFactory;
	QtStringPropertyManager* stringManager;
	QtLineEditFactory* stringFactory;
	QtDateTimePropertyManager* datetimeManager;
	QtDateTimeEditFactory* datetimeFactory;

	int initialValueHack;
	QMap<QString, QtProperty *> propertyGroupByName;
	QMap<QtProperty *, QString> propertyName;
	QMap<QtProperty *, QString> propertyType;
	QMap<QString, QtProperty *> propertyByName;
};

#endif
