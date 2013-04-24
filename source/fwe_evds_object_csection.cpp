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
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QTabBar>
#include <math.h>
#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_csection.h"
#include "fwe_prop_sheet.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
CrossSectionEditor::CrossSectionEditor(Object* in_object) {
	object = in_object;
	editor = object->getEditor();

	//Create cross-section editor
	layout = new QVBoxLayout;
	setLayout(layout);

	//Create section selection bar
	tab = new QTabBar(this);
	tab->setTabsClosable(true);
	tab->setMovable(true);
	tab->setShape(QTabBar::RoundedNorth);
	layout->addWidget(tab);

	//Setup tab signals
	connect(tab,SIGNAL(currentChanged(int)),this,SLOT(sectionSelected(int)));
	connect(tab,SIGNAL(tabCloseRequested(int)),this,SLOT(sectionDeleted(int)));
	connect(tab,SIGNAL(tabMoved(int,int)),this,SLOT(sectionMoved(int,int)));

	//Create section interface
	sections = new QStackedWidget(this);
	layout->addWidget(sections);

	//Initialize cross-section information
	SIMC_LIST_ENTRY* entry;
	SIMC_LIST* csections_list;
	EVDS_VARIABLE* csection;

	geometry = 0;
	if (EVDS_Object_GetVariable(object->getEVDSObject(),"csection_geometry",&geometry) != EVDS_OK) {
		EVDS_Object_AddVariable(object->getEVDSObject(),"csection_geometry",EVDS_VARIABLE_TYPE_NESTED,&geometry);
	}
	EVDS_Variable_GetList(geometry,&csections_list);

	//Traverse list for initial cross-section information
	int index = 0;
	entry = SIMC_List_GetFirst(csections_list);
	while (entry) {
		csection = (EVDS_VARIABLE*)SIMC_List_GetData(csections_list,entry);

		//Create widget
		CrossSection* csection_widget = new CrossSection(csection,this);
		csectionWidgetByIndex[index] = csection_widget;
		indexByCSectionWidget[csection_widget] = index;
		sections->addWidget(csection_widget);
		tab->addTab(tr("%1").arg(index));

		index++;
		entry = SIMC_List_GetNext(csections_list,entry);
	}
	tab->addTab("<..new..>");
	//object->update(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
CrossSectionEditor::~CrossSectionEditor() {

}


int CrossSectionEditor::getSelectedIndex() {
	return sections->currentIndex();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSectionEditor::sectionSelected(int index) {
	if (index == sections->count()) { //Create new section
		EVDS_VARIABLE* csection;
		EVDS_VARIABLE* type;
		EVDS_Variable_AddNested(geometry,"section",EVDS_VARIABLE_TYPE_NESTED,&csection);
		EVDS_Variable_AddAttribute(csection,"type",EVDS_VARIABLE_TYPE_STRING,&type);
		EVDS_Variable_SetString(type,"ellipse",8);
		EVDS_Variable_AddAttribute(csection,"offset",EVDS_VARIABLE_TYPE_FLOAT,&type);
		EVDS_Variable_SetReal(type,0.0);
		EVDS_Variable_AddAttribute(csection,"add_offset",EVDS_VARIABLE_TYPE_FLOAT,&type);
		EVDS_Variable_SetReal(type,1.0);

		//Create widget
		CrossSection* csection_widget = new CrossSection(csection,this);
		csectionWidgetByIndex[index] = csection_widget;
		indexByCSectionWidget[csection_widget] = index;
		sections->addWidget(csection_widget);
		tab->setTabText(index,tr("%1").arg(index));
		tab->addTab("<..new..>");

		//Update geometry
		object->update(true);
		object->getEditor()->setModified();
	}
	sections->setCurrentIndex(index);
	object->update(false);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSectionEditor::sectionDeleted(int index) {
	if (index == sections->count()) return;
	if (sections->count() == 1) return;

	CrossSection* csection_widget = csectionWidgetByIndex[index];
	csectionWidgetByIndex.remove(index);
	for (int i = index; i < sections->count()-1; i++) {
		csectionWidgetByIndex[i] = csectionWidgetByIndex[i+1];
		indexByCSectionWidget[csectionWidgetByIndex[i]] = i;
	}

	EVDS_VARIABLE* csection = csection_widget->getEVDSCrossSection();
	EVDS_Variable_Destroy(csection);

	tab->removeTab(index);
	sections->removeWidget(sections->widget(index));

	if ((index == sections->count()) && (index > 1)) {
		tab->setCurrentIndex(index-1);
	}

	//Update geometry
	object->update(true);
	object->getEditor()->setModified();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSectionEditor::sectionMoved(int from, int to) {
	if (to == sections->count()) sectionSelected(to);
	if (from == sections->count()) sectionSelected(from);

	CrossSection* csection_widget_from = csectionWidgetByIndex[from];
	CrossSection* csection_widget_to = csectionWidgetByIndex[to];

	csectionWidgetByIndex[to] = csection_widget_from;
	csectionWidgetByIndex[from] = csection_widget_to;
	indexByCSectionWidget[csection_widget_from] = to;
	indexByCSectionWidget[csection_widget_to] = from;

	EVDS_VARIABLE* csection_to = csection_widget_to->getEVDSCrossSection();
	EVDS_VARIABLE* csection_from = csection_widget_from->getEVDSCrossSection();

	if (from > to) {
		sections->removeWidget(csection_widget_from);
		sections->insertWidget(to,csection_widget_from);
		EVDS_Variable_MoveInList(csection_to,csection_from);
	} else {
		sections->removeWidget(csection_widget_from);
		sections->insertWidget(to,csection_widget_from);
		EVDS_Variable_MoveInList(csection_from,csection_to);
	}

	tab->setTabText(from,tr("%1").arg(from));
	tab->setTabText(to,tr("%1").arg(to));

	//Update geometry
	object->update(true);
	object->getEditor()->setModified();
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
CrossSection::CrossSection(EVDS_VARIABLE* in_cross_section, CrossSectionEditor* in_editor) {
	cross_section = in_cross_section;
	editor = in_editor;

	//Create layout
	layout = new QVBoxLayout;
	layout->setMargin(0);
	setLayout(layout);

	//Create property sheet
	createPropertySheet();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
CrossSection::~CrossSection() {

}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSection::createPropertySheet() {
	property_sheet = new FWPropertySheet(this);
	layout->addWidget(property_sheet);

	//Connect up signals and slots
	connect(property_sheet, SIGNAL(doubleChanged(const QString&, double)),
			this, SLOT(doubleChanged(const QString&, double)));
	connect(property_sheet, SIGNAL(enumChanged(const QString&, const QString&)),
			this, SLOT(enumChanged(const QString&, const QString&)));
	connect(property_sheet, SIGNAL(propertyUpdate(const QString&)),
			this, SLOT(propertyUpdate(const QString&)));

	//Create default set of properties
	property_sheet->setProperties(editor->getEVDSEditor()->csectionVariables[""]);
	property_sheet->setProperties(editor->getEVDSEditor()->csectionVariables[getType()]);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QString CrossSection::getType() {
	EVDS_VARIABLE* type_var = 0;
	char type[257] = { 0 };
	if (EVDS_Variable_GetAttribute(cross_section,"type",&type_var) == EVDS_OK) {	
		EVDS_Variable_GetString(type_var,type,256,0);
		return QString(type);
	} else {
		return "";
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSection::setVariable(const QString &name, double value) {
	if (name[0] == '@') {
		int specialIndex = name.right(1).toInt();
		EVDS_VARIABLE* pv = 0;
		EVDS_VARIABLE* mv = 0;
		//if (value == 0.0) return;
		switch (specialIndex) {
			case 1: {
				EVDS_Variable_AddAttribute(cross_section,"tangent_p_offset",EVDS_VARIABLE_TYPE_FLOAT,&pv);
				EVDS_Variable_AddAttribute(cross_section,"tangent_m_offset",EVDS_VARIABLE_TYPE_FLOAT,&mv);
				EVDS_Variable_SetReal(pv,value);
				EVDS_Variable_SetReal(mv,value);
				property_sheet->updateProperty("tangent_p_offset");
				property_sheet->updateProperty("tangent_m_offset");
			} break;
			case 2: {
				EVDS_Variable_AddAttribute(cross_section,"tangent_p_radial",EVDS_VARIABLE_TYPE_FLOAT,&pv);
				EVDS_Variable_AddAttribute(cross_section,"tangent_m_radial",EVDS_VARIABLE_TYPE_FLOAT,&mv);
				EVDS_Variable_SetReal(pv,value);
				EVDS_Variable_SetReal(mv,value);
				property_sheet->updateProperty("tangent_p_radial");
				property_sheet->updateProperty("tangent_m_radial");
			} break;
		}
	} else {
		//Ordinary logic
		EVDS_VARIABLE* v;
		EVDS_REAL prev_value;
		if (EVDS_Variable_AddAttribute(cross_section,name.toAscii().data(),EVDS_VARIABLE_TYPE_FLOAT,&v) == EVDS_OK) {
			EVDS_Variable_GetReal(v,&prev_value);
			EVDS_Variable_SetReal(v,value);
		}

		//Special logic
		if (name == "tangent_p_offset")	property_sheet->updateProperty("\x01");
		if (name == "tangent_m_offset")	property_sheet->updateProperty("\x01");
		if (name == "tangent_p_radial")	property_sheet->updateProperty("\x02");
		if (name == "tangent_m_radial")	property_sheet->updateProperty("\x02");
		if (name == "rx") {
			EVDS_REAL ry;
			if (EVDS_Variable_GetAttribute(cross_section,"ry",&v) == EVDS_OK) {
				EVDS_Variable_GetReal(v,&ry);
				if (prev_value == ry) EVDS_Variable_SetReal(v,value);
				property_sheet->updateProperty("ry");
			}
		}
	}
	editor->getObject()->update(true);
	editor->getObject()->getEditor()->setModified();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSection::setVariable(const QString &name, const QString &value) {
	if (name[0] == '@') {
		int specialIndex = name.right(1).toInt();
		if (specialIndex == 3) {
			EVDS_VARIABLE* v; //Change type
			if (EVDS_Variable_AddAttribute(cross_section,"type",EVDS_VARIABLE_TYPE_STRING,&v) == EVDS_OK) {
				EVDS_Variable_SetString(v,value.toAscii().data(),strlen(value.toAscii().data())+1);
			}

			//Update property sheet
			if (property_sheet) {
				//QWidget* prev_sheet = property_sheet;
				//prev_sheet->deleteLater();
				property_sheet->deleteLater();
				createPropertySheet();
				//editor->propertySheetUpdated(prev_sheet,property_sheet);
			}
			editor->getObject()->update(true);
			return;
		}
	} else {
		//FIXME
	}
	editor->getObject()->update(true);
	editor->getObject()->getEditor()->setModified();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
double CrossSection::getVariable(const QString &name) {
	if (name[0] == '@') {
		int specialIndex = name.right(1).toInt();
		EVDS_VARIABLE* pv = 0;
		EVDS_VARIABLE* mv = 0;
		EVDS_REAL p = 0.0;
		EVDS_REAL m = 0.0;
		switch (specialIndex) {
			case 1: {
				EVDS_Variable_GetAttribute(cross_section,"tangent_p_offset",&pv);
				EVDS_Variable_GetAttribute(cross_section,"tangent_m_offset",&mv);
			} break;
			case 2: {
				EVDS_Variable_GetAttribute(cross_section,"tangent_p_radial",&pv);
				EVDS_Variable_GetAttribute(cross_section,"tangent_m_radial",&mv);
			} break;
		}

		EVDS_Variable_GetReal(pv,&p);
		EVDS_Variable_GetReal(mv,&m);

		if (fabs(p-m) < 1e-9) {
			return p;
		} else {
			return 0.0;
		}
	} else {
		EVDS_VARIABLE* variable;
		int error_code = EVDS_Variable_GetAttribute(cross_section,name.toAscii().data(),&variable);
		if (error_code == EVDS_OK) {
			//Special hack for rx/ry
			if ((error_code == EVDS_ERROR_NOT_FOUND) && (name == "ry")) 
				EVDS_Variable_AddAttribute(cross_section,"rx",EVDS_VARIABLE_TYPE_FLOAT,&variable);
			//Special hack for tangent_p_offset/tangent_m_offset
			//if ((error_code == EVDS_ERROR_NOT_FOUND) && (name == "tangent_m_offset")) 
				//EVDS_Variable_AddAttribute(cross_section,"tangent_p_offset",EVDS_VARIABLE_TYPE_FLOAT,&variable);
			//Special hack for tangent_p_radial/tangent_m_radial
			//if ((error_code == EVDS_ERROR_NOT_FOUND) && (name == "tangent_m_radial")) 
				//EVDS_Variable_AddAttribute(cross_section,"tangent_p_radial",EVDS_VARIABLE_TYPE_FLOAT,&variable);

			EVDS_REAL value;
			EVDS_Variable_GetReal(variable,&value);
			return value;
		}
	}
	return 0.0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSection::doubleChanged(const QString& name, double value) {
	setVariable(name,value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSection::enumChanged(const QString& name, const QString& value) {
	setVariable(name,value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void CrossSection::propertyUpdate(const QString& name) {
	if (name == "@3") { //Special: type
		property_sheet->setEnum(name,getType());
	} else { //Other variable
		property_sheet->setDouble(name,getVariable(name));
	}
}