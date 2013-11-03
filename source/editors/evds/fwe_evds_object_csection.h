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
		void stringChanged(const QString& name, const QString& value);
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