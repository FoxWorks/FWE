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
