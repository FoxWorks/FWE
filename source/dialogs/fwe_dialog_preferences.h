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
#ifndef FWE_DIALOG_PREFERENCES_H
#define FWE_DIALOG_PREFERENCES_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidget;
class QStackedWidget;
class QListWidgetItem;
class QLabel;
QT_END_NAMESPACE


////////////////////////////////////////////////////////////////////////////////
class PreferencesDialog : public QDialog {
	Q_OBJECT

public:
	PreferencesDialog();

private slots:
	void apply();
	void changePage(QListWidgetItem *current, QListWidgetItem *previous);
	void setInteger(int value);
	void setIntegerWarn(int value);
	void setIntegerx1000(int value);
	void setBool(int value);
	void setBoolWarn(int value);
	void setDouble(double value);
	void setDoubleWarn(double value);

private:
	void createPerfomance();
	void createEngineering();
	void createOther();

	QLabel* needReload;
	QListWidget* pages;
	QStackedWidget* contents;
};

#endif
