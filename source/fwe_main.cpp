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
#include <QIcon>

#include "fwe_main.h"
#include "fwe_evds.h"
#include "fwe_schematics.h"
#include "fwe_dialog_preferences.h"
#include "rdrs.h"


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow() {
	//Create MDI area
	mdiArea = new QMdiArea;
	mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setCentralWidget(mdiArea);
	connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateInterface()));
	windowMapper = new QSignalMapper(this);
	connect(windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));

	//Define default preferences
	fw_editor_settings->setValue ("rendering.lod_count",			
		fw_editor_settings->value("rendering.lod_count",			6));
	fw_editor_settings->setValue ("rendering.lod_quality",		
		fw_editor_settings->value("rendering.lod_quality",			32.0f));
	fw_editor_settings->setValue ("rendering.min_pixel_culling",	
		fw_editor_settings->value("rendering.min_pixel_culling",	4));
	fw_editor_settings->setValue ("rendering.min_resolution",		
		fw_editor_settings->value("rendering.min_resolution",		0.01f));
	fw_editor_settings->setValue ("rendering.no_lods",			
		fw_editor_settings->value("rendering.no_lods",				false));
	fw_editor_settings->setValue ("rendering.use_fxaa",			
		fw_editor_settings->value("rendering.use_fxaa",				true));
	fw_editor_settings->setValue ("rendering.outline_thickness",			
		fw_editor_settings->value("rendering.outline_thickness",	1.0));
	fw_editor_settings->setValue ("ui.autosave",					
		fw_editor_settings->value("ui.autosave",					30000));
	fw_editor_settings->setValue ("screenshot.width",			
		fw_editor_settings->value("screenshot.width",				2048));
	fw_editor_settings->setValue ("screenshot.height",			
		fw_editor_settings->value("screenshot.heght",				0));

	//No preferences dialog
	preferencesDialog = 0;

	//Create everything
	createActionsMenus();

	//createToolBars();
	updateInterface();
	updateRecentFiles();

	//Set default title and size
	QIcon foxworks_icon = QIcon();
	foxworks_icon.addFile(":/icon/foxworks.png");
	foxworks_icon.addFile(":/icon/foxworks256.png",QSize(32,32));
	foxworks_icon.addFile(":/icon/foxworks256.png",QSize(64,64));
	foxworks_icon.addFile(":/icon/foxworks256.png",QSize(128,128));
	foxworks_icon.addFile(":/icon/foxworks256.png",QSize(256,256));
	foxworks_icon.addFile(":/icon/foxworks1024.png");

	setWindowIcon(foxworks_icon);
	setWindowTitle(tr("FoxWorks Editor"));
	setUnifiedTitleAndToolBarOnMac(true);
	//resize(1024,640);

	//Load window settings
	move(fw_editor_settings->value("window.position", QPoint(200, 200)).toPoint());
	resize(fw_editor_settings->value("window.size", QSize(1024, 640)).toSize());

	//Create new empty file or load some file right away
#ifdef _DEBUG
	if (QFile::exists("bug_test_case.evds")) {
		ChildWindow *child = createMdiChild();
		if (child->loadFile("bug_test_case.evds")) {
			child->show();
		} else {
			child->close();
		}
	} else {
		fileNew();
	}
#else
	fileNew();
