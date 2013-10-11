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
	createActions();
	createMenus();
	//createToolBars();
	createStatusBar();
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
	resize(1024,640);

	//Load settings
	//QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
	//QSize size = settings.value("size", QSize(400, 400)).toSize();
	//move(pos);
	//resize(size);

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
		newFile();
	}
#else
	newFile();
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
void MainWindow::newFile() {
	ChildWindow *child = createMdiChild();
	child->newFile();
	child->showMaximized();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::open() {
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
void MainWindow::openRecent() {
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
void MainWindow::save() {
	if (activeMdiChild() && activeMdiChild()->save()) {
		addRecentFile(activeMdiChild()->getCurrentFile());
		statusBar()->showMessage(tr("File saved"), 2000);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::saveAs() {
	if (activeMdiChild() && activeMdiChild()->saveAs()) {
		addRecentFile(activeMdiChild()->getCurrentFile());
		statusBar()->showMessage(tr("File saved"), 2000);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::cut() {
	if (activeMdiChild())
		activeMdiChild()->cut();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::copy() {
	if (activeMdiChild())
		activeMdiChild()->copy();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::paste() {
	if (activeMdiChild())
		activeMdiChild()->paste();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::preferences() {
	if (!preferencesDialog) {
		preferencesDialog = new PreferencesDialog();
	}
	preferencesDialog->exec();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::showEVDS() {
	if (activeMdiChild()) activeMdiChild()->showEVDS();
}
void MainWindow::showSchematics() {
	if (activeMdiChild()) activeMdiChild()->showSchematics();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::about() {
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


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateInterface() {
	bool hasMdiChild = (activeMdiChild() != 0);
	saveAct->setEnabled(hasMdiChild);
	saveAsAct->setEnabled(hasMdiChild);
	pasteAct->setEnabled(false);//hasMdiChild);
	copyAct->setEnabled(false);//hasMdiChild);
	cutAct->setEnabled(false);//hasMdiChild);
	closeAct->setEnabled(hasMdiChild);
	closeAllAct->setEnabled(hasMdiChild);
	tileAct->setEnabled(hasMdiChild);
	cascadeAct->setEnabled(hasMdiChild);
	nextAct->setEnabled(hasMdiChild);
	previousAct->setEnabled(hasMdiChild);
	windowSeparatorAct->setVisible(hasMdiChild);

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
	windowMenu->addAction(tileAct);
	windowMenu->addAction(cascadeAct);
	windowMenu->addSeparator();
	windowMenu->addAction(nextAct);
	windowMenu->addAction(previousAct);
	windowMenu->addAction(windowSeparatorAct);

	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	windowSeparatorAct->setVisible(!windows.isEmpty());

	for (int i = 0; i < windows.size(); ++i) {
		ChildWindow *child = qobject_cast<ChildWindow *>(windows.at(i)->widget());
		if (!child) continue; //FIXME: what is this

		QString text;
		if (i < 9) {
			text = tr("&%1 %2").arg(i + 1)
							   .arg(child->windowTitle());
		} else {
			text = tr("%1 %2").arg(i + 1)
							  .arg(child->windowTitle());
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

	fileSeparatorAct->setVisible(numRecentFiles > 0);
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
void MainWindow::createActions() {
	newAct = new QAction(QIcon(":/icon/new.png"), tr("&New"), this);
	newAct->setShortcuts(QKeySequence::New);
	newAct->setStatusTip(tr("Create a new file"));
	connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

	openAct = new QAction(QIcon(":/icon/open.png"), tr("&Open..."), this);
	openAct->setShortcuts(QKeySequence::Open);
	openAct->setStatusTip(tr("Open an existing file"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

	saveAct = new QAction(QIcon(":/icon/save.png"), tr("&Save"), this);
	saveAct->setShortcuts(QKeySequence::Save);
	saveAct->setStatusTip(tr("Save the document to disk"));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

	saveAsAct = new QAction(QIcon(":/icon/save_as.png"), tr("Save &As..."), this);
	saveAsAct->setShortcuts(QKeySequence::SaveAs);
	saveAsAct->setStatusTip(tr("Save the document under a new name"));
	connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

	exitAct = new QAction(QIcon(":/icon/exit.png"), tr("E&xit"), this);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

	fileSeparatorAct = new QAction(this);
	fileSeparatorAct->setSeparator(true);

	for (int i = 0; i < FWE_EDITOR_MAX_RECENT_FILES; ++i) {
		recentFiles[i] = new QAction(this);
		recentFiles[i]->setVisible(false);
		connect(recentFiles[i], SIGNAL(triggered()), this, SLOT(openRecent()));
	}

	cutAct = new QAction(QIcon(":/icon/cut.png"), tr("Cu&t"), this);
	cutAct->setShortcuts(QKeySequence::Cut);
	cutAct->setStatusTip(tr("Cut the current selection's contents to the "
							"clipboard"));
	connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

	copyAct = new QAction(QIcon(":/icon/copy.png"), tr("&Copy"), this);
	copyAct->setShortcuts(QKeySequence::Copy);
	copyAct->setStatusTip(tr("Copy the current selection's contents to the "
							 "clipboard"));
	connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

	pasteAct = new QAction(QIcon(":/icon/paste.png"), tr("&Paste"), this);
	pasteAct->setShortcuts(QKeySequence::Paste);
	pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
							  "selection"));
	connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

	preferencesAct = new QAction(QIcon(":/icon/preferences.png"), tr("&Preferences..."), this);
	preferencesAct->setStatusTip(tr("Edit FoxWorks Editor preferences and configuration"));
	connect(preferencesAct, SIGNAL(triggered()), this, SLOT(preferences()));

	closeAct = new QAction(QIcon(":/icon/close.png"), tr("Cl&ose"), this);
	closeAct->setStatusTip(tr("Close the active window"));
	connect(closeAct, SIGNAL(triggered()),
			mdiArea, SLOT(closeActiveSubWindow()));

	closeAllAct = new QAction(tr("Close &All"), this);
	closeAllAct->setStatusTip(tr("Close all the windows"));
	connect(closeAllAct, SIGNAL(triggered()),
			mdiArea, SLOT(closeAllSubWindows()));

	tileAct = new QAction(tr("&Tile"), this);
	tileAct->setStatusTip(tr("Tile the windows"));
	connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

	cascadeAct = new QAction(tr("&Cascade"), this);
	cascadeAct->setStatusTip(tr("Cascade the windows"));
	connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

	nextAct = new QAction(QIcon(":/icon/next.png"), tr("Ne&xt"), this);
	nextAct->setShortcuts(QKeySequence::NextChild);
	nextAct->setStatusTip(tr("Move the focus to the next window"));
	connect(nextAct, SIGNAL(triggered()),
			mdiArea, SLOT(activateNextSubWindow()));

	previousAct = new QAction(QIcon(":/icon/previous.png"), tr("Pre&vious"), this);
	previousAct->setShortcuts(QKeySequence::PreviousChild);
	previousAct->setStatusTip(tr("Move the focus to the previous window"));
	connect(previousAct, SIGNAL(triggered()),
			mdiArea, SLOT(activatePreviousSubWindow()));

	windowSeparatorAct = new QAction(this);
	windowSeparatorAct->setSeparator(true);

	aboutAct = new QAction(QIcon(":/icon/about.png"), tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the About box"));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));


	evdsAct = new QAction(tr("EVDS Editor"), this);
	evdsAct->setStatusTip(tr("Show EVDS editor"));
	connect(evdsAct, SIGNAL(triggered()), this, SLOT(showEVDS()));

	schematicsAct = new QAction(tr("Schematics Editor"), this);
	schematicsAct->setStatusTip(tr("Show schematics editor"));
	connect(schematicsAct, SIGNAL(triggered()), this, SLOT(showSchematics()));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMenus() {
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(newAct);
	fileMenu->addAction(openAct);
	recentMenu = fileMenu->addMenu(tr("&Open Recent"));
	for (int i = 0; i < FWE_EDITOR_MAX_RECENT_FILES; ++i) {
		recentMenu->addAction(recentFiles[i]);
	}
	//fileMenu->addSeparator();
	fileMenu->addAction(saveAct);
	fileMenu->addAction(saveAsAct);
	fileMenu->addSeparator();
	fileMenu->addAction(closeAct);
	fileMenu->addAction(closeAllAct);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAct);
	editMenu->addAction(copyAct);
	editMenu->addAction(pasteAct);
	editMenu->addSeparator();
	editMenu->addAction(evdsAct);
	editMenu->addAction(schematicsAct);
	editMenu->addSeparator();
	editMenu->addAction(preferencesAct);

	viewMenu = menuBar()->addMenu(tr("&View"));

	windowMenu = menuBar()->addMenu(tr("&Window"));
	updateWindowMenu();
	connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

	menuBar()->addSeparator();

	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAct);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::createToolBars() {
	fileToolBar = addToolBar(tr("File"));
	fileToolBar->addAction(newAct);
	fileToolBar->addAction(openAct);
	fileToolBar->addAction(saveAct);

	editToolBar = addToolBar(tr("Edit"));
	editToolBar->addAction(cutAct);
	editToolBar->addAction(copyAct);
	editToolBar->addAction(pasteAct);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::createStatusBar() {
	statusBar()->showMessage(tr("Ready"));
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
	//SchematicsEditor->initializeForFile();
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
	//SchematicsEditor->updateInterface(isInFront);
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