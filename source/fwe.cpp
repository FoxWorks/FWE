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
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QFontDatabase>
//#include <QCleanlooksStyle>

#include "fwe.h"
#include "fwe_main.h"

/// Stores current FWE flags
int fw_editor_flags = 0;

/// FoxWorks application
QApplication* fw_application;

/// FoxWorks main window
MainWindow* fw_mainWindow;

/// FoxWorks editor settings
QSettings* fw_editor_settings = 0;


////////////////////////////////////////////////////////////////////////////////
/// @brief Qt messages
////////////////////////////////////////////////////////////////////////////////
void fw_editor_message(QtMsgType type, const char *msg) {
	switch (type) {
		case QtDebugMsg:
			fprintf(stderr, "[ ] %s\n", msg);
			break;
		case QtWarningMsg:
			fprintf(stderr, "[!] %s\n", msg);
			break;
		case QtCriticalMsg:
			fprintf(stderr, "Critical: %s\n", msg);
			break;
		case QtFatalMsg:
			fprintf(stderr, "Fatal: %s\n", msg);
#ifdef _DEBUG
			_asm { int 3 };
			abort();
#endif
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize FoxWorks Editor with flags
////////////////////////////////////////////////////////////////////////////////
void fw_editor_initialize(int flags, int argc, char *argv[]) {
	fw_editor_flags = flags;

	if (flags & FOXWORKS_EDITOR_STANDALONE) {
		qInstallMsgHandler(fw_editor_message);

		fw_application = new QApplication(argc,argv);
		Q_INIT_RESOURCE(resources);

		fw_editor_settings = new QSettings("settings.ini",QSettings::IniFormat);
		fw_mainWindow = new MainWindow(); //Show main window
		fw_mainWindow->show();

		//Load default fonts
		QFontDatabase fontDatabase; 
		fontDatabase.addApplicationFont(":/gost_2_304-81_type_b.ttf");

		//Load default stylesheet
		QFile stylesheet(":/stylesheet.qss");
		stylesheet.open(QFile::ReadOnly);
		fw_application->setStyleSheet(QLatin1String(stylesheet.readAll()));

		//fw_application->setStyle(new QCleanlooksStyle);
	}

	if (flags & FOXWORKS_EDITOR_BLOCKING) {
		fw_application->exec();
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Shutdown and clean up all resources
////////////////////////////////////////////////////////////////////////////////
void fw_editor_deinitialize() {
	delete fw_editor_settings;
	delete fw_application;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Call from main application OpenGL rendering frame if not standalone
////////////////////////////////////////////////////////////////////////////////
void fw_editor_frame() {

}


////////////////////////////////////////////////////////////////////////////////
/// @brief Main call (used when compiling standalone)
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	fw_editor_initialize(FOXWORKS_EDITOR_STANDALONE | FOXWORKS_EDITOR_BLOCKING,argc,argv);
}