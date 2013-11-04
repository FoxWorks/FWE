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
#ifndef FWE_MAIN_H
#define FWE_MAIN_H

#include <QMainWindow>
#include <QHash>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
class QStackedLayout;
class QSettings;
QT_END_NAMESPACE

////////////////////////////////////////////////////////////////////////////////
extern QSettings* fw_editor_settings; //See fwe.cpp


////////////////////////////////////////////////////////////////////////////////
#define FWE_EDITOR_MAX_RECENT_FILES	10
class ChildWindow;
class PreferencesDialog;
class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void newFile();
	void open();
	void openRecent();
	void save();
	void saveAs();
	void cut();
	void copy();
	void paste();
	void preferences();
	void about();
	void updateInterface();
	void updateWindowMenu();
	void showEVDS();
	void showSchematics();
	ChildWindow *createMdiChild();
	void setActiveSubWindow(QWidget *window);

public:
	QMenu* getFileMenu() { return fileMenu; }
	QMenu* getEditMenu() { return editMenu; }
	QMenu* getViewMenu() { return viewMenu; }
	QMenu* getHelpMenu() { return helpMenu; }

private:
	void createApplicationMenus();

	void createToolBars();
	void createStatusBar();
	void updateRecentFiles();
	void addRecentFile(const QString &fileName);
	ChildWindow *activeMdiChild();
	QMdiSubWindow *findMdiChild(const QString &fileName);

	QMdiArea *mdiArea;
	QSignalMapper *windowMapper;

	//Global dialogs
	PreferencesDialog* preferencesDialog;

	//Main menu bars
	QMenu *fileMenu;
	QMenu *recentMenu;
	QMenu *editMenu;
	QMenu *viewMenu;
	QMenu *windowMenu;
	QMenu *helpMenu;
	QToolBar *fileToolBar;
	QToolBar *editToolBar;

	//Actions
	QHash<QString,QAction*> globalActions;	//List of global actions
	QHash<QString,QAction*> childActions;	//List of actions specific to children
	QList<QAction*> recentFiles;			//List of actions for recent files
};


////////////////////////////////////////////////////////////////////////////////
namespace EVDS {
	class Editor;
	class SchematicsEditor;
}

class ChildWindow : public QMainWindow
{
	Q_OBJECT

public:
	ChildWindow(MainWindow* window);

	void newFile();
	bool loadFile(const QString &fileName);
	bool save();
	bool saveAs();
	bool saveFile(const QString &fileName, bool autoSave = false);

	void updateInterface(bool isInFront);

	void cut();
	void copy();
	void paste();
	void showEVDS();
	void showSchematics();

	void setModified() { isModified = true; updateTitle(); }

protected:
	void closeEvent(QCloseEvent *event);

private:
	bool trySave();
	void updateTitle();
	QString currentFile;
	bool isModified;

public slots:
	void autoSave();

public:
	QString getCurrentFile() { return currentFile; }
	MainWindow* getMainWindow() { return mainWindow; }
protected:
	MainWindow* mainWindow;

	QWidget* editorsWidget;
	QStackedLayout* editorsLayout;
	EVDS::Editor* EVDSEditor;
	EVDS::SchematicsEditor* SchematicsEditor;
};

#endif
