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
#ifndef FWE_PROP_THUMBWHEEL_H
#define FWE_PROP_THUMBWHEEL_H

#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include <QtGui/QDoubleSpinBox>

class ThumbWheelPropertyManager : public QtDoublePropertyManager
{
	Q_OBJECT
public:
	ThumbWheelPropertyManager(QObject *parent = 0);
	~ThumbWheelPropertyManager();

	QString prefix(const QtProperty *property) const;
	QString suffix(const QtProperty *property) const;
	bool hideZeroes(const QtProperty *property) const;

public Q_SLOTS:
	void setPrefix(QtProperty *property, const QString &prefix);
	void setSuffix(QtProperty *property, const QString &suffix);
	void setHideZeroes(QtProperty *property, bool value);
	void setHiddenValue(QtProperty *property, double value);
Q_SIGNALS:
	void prefixChanged(QtProperty *property, const QString &prefix);
	void suffixChanged(QtProperty *property, const QString &suffix);
	void hideZeroesChanged(QtProperty *property, bool value);
	void hiddenValueChanged(QtProperty *property, double value);
protected:
	QString valueText(const QtProperty *property) const;
	virtual void initializeProperty(QtProperty *property);
	virtual void uninitializeProperty(QtProperty *property);
private:
	struct Data {
		QString prefix;
		QString suffix;
		bool hideZeroes;
		double hiddenValue;
	};
	QMap<const QtProperty *, Data> propertyToData;
};

class ThumbWheelFactory : public QtAbstractEditorFactory<ThumbWheelPropertyManager>
{
	Q_OBJECT
public:
	ThumbWheelFactory(QObject *parent = 0);
	~ThumbWheelFactory();
protected:
	void connectPropertyManager(ThumbWheelPropertyManager *manager);
	QWidget *createEditor(ThumbWheelPropertyManager *manager, QtProperty *property,
				QWidget *parent);
	void disconnectPropertyManager(ThumbWheelPropertyManager *manager);
private slots:
	void slotPrefixChanged(QtProperty *property, const QString &prefix);
	void slotSuffixChanged(QtProperty *property, const QString &prefix);
	void slotEditorDestroyed(QObject *object);

private:
	QtDoubleSpinBoxFactory *originalFactory;
	QMap<QtProperty *, QList<QWidget *> > createdEditors;
	QMap<QWidget *, QtProperty *> editorToProperty;
};

#endif
