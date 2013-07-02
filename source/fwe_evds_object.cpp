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
//#include <QtGui>
#include <QString>
#include <math.h>

#include "fwe_evds.h"
#include "fwe_evds_object.h"
#include "fwe_evds_object_renderer.h"
#include "fwe_evds_object_csection.h"
#include "fwe_evds_modifiers.h"
#include "fwe_prop_sheet.h"

using namespace EVDS;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Object::Object(EVDS_OBJECT* in_object, EVDS::Object* in_parent, EVDS::Editor* in_editor) {
	//Get C pointer
	object = in_object;
	editor = in_editor;
	parent = in_parent;

	//Is object initialized
	int initialized;
	EVDS_Object_IsInitialized(object,&initialized);

	//Store this object as EVDS userdata
	if (!initialized) EVDS_Object_SetUserdata(object,(void*)this);

	//Create a renderer for the object before children are created
	renderer = 0;
	if ((!initialized) && (in_parent)) {
		QTime time; time.start();

		renderer = new ObjectRenderer(this);
		editor->updateObject(this);

		int elapsed = time.elapsed();
		if (elapsed > 40) {
			qDebug("Object::Object: Long generation: %s (%d msec)",getName().toAscii().data(),elapsed);
		}
	}

	//Enumerate and store all children
	if (!initialized) invalidateChildren();

	//No property sheet by default
	property_sheet = 0;

	//No cross-sections editor
	csection_editor = 0;

	//Create editor-wide unique identifier
	static int editor_uid_counter = 0;
	editor_uid = editor_uid_counter++;

	//Create initial renderer data
	if (renderer) {
		renderer->meshChanged();
		renderer->positionChanged();

		//Add to modifiers
		if (editor) editor->getModifiersManager()->objectAdded(this);
	}
}


