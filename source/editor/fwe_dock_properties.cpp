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
#include <QDockWidget>
#include <QStackedLayout>

#include "fwe_dock_properties.h"

using namespace EVDS;
using namespace Dock;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Properties::Properties(QWidget* parent) : QDockWidget(tr("Object Properties"),parent) {
	//Set dock properties
	setFeatures(QDockWidget::AllDockWidgetFeatures);
	setAllowedAreas(Qt::AllDockWidgetAreas);

	//Create form and stacked layout
	form = new QWidget();
	form->setMinimumWidth(250);
	form->setMinimumHeight(80);
	setWidget(form);

	//Create layout
	layout = new QStackedLayout;
	form->setLayout(layout);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Properties::setPropertySheet(QWidget* sheet) {
	if (layout->indexOf(sheet) < 0) {
		layout->addWidget(sheet);
	}
	layout->setCurrentWidget(sheet);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Properties::removePropertySheet(QWidget* sheet) {
	layout->removeWidget(sheet);
}