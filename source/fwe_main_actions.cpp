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
#include <QFileDialog>
#include <QMessageBox>
#include <QMdiArea>
#include <QStatusBar>
#include <QMenu>

#include "fwe_main.h"
#include "fwe_evds.h"
#include "fwe_schematics.h"
#include "fwe_dialog_preferences.h"
#include "rdrs.h"



////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::fileNew() {
	ChildWindow *child = createMdiChild();
	child->newFile();
	child->showMaximized();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::fileOpen() {
	QString fileName = QFileDialog::getOpenFileName(this,"Load vessel","",
		//"FoxWorks Data Files (*.evds *.ivss);;"
		//"External Vessel Dynamics Simulator (*.evds);;"
		//"Internal Vessel Systems Simulator (*.ivss);;"
		"External Vessel Dynamics Simulator Model (*.evds);;"
		"All files (*.*)");

	if (!fileName.isEmpty()) {
		addRecentFile(fileName);

		QMdiSubWindow *existing = findMdiChild(fileName);
		if (existing) {
			mdiArea->setActiveSubWindow(existing);
			return;
		}

		ChildWindow *child = createMdiChild();
		if (child->loadFile(fileName)) {
			statusBar()->showMessage(tr("File loaded"), 2000);
			child->show();
		} else {
			child->close();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::fileOpenRecent() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		QMdiSubWindow *existing = findMdiChild(action->data().toString());
		if (existing) {
			mdiArea->setActiveSubWindow(existing);
			addRecentFile(action->data().toString());
			return;
		}

		ChildWindow *child = createMdiChild();
		if (child->loadFile(action->data().toString())) {
			statusBar()->showMessage(tr("File loaded"), 2000);
			child->show();
		} else {
			child->close();
		}
		addRecentFile(action->data().toString());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::fileSave() {
	if (activeMdiChild() && activeMdiChild()->save()) {
		addRecentFile(activeMdiChild()->getCurrentFile());
		statusBar()->showMessage(tr("File saved"), 2000);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::fileSaveAs() {
	if (activeMdiChild() && activeMdiChild()->saveAs()) {
		addRecentFile(activeMdiChild()->getCurrentFile());
		statusBar()->showMessage(tr("File saved"), 2000);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::editCut() {
	if (activeMdiChild())
		activeMdiChild()->cut();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::editCopy() {
	if (activeMdiChild())
		activeMdiChild()->copy();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::editPaste() {
	if (activeMdiChild())
		activeMdiChild()->paste();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::editPreferences() {
	if (!preferencesDialog) {
		preferencesDialog = new PreferencesDialog();
	}
	preferencesDialog->exec();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::editShowVME() {
	globalActions["edit.vessel_model"]->setChecked(true);
	globalActions["edit.schematics"]->setChecked(false);
	if (activeMdiChild()) activeMdiChild()->showEVDS();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::editShowSchematics() {
	globalActions["edit.vessel_model"]->setChecked(false);
	globalActions["edit.schematics"]->setChecked(true);
	if (activeMdiChild()) activeMdiChild()->showSchematics();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::helpAbout() {
	QString ivss_version = "r1a";

	char evds_version_str[64];
	EVDS_Version(0,evds_version_str);
	QString evds_version = "r" + QString(evds_version_str);

	char rdrs_version_str[64];
	RDRS_Version(0,rdrs_version_str);
	QString rdrs_version = "r" + QString(rdrs_version_str);

	QMessageBox::about(this, tr("About FoxWorks Editor"),
			tr("<b>FoxWorks Editor (Alpha)</b> (C) 2012-2013 by Black Phoenix<br>"
			"<br>"
			"Contact information:<br>"
			"<b>Black Phoenix</b> (<i>phoenix@uol.ua</i>, <i>popovych@yanedx.ru</i>)<br>"
			"<br>"
			"Additional software:<br>"
			"<b>GLC_lib</b> (C) 2005-2013 by Laurent Ribon (<i>laumaya@users.sourceforge.net</i>)<br>"
			"<b>External Vessel Dynamics Simulator (%1)</b> (C) 2012-2013 by Black Phoenix<br>"
			//"<b>Internal Vessel Systems Simulator (%2)</b> (C) 2011-2013 by Black Phoenix<br>"
			"<b>Realtime Digital Radio Simulator (%3)</b> (C) 2013 by Black Phoenix"
			)
			.arg(evds_version)
			.arg(ivss_version)
			.arg(rdrs_version));
}