////////////////////////////////////////////////////////////////////////////////d
/// @brief
////////////////////////////////////////////////////////////////////////////////
Object::~Object() {
	if (editor) editor->getModifiersManager()->objectRemoved(this);
	if (editor && (editor->getSelected() == this)) editor->clearSelection();
	for (int i = 0; i < children.count(); i++) {
		delete children[i];
	}
	for (int i = 0; i < hidden_children.count(); i++) {
		delete hidden_children[i];
	}
	if (renderer) delete renderer;
	if (property_sheet) property_sheet->deleteLater();
	if (csection_editor) csection_editor->deleteLater();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
int Object::getSelectedCrossSection() {
	if (csection_editor) {
		return csection_editor->getSelectedIndex();
	} else {
		return -1;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Object* Object::insertNewChild(int index) {
	static int object_number = 1;

	//Create new object
	EVDS_SYSTEM* system;
	EVDS_OBJECT* new_object;

	EVDS_Object_GetSystem(object,&system);
	EVDS_Object_Create(system,object,&new_object);
	EVDS_Object_SetName(new_object,tr("New object %1").arg(object_number++).toAscii().data());

	//Find head for this new object
	int idx = 0;
	SIMC_LIST* list;
	SIMC_LIST_ENTRY* entry;
	EVDS_OBJECT* head = 0;

	//Find object in list
	EVDS_Object_GetAllChildren(object,&list);
	entry = SIMC_List_GetFirst(list);
	while (entry && (idx <= index)) {
		if (idx == index) break;
		head = (EVDS_OBJECT*)SIMC_List_GetData(list,entry);

		idx++;
		entry = SIMC_List_GetNext(list,entry);
	}
	SIMC_List_Stop(list,entry);

	//Move object into correct position in EVDS internal list
	if (entry) EVDS_Object_MoveInList(new_object,head);

	//Create object for it and add it to the children
	Object* new_object_obj = new Object(new_object,this,editor);
	children.insert(index,new_object_obj);
	return new_object_obj;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
Object* Object::appendHiddenChild() {
	//Create new object
	EVDS_SYSTEM* system;
	EVDS_OBJECT* new_object;

	EVDS_Object_GetSystem(object,&system);
	EVDS_Object_Create(system,object,&new_object);

	//Create object for it
	Object* new_object_obj = new Object(new_object,this,editor);
	hidden_children.insert(0,new_object_obj);
	return new_object_obj;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::hideChild(int index) {
	if (index < 0) return;
	if (index >= children.count()) return;

	Object* child = children.at(index);
	children.removeAt(index);
	hidden_children.insert(0,child);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
int FWE_InsertChild_OnLoadObject(EVDS_OBJECT_LOADEX* info, EVDS_OBJECT* object) {
	Object* parent = (Object*)info->userdata;
	return EVDS_OK;
}

Object* Object::insertChild(int index, const QString &description) {
	EVDS_OBJECT_LOADEX info = { 0 };
	info.OnLoadObject = &FWE_InsertChild_OnLoadObject;
	info.flags = EVDS_OBJECT_LOADEX_SKIP_MODIFIERS | 
				 EVDS_OBJECT_LOADEX_ONLY_FIRST;
	info.userdata = this;

	//Load description
	info.description = (char*)malloc(description.toUtf8().count()+1);
	memcpy(info.description,description.toUtf8().data(),description.toUtf8().count());
	info.description[description.toUtf8().count()] = 0;

	//Load objects
	EVDS_SYSTEM* system;
	EVDS_Object_GetSystem(object,&system);
	EVDS_Object_LoadEx(object,0,&info);

	EVDS_OBJECT* new_object = info.firstObject;
	free(info.description);

	//Bail out on load failure
	if (!info.firstObject) {
		return insertNewChild(index);
	}

	//Find head for this new object
	int idx = 0;
	SIMC_LIST* list;
	SIMC_LIST_ENTRY* entry;
	EVDS_OBJECT* head = 0;

	//Find object in list
	EVDS_Object_GetAllChildren(object,&list);
	entry = SIMC_List_GetFirst(list);
	while (entry && (idx <= index)) {
		if (idx == index) break;
		head = (EVDS_OBJECT*)SIMC_List_GetData(list,entry);

		idx++;
		entry = SIMC_List_GetNext(list,entry);
	}
	SIMC_List_Stop(list,entry);

	//Move object into correct position in EVDS internal list
	if (entry) EVDS_Object_MoveInList(new_object,head);

	//Create object for it and add it to the children
	Object* new_object_obj = new Object(new_object,this,editor);
	children.insert(index,new_object_obj);
	return new_object_obj;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::removeChild(int index) {
	if (index < 0) return;
	if (index >= children.count()) return;

	Object* child = children.at(index);
	if (child) {
		EVDS_Object_Destroy(child->object);
	}
	children.removeAt(index);
	delete child;
	editor->updateObject(NULL);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::invalidateChildren() {
	//Delete all previous children entries
	for (int i = 0; i < children.count(); i++) delete children[i];

	//Re-build children list
	SIMC_LIST* list;
	EVDS_Object_GetAllChildren(object,&list);
	SIMC_LIST_ENTRY* entry = SIMC_List_GetFirst(list);
	while (entry) {
		EVDS_OBJECT* child = (EVDS_OBJECT*)SIMC_List_GetData(list,entry);

		//Create new object for it
		Object* child_obj = new Object(child,this,editor);
		children.append(child_obj);

		entry = SIMC_List_GetNext(list,entry);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QString Object::getName() {
	char name[257] = { 0 };
	EVDS_Object_GetName(object,name,256);
	return QString(name);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::setName(const QString &name) {
	editor->setModified();
	EVDS_Object_SetName(object,name.toUtf8().data());
	update(false);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QString Object::getType() {
	char type[257] = { 0 };
	EVDS_Object_GetType(object,type,256);
	return QString(type);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::setType(const QString &type) {
	editor->setModified();
	EVDS_Object_SetType(object,type.toUtf8().data());
	update(false);

	editor->getModifiersManager()->modifierChanged(this);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::meshReady() {
	update(false);
}

/*void Object::draw(bool objectSelected) {
	if (!renderer) {
		renderer = new ObjectRenderer(this);
		connect(renderer, SIGNAL(signalHQMeshReady()), this, SLOT(meshReady()));
	}
	renderer->drawMesh(objectSelected);
}*/


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QWidget* Object::getPropertySheet() {
	if (property_sheet) {
		return property_sheet;
	} else {
		property_sheet = new FWEPropertySheet();

		//Connect up signals and slots
		connect(property_sheet, SIGNAL(doubleChanged(const QString&, double)),
				this, SLOT(doubleChanged(const QString&, double)));
		connect(property_sheet, SIGNAL(stringChanged(const QString&, const QString&)),
				this, SLOT(stringChanged(const QString&, const QString&)));
		connect(property_sheet, SIGNAL(propertyUpdate(const QString&)),
				this, SLOT(propertyUpdate(const QString&)));

		//Create default set of properties FIXME: make it less of a hack
		if ((getType() != "metadata") &&
			(getType().mid(0,8) != "foxworks")) {
			property_sheet->setProperties(editor->objectVariables[""]);
		}
		if (getType().mid(0,20) == "foxworks.schematics.") {
			property_sheet->setProperties(editor->objectVariables["foxworks.schematics."]);
		}
		if (!getType().isEmpty()) property_sheet->setProperties(editor->objectVariables[getType()]);
		return property_sheet;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::doubleChanged(const QString& name, double value) {
	setVariable(name,value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::stringChanged(const QString& name, const QString& value) {
	setVariable(name,value);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::propertyUpdate(const QString& name) {
	if (name == "@7") { //Special: type
		property_sheet->setString(name,getType());
	} else { //Other variable
		property_sheet->setString(name,getString(name));
		property_sheet->setDouble(name,getVariable(name));
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::setVariable(const QString &name, double value) {
	editor->setModified();

	if (name[0] == '@') {
		int specialIndex = name.right(1).toInt();
		EVDS_STATE_VECTOR vector;
		EVDS_REAL pitch,yaw,roll;
		EVDS_Object_GetStateVector(object,&vector);

		property_sheet->hackPitchYawRoll(&pitch,&yaw,&roll); //Get PYR from UI
		pitch = EVDS_RAD(pitch);
		yaw = EVDS_RAD(yaw);
		roll = EVDS_RAD(roll);

		switch (specialIndex) {
			case 1: vector.position.x = value; break;
			case 2: vector.position.y = value; break;
			case 3: vector.position.z = value; break;
			case 4: pitch = EVDS_RAD(value); break;
			case 5: yaw = EVDS_RAD(value); break;
			case 6: roll = EVDS_RAD(value); break;
			case 8: EVDS_Object_SetUID(object,(unsigned int)value); break;
		}
		EVDS_Quaternion_FromEuler(&vector.orientation,vector.orientation.coordinate_system,roll,pitch,yaw);
		EVDS_Object_SetStateVector(object,&vector);
		update(false);
	} else {
		EVDS_VARIABLE* variable;
		EVDS_Object_AddRealVariable(object,name.toAscii().data(),value,&variable);
		EVDS_Variable_SetReal(variable,value);
		if ((name == "disable") ||
			(name == "mass") ||
			(name == "ixx") ||
			(name == "iyy") ||
			(name == "izz") ||
			(name == "jxx") ||
			(name == "jyy") ||
			(name == "jzz")) {
			update(false);
		} else {
			update(true);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::setVariable(const QString &name, const QString &value) {
	editor->setModified(name != "comments");

	if (name[0] == '@') {
		int specialIndex = name.right(1).toInt();
		if (specialIndex == 7) {
			setType(value); //Change type
			if (property_sheet) { //Update property sheet
				QWidget* prev_sheet = property_sheet;
				property_sheet = 0;
				editor->propertySheetUpdated(prev_sheet,getPropertySheet());
				prev_sheet->deleteLater();
			}
			update(true);
			return;
		}
	} else {
		EVDS_VARIABLE* variable;
		if (EVDS_Object_AddVariable(object,name.toAscii().data(),EVDS_VARIABLE_TYPE_STRING,&variable) == EVDS_OK) {
			EVDS_Variable_SetString(variable,value.toAscii().data(),value.toAscii().count());
		}
		if (name != "comments") update(true);
	}
	//update(false);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
double Object::getVariable(const QString &name) {
	if (name[0] == '@') {
		int specialIndex = name.right(1).toInt();
		unsigned int uid;
		EVDS_STATE_VECTOR vector;
		EVDS_REAL pitch,yaw,roll;
		EVDS_Object_GetStateVector(object,&vector);
		EVDS_Quaternion_ToEuler(&vector.orientation,vector.orientation.coordinate_system,&roll,&pitch,&yaw);
		EVDS_Object_GetUID(object,&uid);
		switch (specialIndex) {
			case 1: return vector.position.x; break;
			case 2: return vector.position.y; break;
			case 3: return vector.position.z; break;
			case 4: return EVDS_DEG(pitch); break;
			case 5: return EVDS_DEG(yaw); break;
			case 6: return EVDS_DEG(roll); break;
			case 8: return uid; break;
		}
	} else {
		EVDS_VARIABLE* variable;
		if (EVDS_Object_GetVariable(object,name.toUtf8().data(),&variable) == EVDS_OK) {
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
QString Object::getString(const QString &name) {
	EVDS_VARIABLE* variable;
	if (EVDS_Object_GetVariable(object,name.toUtf8().data(),&variable) == EVDS_OK) {
		char str[8192] = { 0 };
		EVDS_Variable_GetString(variable,str,8191,0);
		return QString(str);
	}
	return QString();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QVector3D Object::getVector(const QString &name) {
	EVDS_VARIABLE* variable;
	if (EVDS_Object_GetVariable(object,name.toUtf8().data(),&variable) == EVDS_OK) {
		EVDS_VECTOR value;
		EVDS_Variable_GetVector(variable,&value);
		return QVector3D(value.x,value.y,value.z);
	}
	return QVector3D();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool Object::isVariableDefined(const QString &name) {
	EVDS_VARIABLE* variable;
	return EVDS_Object_GetVariable(object,name.toUtf8().data(),&variable) == EVDS_OK;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool Object::isOxidizerTank() {
	QString fuel_type = getString("fuel_type");
	if (fuel_type != "") {
		EVDS_SYSTEM* system;
		EVDS_VARIABLE* database;
		EVDS_VARIABLE* material;
		EVDS_Object_GetSystem(object,&system);

		if ((EVDS_System_GetDatabaseByName(system,"material",&database) == EVDS_OK) &&
			(EVDS_Variable_GetNested(database,fuel_type.toAscii().data(),&material) == EVDS_OK)) {
			char class_str[256] = { 0 };
			EVDS_VARIABLE* attribute;
			if (EVDS_Variable_GetAttribute(material,"class",&attribute) == EVDS_OK) {
				EVDS_Variable_GetString(attribute,class_str,255,0);
				if (QString(class_str) == "oxidizer") return true;
			}
		}
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QWidget* Object::getCrossSectionsEditor() {
	if (csection_editor) {
		return csection_editor;
	} else {
		csection_editor = new CrossSectionEditor(this);
		return csection_editor;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::deleteCrossSectionsEditor() {
	if (csection_editor) {
		csection_editor->deleteLater();
		csection_editor = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::update(bool visually) {
	if (getType() == "metadata") return; //Do not do any updates for metadata

	if (renderer) {
		if (visually) {
			renderer->meshChanged();
			if (getType() == "modifier") editor->getModifiersManager()->modifierChanged(this);
		} else {
			renderer->positionChanged();
			editor->getModifiersManager()->objectPositionChanged(this);
		}
	}
	//if (visually && renderer) renderer->meshChanged();
	editor->updateObject(this);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Object::recursiveUpdateInformation(ObjectInitializer* initializer) {
	//Update information about the current object
	TemporaryObject* temporary_object = initializer->getObject(this);
	EVDS_OBJECT* evds_object = temporary_object->getEVDSObject();

	SIMC_LIST* list;
	SIMC_LIST_ENTRY* entry;

	//Scan all variables
	EVDS_Object_GetVariables(evds_object,&list);
	entry = SIMC_List_GetFirst(list);
	while (entry) {
		EVDS_VARIABLE* variable = (EVDS_VARIABLE*)SIMC_List_GetData(list,entry);

		//Get name and type to fill out info_vectors/info_variables arrays
		char name_str[65] = { 0 };
		EVDS_VARIABLE_TYPE type;
		EVDS_Variable_GetName(variable,name_str,64);
		EVDS_Variable_GetType(variable,&type);

		QString name(name_str);
		if (type == EVDS_VARIABLE_TYPE_FLOAT) { //Floating point var
			info_variables[name] = temporary_object->getVariable(name);
		} else if (type == EVDS_VARIABLE_TYPE_VECTOR) { //Vector var
			info_vectors[name] = temporary_object->getVector(name);
		}

		info_defined[name] = temporary_object->isVariableDefined(name);
		entry = SIMC_List_GetNext(list,entry);
	}

	delete temporary_object;

	//Update information for all children
	for (int i = 0; i < getChildrenCount(); i++) {
		getChild(i)->recursiveUpdateInformation(initializer);
	}
}

double Object::getInformationVariable(const QString &name) {
	if (!info_variables.contains(name)) return 0.0;
	return info_variables[name];
}

QVector3D Object::getInformationVector(const QString &name) {
	if (!info_vectors.contains(name)) return QVector3D();
	return info_vectors[name];
}

bool Object::isInformationDefined(const QString &name) {
	if (!info_defined.contains(name)) return false;
	return info_defined[name];
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ObjectInitializer::ObjectInitializer(Object* in_object) {
	object = in_object;

	//Initialize temporary object
	object_copy = 0;

	//Temporary hack to check in_object for validity
	in_object->getType(); //Will crash when in_object is invalid

	//Delete thread when work is finished
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));	
	connect(&updateCallTimer, SIGNAL(timeout()), this, SLOT(doUpdateObject()));
	doStopWork = false;
	needObject = false; 
	objectCompleted = true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
TemporaryObject* ObjectInitializer::getObject(Object* object) {
	while (!objectCompleted) msleep(1); //Wait until object initialization is completed

	readingLock.lock();
	EVDS_OBJECT* found_object = 0;
	EVDS_SYSTEM* system;
	EVDS_Object_GetSystem(object_copy,&system);
	if (EVDS_System_GetObjectByUID(system,object->getEditorUID(),object_copy,&found_object) != EVDS_OK) {
		qWarning("ObjectInitializer::getObject: could not find object");
		return new TemporaryObject(object_copy,0);
	}
	return new TemporaryObject(found_object,&readingLock);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Get a temporary copy of the object
///
/// Must be called before the first call of getObject!
////////////////////////////////////////////////////////////////////////////////
void ObjectInitializer::doUpdateObject() {
	updateCallTimer.stop();
	//qDebug("ObjectInitializer::doUpdateObject: fire!");
	needObject = true;
	if (this->isRunning()) {
		readingLock.lock();
			//Destroy old copy of initialized object
			if (object_copy) EVDS_Object_Destroy(object_copy);
			//Create new one
			EVDS_Object_Copy(object->getEVDSObject(),0,&object_copy);
			//Object not completed
			objectCompleted = false;
		readingLock.unlock();
	}
}

void ObjectInitializer::updateObject() {
	//qDebug("ObjectInitializer::updateObject: start timer");
	updateCallTimer.start(200);
	//qDebug("ObjectInitializer::updateObject: requested update");
	
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void FWE_ObjectInitializer_FixUIDs(EVDS_OBJECT* object) {
	//Get userdata
	void* userdata;
	EVDS_Object_GetUserdata(object,&userdata);
	Object* editor_object = static_cast<Object*>(userdata);

	if (!editor_object) {
		qDebug("FWE_ObjectInitializer_FixUIDs: for some reason no UID");
		return;
	}

	//Set UID
	EVDS_Object_SetUID(object,editor_object->getEditorUID());

	//Get list of children
	SIMC_LIST* list;
	SIMC_LIST_ENTRY* entry;
	EVDS_Object_GetAllChildren(object,&list);

	//Do same for every child
	entry = SIMC_List_GetFirst(list);
	while (entry) {
		FWE_ObjectInitializer_FixUIDs((EVDS_OBJECT*)SIMC_List_GetData(list,entry));
		entry = SIMC_List_GetNext(list,entry);
	}
}

void ObjectInitializer::stopWork() {
	doStopWork = true;
	while (isRunning()) {
		msleep(10);
		//QApplication::processEvents();
	}
}

void ObjectInitializer::run() {
	msleep(2000); //Give enough time for the rest of application to initialize
	while (!object_copy && (!doStopWork)) msleep(100); //Wait until there's an object to initialize
	while (!doStopWork) {
		if (needObject) {
			readingLock.lock();
				//Start making the mesh
				needObject = false;

				//Transfer and initialize object
				//qDebug("ObjectInitializer::run: initializing...");
				EVDS_Object_TransferInitialization(object_copy); //Get rights to work with variables
				FWE_ObjectInitializer_FixUIDs(object_copy); //Fix UID's for the objects
				EVDS_Object_Initialize(object_copy,1);
				EVDS_Object_Solve(object_copy,0.0);
				//qDebug("ObjectInitializer::run: done!");

				//Finish working
				objectCompleted = true;
			readingLock.unlock();

			//If new mesh is needed, do not return generated one - return actually needed one instead
			if (!needObject) {
				emit signalObjectReady();
			}
		}
		msleep(50);
	}

	//Remove object
	//qDebug("ObjectInitializer::run: stopped");
	//if (object_copy) EVDS_Object_Destroy(object_copy);
}
