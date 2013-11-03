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
#ifndef FWE_PROP_STRINGENUM_H
#define FWE_PROP_STRINGENUM_H

#include "qtpropertymanager.h"
#include "qteditorfactory.h"

class StringEnumPropertyManager : public QtEnumPropertyManager
{
	Q_OBJECT
public:
	StringEnumPropertyManager(QObject *parent = 0);
	~StringEnumPropertyManager();

	QString stringValue(const QtProperty *property) const;
	void setStringValue(QtProperty *property, QString value);

public Q_SLOTS:
	void setEnumValues(QtProperty *property, const QStringList &values);
Q_SIGNALS:
	void enumValuesChanged(QtProperty *property, const QStringList &values);
protected:
	//QString valueText(const QtProperty *property) const;
	virtual void initializeProperty(QtProperty *property);
	virtual void uninitializeProperty(QtProperty *property);
private:
	QMap<const QtProperty *, QStringList> propertyToValues;
};

class StringEnumFactory : public QtAbstractEditorFactory<StringEnumPropertyManager>
{
	Q_OBJECT
public:
	StringEnumFactory(QObject *parent = 0);
	~StringEnumFactory();
protected:
	void connectPropertyManager(StringEnumPropertyManager *manager);
	QWidget *createEditor(StringEnumPropertyManager *manager, QtProperty *property,
				QWidget *parent);
	void disconnectPropertyManager(StringEnumPropertyManager *manager);
private slots:
	void slotEditorDestroyed(QObject *object);

private:
	QtEnumEditorFactory *originalFactory;
	QMap<QtProperty *, QList<QWidget *> > createdEditors;
	QMap<QWidget *, QtProperty *> editorToProperty;
};

#endif
