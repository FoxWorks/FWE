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
#ifndef FWE_MAIN_H
#define FWE_MAIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
class ChildWindow;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void cut();
    void copy();
    void paste();
    void about();
    void updateMenus();
    void updateWindowMenu();
    ChildWindow *createMdiChild();
    void setActiveSubWindow(QWidget *window);

public:
	QMenu* getFileMenu() { return fileMenu; }
	QMenu* getEditMenu() { return editMenu; }
	QMenu* getViewMenu() { return viewMenu; }
	QMenu* getHelpMenu() { return helpMenu; }

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    ChildWindow *activeMdiChild();
    QMdiSubWindow *findMdiChild(const QString &fileName);

    QMdiArea *mdiArea;
    QSignalMapper *windowMapper;

    QMenu *fileMenu;
    QMenu *editMenu;
	QMenu *viewMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QToolBar *editToolBar;

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *separatorAct;
    QAction *aboutAct;
};

class ChildWindow : public QMainWindow
{
    Q_OBJECT

public:
	ChildWindow(MainWindow* window);

	void newFile();
	bool loadFile(const QString &fileName);
	bool save();
	bool saveAs();
	bool saveFile(const QString &fileName);

	void updateMenus(bool isInFront);

	void cut();
	void copy();
	void paste();

	void setModified() { isModified = true; updateTitle(); }

protected:
	void closeEvent(QCloseEvent *event);

private:
	bool trySave();
	void updateTitle();
	QString currentFile;
	bool isModified;

public:
	QString getCurrentFile() { return currentFile; }
	MainWindow* getMainWindow() { return mainWindow; }
protected:
	MainWindow* mainWindow;
};

#endif