#endif
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *event) {
	mdiArea->closeAllSubWindows();
	if (mdiArea->currentSubWindow()) {
		event->ignore();
	} else {
		fw_editor_settings->setValue("window.position", pos());
		fw_editor_settings->setValue("window.size", size());
		event->accept();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateInterface() {
	bool hasMdiChild = (activeMdiChild() != 0);

	//Enable or disable all child actions
	QHash<QString, QAction*>::iterator i;
	for (i = childActions.begin(); i != childActions.end(); ++i) {
		i.value()->setEnabled(hasMdiChild);
	}

	//Update menu items for all windows
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		ChildWindow *child = qobject_cast<ChildWindow *>(windows.at(i)->widget());
		if (!child) continue;

		child->updateInterface(child == activeMdiChild());
	}

	//Check if view menu has items
	//bool has_items = false;
	//for (int i = 0; i < viewMenu->actions().count(); i++) {
		//if (viewMenu->actions()[i]->isVisible()) has_items = true;
	//}
	//viewMenu->menuAction()->setVisible(has_items);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateWindowMenu() {
	windowMenu->clear();
	windowMenu->addAction(globalActions["window.tile"]);
	windowMenu->addAction(globalActions["window.cascade"]);
	windowMenu->addSeparator();
	windowMenu->addAction(globalActions["window.next"]);
	windowMenu->addAction(globalActions["window.previous"]);
	windowMenu->addAction(globalActions["window.list_separator"]);

	QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
	globalActions["window.list_separator"]->setVisible(!windows.isEmpty());

	for (int i = 0; i < windows.size(); ++i) {
		ChildWindow *child = qobject_cast<ChildWindow *>(windows.at(i)->widget());
		if (!child) continue; //FIXME: what is this

		QString text;
		if (i < 9) {
			text = tr("&%1 %2").arg(i + 1).arg(child->windowTitle());
		} else {
			text = tr("%1 %2").arg(i + 1).arg(child->windowTitle());
		}
		QAction *action  = windowMenu->addAction(text);
		action->setCheckable(true);
		action ->setChecked(child == activeMdiChild());
		connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
		windowMapper->setMapping(action, windows.at(i));
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateRecentFiles() {
	QStringList files = fw_editor_settings->value("ui.recent_files").toStringList();
	int numRecentFiles = qMin(files.size(), (int)FWE_EDITOR_MAX_RECENT_FILES);
	bool hasRecentFiles = numRecentFiles > 0;

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		recentFiles[i]->setText(text);
		recentFiles[i]->setData(files[i]);
		recentFiles[i]->setStatusTip(files[i]);
		recentFiles[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < FWE_EDITOR_MAX_RECENT_FILES; ++j) {
		recentFiles[j]->setVisible(false);
	}

	recentMenu->setEnabled(hasRecentFiles);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::addRecentFile(const QString &fileName) {
	QStringList files = fw_editor_settings->value("ui.recent_files").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);

	while (files.size() > FWE_EDITOR_MAX_RECENT_FILES) {
		files.removeLast();
	}

	fw_editor_settings->setValue("ui.recent_files", files);
	updateRecentFiles();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ChildWindow *MainWindow::createMdiChild() {
	ChildWindow *child = new ChildWindow(this);
	mdiArea->addSubWindow(child);
	mdiArea->setWindowIcon(QIcon(":/icon/mdi.png"));

	updateInterface();
	return child;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::createActionsMenus() {
	QAction* action;


	//--------------------------------------------------------------------------
	// File menu
	//--------------------------------------------------------------------------
	fileMenu = menuBar()->addMenu(tr("&File"));

	action = new QAction(QIcon(":/icon/new.png"), tr("&New"), this);
	action->setShortcuts(QKeySequence::New);
	action->setStatusTip(tr("Create a new file"));
	connect(action, SIGNAL(triggered()), this, SLOT(fileNew()));
	fileMenu->addAction(action);
	globalActions["file.new"] = action;

	action = new QAction(QIcon(":/icon/open.png"), tr("&Open..."), this);
	action->setShortcuts(QKeySequence::Open);
	action->setStatusTip(tr("Open an existing file"));
	connect(action, SIGNAL(triggered()), this, SLOT(fileOpen()));
	fileMenu->addAction(action);
	globalActions["file.open"] = action;

	//Recent menu and menu items
	recentMenu = fileMenu->addMenu(tr("&Open Recent"));
	for (int i = 0; i < FWE_EDITOR_MAX_RECENT_FILES; ++i) {
		action = new QAction(this);
		action->setVisible(false);
		connect(action, SIGNAL(triggered()), this, SLOT(fileOpenRecent()));

		recentFiles.append(action);
		recentMenu->addAction(action);
	}

	action = new QAction(QIcon(":/icon/save.png"), tr("&Save"), this);
	action->setShortcuts(QKeySequence::Save);
	action->setStatusTip(tr("Save the document to disk"));
	connect(action, SIGNAL(triggered()), this, SLOT(fileSave()));
	fileMenu->addAction(action);
	globalActions["file.save"] = action;

	action = new QAction(QIcon(":/icon/save_as.png"), tr("Save &As..."), this);
	action->setShortcuts(QKeySequence::SaveAs);
	action->setStatusTip(tr("Save the document under a new name"));
	connect(action, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	fileMenu->addAction(action);
	globalActions["file.saveAs"] = action;

	fileMenu->addSeparator(); //------------------------------------------------

	action = new QAction(QIcon(":/icon/close.png"), tr("Cl&ose"), this);
	action->setStatusTip(tr("Close the active window"));
	connect(action, SIGNAL(triggered()), mdiArea, SLOT(closeActiveSubWindow()));
	fileMenu->addAction(action);
	globalActions["file.close"] = action;

	action = new QAction(tr("Close &All"), this);
	action->setStatusTip(tr("Close all the windows"));
	connect(action, SIGNAL(triggered()), mdiArea, SLOT(closeAllSubWindows()));
	fileMenu->addAction(action);
	globalActions["file.closeall"] = action;
	
	fileMenu->addSeparator(); //------------------------------------------------

	action = new QAction(QIcon(":/icon/exit.png"), tr("E&xit"), this);
	action->setShortcuts(QKeySequence::Quit);
	action->setStatusTip(tr("Exit the application"));
	connect(action, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	fileMenu->addAction(action);
	globalActions["file.exit"] = action;


	//--------------------------------------------------------------------------
	// Edit menu
	//--------------------------------------------------------------------------
	editMenu = menuBar()->addMenu(tr("&Edit"));

	action = new QAction(QIcon(":/icon/cut.png"), tr("Cu&t"), this);
	action->setShortcuts(QKeySequence::Cut);
	action->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
	connect(action, SIGNAL(triggered()), this, SLOT(editCut()));
	editMenu->addAction(action);
	globalActions["edit.cut"] = action;

	action = new QAction(QIcon(":/icon/copy.png"), tr("&Copy"), this);
	action->setShortcuts(QKeySequence::Copy);
	action->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
	connect(action, SIGNAL(triggered()), this, SLOT(editCopy()));
	editMenu->addAction(action);
	globalActions["edit.copy"] = action;

	action = new QAction(QIcon(":/icon/paste.png"), tr("&Paste"), this);
	action->setShortcuts(QKeySequence::Paste);
	action->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
	connect(action, SIGNAL(triggered()), this, SLOT(editPaste()));
	editMenu->addAction(action);
	globalActions["edit.paste"] = action;

	editMenu->addSeparator(); //------------------------------------------------

	action = new QAction(tr("Vessel Model"), this);
	action->setStatusTip(tr("Show vessel model editor"));
	action->setCheckable(true);
	connect(action, SIGNAL(triggered()), this, SLOT(editShowVME()));
	editMenu->addAction(action);
	globalActions["edit.vessel_model"] = action;

	action = new QAction(tr("Schematics"), this);
	action->setStatusTip(tr("Show schematics editor"));
	action->setCheckable(true);
	connect(action, SIGNAL(triggered()), this, SLOT(editShowSchematics()));
	editMenu->addAction(action);
	globalActions["edit.schematics"] = action;

	editShowVME(); //FIXME

	editMenu->addSeparator(); //------------------------------------------------

	action = new QAction(QIcon(":/icon/preferences.png"), tr("&Preferences..."), this);
	action->setStatusTip(tr("Edit FoxWorks Editor preferences and configuration"));
	connect(action, SIGNAL(triggered()), this, SLOT(editPreferences()));
	editMenu->addAction(action);
	globalActions["edit.preferences"] = action;


	//--------------------------------------------------------------------------
	// View menu
	//--------------------------------------------------------------------------
	viewMenu = menuBar()->addMenu(tr("&View"));


	//--------------------------------------------------------------------------
	// Window menu
	//--------------------------------------------------------------------------
	windowMenu = menuBar()->addMenu(tr("&Window"));

	action = new QAction(tr("&Tile"), this);
	action->setStatusTip(tr("Tile the windows"));
	connect(action, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));
	globalActions["window.tile"] = action;

	action = new QAction(tr("&Cascade"), this);
	action->setStatusTip(tr("Cascade the windows"));
	connect(action, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));
	globalActions["window.cascade"] = action;

	action = new QAction(QIcon(":/icon/next.png"), tr("Ne&xt"), this);
	action->setShortcuts(QKeySequence::NextChild);
	action->setStatusTip(tr("Move the focus to the next window"));
	connect(action, SIGNAL(triggered()), mdiArea, SLOT(activateNextSubWindow()));
	globalActions["window.next"] = action;

	action = new QAction(QIcon(":/icon/previous.png"), tr("Pre&vious"), this);
	action->setShortcuts(QKeySequence::PreviousChild);
	action->setStatusTip(tr("Move the focus to the previous window"));
	connect(action, SIGNAL(triggered()), mdiArea, SLOT(activatePreviousSubWindow()));
	globalActions["window.previous"] = action;

	action = new QAction(this);
	action->setSeparator(true);
	globalActions["window.list_separator"] = action;


	//--------------------------------------------------------------------------
	// Help menu
	//--------------------------------------------------------------------------
	helpMenu = menuBar()->addMenu(tr("&Window"));

	action = new QAction(QIcon(":/icon/about.png"), tr("&About"), this);
	action->setStatusTip(tr("Show the About box"));
	connect(action, SIGNAL(triggered()), this, SLOT(helpAbout()));
	helpMenu->addAction(action);
	globalActions["help.about"] = action;


	///....
	updateWindowMenu();
	connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

	menuBar()->addSeparator();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::createToolBars() {
	fileToolBar = addToolBar(tr("File"));
	fileToolBar->addAction(globalActions["file.new"]);
	fileToolBar->addAction(globalActions["file.open"]);
	fileToolBar->addAction(globalActions["file.save"]);

	editToolBar = addToolBar(tr("Edit"));
	editToolBar->addAction(globalActions["file.cut"]);
	editToolBar->addAction(globalActions["file.copy"]);
	editToolBar->addAction(globalActions["file.paste"]);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ChildWindow *MainWindow::activeMdiChild() {
	if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
		return qobject_cast<ChildWindow *>(activeSubWindow->widget());
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName) {
	QString canonicalFilePath = fileName; //FIXME //QFileInfo(fileName).canonicalFilePath();

	foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
		ChildWindow *mdiChild = qobject_cast<ChildWindow *>(window->widget());
		if (!mdiChild) continue; //FIXME: what is this
		if (mdiChild->getCurrentFile() == canonicalFilePath)
			return window;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::setActiveSubWindow(QWidget *window) {
	if (!window)
		return;
	mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}








////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
ChildWindow::ChildWindow(MainWindow* window) { 
	mainWindow = window;
	editorsWidget = new QWidget(this);
	editorsLayout = new QStackedLayout(editorsWidget);
	setCentralWidget(editorsWidget);

	//Create EVDS editor
	EVDSEditor = new EVDS::Editor(this);
	editorsLayout->addWidget(EVDSEditor);

	//Create schematics editor
	SchematicsEditor = new EVDS::SchematicsEditor(this,EVDSEditor);
	editorsLayout->addWidget(SchematicsEditor);
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
void ChildWindow::showEVDS() {
	editorsLayout->setCurrentWidget(EVDSEditor);
	SchematicsEditor->setEditorHidden(true);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::showSchematics() {
	editorsLayout->setCurrentWidget(SchematicsEditor);
	SchematicsEditor->setEditorHidden(false);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::newFile() {
	EVDSEditor->newFile();
	SchematicsEditor->initializeForFile();

	isModified = false;
	currentFile = "";
	updateTitle();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ChildWindow::loadFile(const QString &fileName) {
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
bool ChildWindow::saveFile(const QString &fileName, bool autoSave) {
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
bool ChildWindow::save() {
	if (currentFile == "") {
		return saveAs();
	} else {
		return saveFile(currentFile);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::autoSave() {
	if (currentFile != "") {
		saveFile("_auto_" + QFileInfo(currentFile).fileName(),true);
		mainWindow->statusBar()->showMessage(tr("Autosaved..."), 2000);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ChildWindow::saveAs() {
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
bool ChildWindow::trySave() {
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
void ChildWindow::closeEvent(QCloseEvent *event) {
	if (trySave()) {
		event->accept();
	} else {
		event->ignore();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::updateTitle() {
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
void ChildWindow::updateInterface(bool isInFront) {
	EVDSEditor->updateInterface(isInFront);
	SchematicsEditor->updateInterface(isInFront);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::cut() {
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::copy() {
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::paste() {
}