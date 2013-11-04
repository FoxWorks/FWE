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
#include <QtGui>

#include "fwe_main.h"
#include "fwe_evds.h"
#include "fwe_schematics.h"
#include "fwe_dialog_preferences.h"

using namespace FWE;


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
EditorWindow::EditorWindow(MainWindow* window) {
	mainWindow = window;
	editorsWidget = new QWidget(this);
	editorsLayout = new QStackedLayout(editorsWidget);
	setCentralWidget(editorsWidget);

	//Create EVDS editor
	EVDSEditor = new EVDS::Editor(this);
	editorsLayout->addWidget(EVDSEditor);
	EVDSEditor->setActive(true);

	//Create schematics editor
	SchematicsEditor = new EVDS::SchematicsEditor(this,EVDSEditor);
	editorsLayout->addWidget(SchematicsEditor);
	SchematicsEditor->setActive(false);
	//showSchematics();
	//SchematicsEditor->setEditorHidden(true);

	//Delete child on close
	setAttribute(Qt::WA_DeleteOnClose);

	//Enable autosave for this window
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(autoSave()));
	timer->start(fw_editor_settings->value("ui.autosave").toInt());
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::newFile() {
	EVDSEditor->newFile();
	SchematicsEditor->initializeForFile();

	isModified = false;
	currentFile = "";
	updateTitle();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool EditorWindow::loadFile(const QString &fileName) {
	//Setup window
	isModified = false;
	currentFile = fileName;
	updateTitle();

	//Load file data
	if (!EVDSEditor->loadFile(fileName)) return false;
	SchematicsEditor->initializeForFile();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool EditorWindow::saveFile(const QString &fileName, bool autoSave) {
	if (!EVDSEditor->saveFile(fileName)) return false;

	if (!autoSave) {
		isModified = false;
		currentFile = fileName;
		updateTitle();
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::updateInterface(bool isInFront) {
	//EVDSEditor->updateInterface(isInFront);
	//SchematicsEditor->updateInterface(isInFront);
	//for (int i = 0; i < actions.count(); i++) {
		//actions[i]->setVisible(isInFront);
	//}
	//->setVisible(isInFront);

	QHash<Editor*, QAction*>::iterator i;
	for (i = editorsActions.begin(); i != editorsActions.end(); ++i) {
		i.value()->setVisible(isInFront);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::addEditorAction(Editor* editor, QAction* action) {
	editorsActions[editor] = action;
}








////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::showVME() {
	editorsLayout->setCurrentWidget(EVDSEditor);
	SchematicsEditor->setEditorHidden(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::showSchematics() {
	editorsLayout->setCurrentWidget(SchematicsEditor);
	SchematicsEditor->setEditorHidden(false);
}




////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool EditorWindow::save() {
	if (currentFile == "") {
		return saveAs();
	} else {
		return saveFile(currentFile);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::autoSave() {
	if (currentFile != "") {
		saveFile("_auto_" + QFileInfo(currentFile).fileName(),true);
		mainWindow->statusBar()->showMessage(tr("Autosaved..."), 2000);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool EditorWindow::saveAs() {
	QString fileName = QFileDialog::getSaveFileName(this, "Save As", currentFile,
		//"FoxWorks Data Files (*.evds *.ivss);;"
		//"External Vessel Dynamics Simulator (*.evds);;"
		//"Internal Vessel Systems Simulator (*.ivss);;"
		"External Vessel Dynamics Simulator Model (*.evds);;"
		"All files (*.*)");

	if (fileName.isEmpty())	return false;
	return saveFile(fileName);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool EditorWindow::trySave() {
	if (isModified) {
		QMessageBox::StandardButton ret;
		QString fileName = QFileInfo(currentFile).fileName();
		if (currentFile == "") fileName = "New vessel";

		ret = QMessageBox::warning(this, tr("EVDS Editor"),
					 tr("'%1' has been modified.\n"
						"Do you want to save pending changes?")
						.arg(fileName),
					 QMessageBox::Save | QMessageBox::Discard
			 | QMessageBox::Cancel);
		if (ret == QMessageBox::Save)
			return save();
		else if (ret == QMessageBox::Cancel)
			return false;
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::closeEvent(QCloseEvent *event) {
	if (trySave()) {
		event->accept();
	} else {
		event->ignore();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::updateTitle() {
	if (currentFile == "") {
		if (isModified) {
			setWindowTitle("New vessel*");
		} else {
			setWindowTitle("New vessel");
		}
	} else {
		if (isModified) {
			setWindowTitle(QFileInfo(currentFile).fileName() + "*");
		} else {
			setWindowTitle(QFileInfo(currentFile).fileName());
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::cut() {
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::copy() {
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void EditorWindow::paste() {
}





////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::setModified() {
	editorWindow->setModified();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void Editor::addAction(QAction* action) {
	editorWindow->addEditorAction(this,action);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
MainWindow* Editor::getMainWindow() { 
	return editorWindow->getMainWindow();
}