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
#ifndef FWE_EVDS_OBJECT_CSECTION_H
#define FWE_EVDS_OBJECT_CSECTION_H

#include <QWidget>
#include <QMap>
#include "evds.h"

QT_BEGIN_NAMESPACE
class QTabBar;
class QStackedWidget;
class QVBoxLayout;
QT_END_NAMESPACE

class FWEPropertySheet;
namespace EVDS {
	class Editor;
	class Object;
	class CrossSection;
	class CrossSectionEditor : public QWidget
	{
		Q_OBJECT

	public:
		CrossSectionEditor(Object* in_object);
		~CrossSectionEditor();

		Object* getObject() { return object; }
		Editor* getEVDSEditor() { return editor; }
		int getSelectedIndex();

	private slots:
		void sectionSelected(int index);
		void sectionDeleted(int index);
		void sectionMoved(int from, int to);

	private:
		QTabBar* tab;
		QStackedWidget* sections;
		QVBoxLayout* layout;

		Editor* editor;
		Object* object;
		EVDS_VARIABLE* geometry;

		QMap<int, CrossSection*> csectionWidgetByIndex;
		QMap<CrossSection*, int> indexByCSectionWidget;
	};

	class CrossSection : public QWidget
	{
		Q_OBJECT

	public:
		CrossSection(EVDS_VARIABLE* in_cross_section, CrossSectionEditor* in_editor);
		~CrossSection();

		void setVariable(const QString &name, double value);
		void setVariable(const QString &name, const QString &value);
		double getVariable(const QString &name);
		QString getType();

		EVDS_VARIABLE* getEVDSCrossSection() { return cross_section; }

	private slots:
		void doubleChanged(const QString& name, double value);
		void enumChanged(const QString& name, const QString& value);
		void propertyUpdate(const QString& name);

	private:
		void createPropertySheet();

		FWEPropertySheet* property_sheet;
		QVBoxLayout* layout;
		
		CrossSectionEditor* editor;
		EVDS_VARIABLE* cross_section;
	};
}


#endif