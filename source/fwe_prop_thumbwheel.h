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
