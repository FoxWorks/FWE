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

	//Create everything
	createActions();
	createMenus();
	//createToolBars();
	createStatusBar();
	updateInterface();

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

	//Create new empty file
	newFile();
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
void MainWindow::save() {
	if (activeMdiChild() && activeMdiChild()->save())
		statusBar()->showMessage(tr("File saved"), 2000);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::saveAs() {
	if (activeMdiChild() && activeMdiChild()->saveAs())
		statusBar()->showMessage(tr("File saved"), 2000);
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
void MainWindow::about() {
	//char evds_version_str[64];
	//EVDS_Version(0,evds_version_str);
	//QString evds_version(evds_version_str);
	QString evds_version = "r1a";
	QString ivss_version = "r1a";
	QString rdrs_version = "r1a";

	QMessageBox::about(this, tr("About FoxWorks Editor"),
			tr("<b>FoxWorks Editor</b> (C) 2012-2013 by Black Phoenix<br>"
			"<b>External Vessel Dynamics Simulator (EVDS %1)</b> (C) 2012-2013 by Black Phoenix<br>"
			"<b>Internal Vessel Systems Simulator (IVSS %2)</b> (C) 2011-2013 by Black Phoenix<br>"
			"<b>Realtime Digital Radio Simulator (RDRS %3)</b> (C) 2013 by Black Phoenix")
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
	pasteAct->setEnabled(hasMdiChild);
	copyAct->setEnabled(hasMdiChild);
	cutAct->setEnabled(hasMdiChild);
	closeAct->setEnabled(hasMdiChild);
	closeAllAct->setEnabled(hasMdiChild);
	tileAct->setEnabled(hasMdiChild);
	cascadeAct->setEnabled(hasMdiChild);
	nextAct->setEnabled(hasMdiChild);
	previousAct->setEnabled(hasMdiChild);
	separatorAct->setVisible(hasMdiChild);

	//Update menu items for all windows
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		ChildWindow *child = qobject_cast<ChildWindow *>(windows.at(i)->widget());
		if (!child) continue;

		child->updateInterface(child == activeMdiChild());
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateWindowMenu() {
	windowMenu->clear();
	windowMenu->addAction(closeAct);
	windowMenu->addAction(closeAllAct);
	windowMenu->addSeparator();
	windowMenu->addAction(tileAct);
	windowMenu->addAction(cascadeAct);
	windowMenu->addSeparator();
	windowMenu->addAction(nextAct);
	windowMenu->addAction(previousAct);
	windowMenu->addAction(separatorAct);

	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	separatorAct->setVisible(!windows.isEmpty());

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

	separatorAct = new QAction(this);
	separatorAct->setSeparator(true);

	aboutAct = new QAction(QIcon(":/icon/about.png"), tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the About box"));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMenus() {
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(newAct);
	fileMenu->addAction(openAct);
	fileMenu->addAction(saveAct);
	fileMenu->addAction(saveAsAct);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAct);
	editMenu->addAction(copyAct);
	editMenu->addAction(pasteAct);

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
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

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

	EVDSEditor = new EVDS::Editor(this);
	editorsLayout->addWidget(EVDSEditor);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void ChildWindow::newFile() {
	EVDSEditor->newFile();

	currentFile = "";
	isModified = false;
	updateTitle();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ChildWindow::loadFile(const QString &fileName) {
	if (!EVDSEditor->loadFile(fileName)) return false;

	//Setup window
	updateTitle();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
bool ChildWindow::saveFile(const QString &fileName) {
	if (!EVDSEditor->saveFile(fileName)) return false;

	isModified = false;
	currentFile = fileName;
	updateTitle();
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