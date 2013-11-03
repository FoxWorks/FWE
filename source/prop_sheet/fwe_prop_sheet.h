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
